
#include "SkinAnimMesh.h"

#include <exception>

#include "Common.h"
#include "Util.h"
#include "SkinAnimMeshAlloc.h"

namespace NSRender
{
const std::wstring SkinAnimMesh::SHADER_FILENAME = L"res\\shader\\SkinAnimMeshShader.fx";

void SkinAnimMesh::frame_root_deleter_object::operator()(const LPD3DXFRAME frame_root)
{
    release_mesh_allocator(frame_root);
}

void SkinAnimMesh::frame_root_deleter_object::release_mesh_allocator(
    const LPD3DXFRAME frame)
{
    if (frame->pMeshContainer != nullptr)
    {
        allocator_->DestroyMeshContainer(frame->pMeshContainer);
    }

    if (frame->pFrameSibling != nullptr)
    {
        release_mesh_allocator(frame->pFrameSibling);
    }

    if (frame->pFrameFirstChild != nullptr)
    {
        release_mesh_allocator(frame->pFrameFirstChild);
    }

    allocator_->DestroyFrame(frame);
}

SkinAnimMesh::SkinAnimMesh(const std::wstring &x_filename,
                           const D3DXVECTOR3 &position,
                           const D3DXVECTOR3 &rotation,
                           const float &scale,
                           const AnimSetMap& animSetMap)
    : allocator_{NEW SkinAnimMeshAlloc(x_filename)},
      frame_root_{nullptr, frame_root_deleter_object{allocator_}},
      rotation_matrix_{D3DMATRIX{}},
      center_coodinate_{0.0f, 0.0f, 0.0f},
      view_projection_handle_{},
      scale_handle_{},
      scale_(scale),
      position_(position),
      rotation_(rotation)
{
    HRESULT hr = E_FAIL;

    hr = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                  SHADER_FILENAME.c_str(),
                                  nullptr,
                                  nullptr,
                                  0,
                                  nullptr,
                                  &m_D3DEffect,
                                  nullptr);

    if (FAILED(hr))
    {
        throw std::exception("Failed to create an effect file.");
    }

    view_projection_handle_ = m_D3DEffect->GetParameterByName(nullptr, "g_view_projection");
    LPD3DXFRAME temp_frame_root = NULL;
    LPD3DXANIMATIONCONTROLLER temp_animation_controller = NULL;

    if (FAILED(D3DXLoadMeshHierarchyFromX(x_filename.c_str(),
                                          D3DXMESH_MANAGED,
                                          Common::D3DDevice(),
                                          allocator_.get(),
                                          nullptr,
                                          &temp_frame_root,
                                          &temp_animation_controller)))
    {
        auto msg = L"Failed to load a x-file.: " + x_filename;
        auto msg2 = Util::WstringToUtf8(msg);
        throw std::exception(msg2.c_str());
    }

    // lazy initialization 
    frame_root_.reset(temp_frame_root);
    m_animCtrlr.Init(temp_animation_controller, animSetMap);

    allocate_all_bone_matrices(frame_root_.get());

    scale_ = scale;
}

SkinAnimMesh::~SkinAnimMesh()
{
}

void SkinAnimMesh::Render(const D3DXMATRIX& view_matrix,
                          const D3DXMATRIX& projection_matrix,
                          const D3DXVECTOR4& light_normal,
                          const float& brightness)
{
    m_D3DEffect->SetVector("g_light_normal", &light_normal);
    m_D3DEffect->SetFloat("g_light_brightness", brightness);

    render_impl(view_matrix, projection_matrix);
}

void SkinAnimMesh::render_impl(const D3DXMATRIX &view_matrix,
                               const D3DXMATRIX &projection_matrix)
{
    D3DXMATRIX view_projection_matrix = view_matrix * projection_matrix;

    m_D3DEffect->SetMatrix(view_projection_handle_, &view_projection_matrix);

    m_animCtrlr.Update();

    D3DXMATRIX world_matrix;
    D3DXMatrixIdentity(&world_matrix);
    {
        D3DXMATRIX mat;
        D3DXMatrixTranslation(
            &mat, -center_coodinate_.x, -center_coodinate_.y, -center_coodinate_.z);
        world_matrix *= mat;

        D3DXMatrixScaling(&mat, scale_, scale_, scale_);
        world_matrix *= mat;

        D3DXMatrixRotationYawPitchRoll(&mat, rotation_.y, rotation_.x, rotation_.z);
        world_matrix *= mat;

        D3DXMatrixTranslation(&mat, position_.x, position_.y, position_.z);
        world_matrix *= mat;
    }

    update_frame_matrix(frame_root_.get(), &world_matrix);
    render_frame(frame_root_.get());
}

void SkinAnimMesh::update_frame_matrix(const LPD3DXFRAME frame_base,
                                       const LPD3DXMATRIX parent_matrix)
{
    auto frame = (SkinAnimMeshFrame*)frame_base;
    
    if (parent_matrix != nullptr)
    {
        frame->m_combinedMatrix = frame->TransformationMatrix * (*parent_matrix);
    }
    else
    {
        frame->m_combinedMatrix = frame->TransformationMatrix;
    }

    if (frame->pFrameSibling != nullptr)
    {
        update_frame_matrix(frame->pFrameSibling, parent_matrix);
    }

    if (frame->pFrameFirstChild != nullptr)
    {
        update_frame_matrix(frame->pFrameFirstChild, &frame->m_combinedMatrix);
    }
}

void SkinAnimMesh::render_frame(const LPD3DXFRAME frame)
{
    {
        LPD3DXMESHCONTAINER mesh_container{frame->pMeshContainer};
        while (mesh_container != nullptr)
        {
            render_mesh_container(mesh_container);
            mesh_container = mesh_container->pNextMeshContainer;
        }
    }

    if (frame->pFrameSibling != nullptr)
    {
        render_frame(frame->pFrameSibling);
    }

    if (frame->pFrameFirstChild != nullptr)
    {
        render_frame(frame->pFrameFirstChild);
    }
}

void SkinAnimMesh::render_mesh_container(const LPD3DXMESHCONTAINER mesh_container_base)
{
    auto mesh_container = (SkinAnimMeshContainer*)mesh_container_base;

    LPD3DXBONECOMBINATION bone_combination = NULL;

    bone_combination = (LPD3DXBONECOMBINATION)mesh_container->m_boneBuffer->GetBufferPointer();

    const DWORD dw_palette_size = mesh_container->m_paletteSize;

    for (DWORD i = 0; i < mesh_container->m_boneCount; ++i)
    {
        for (DWORD k = 0; k < dw_palette_size; ++k)
        {
            DWORD dw_bone_id = bone_combination[i].BoneId[k];
            if (dw_bone_id == UINT_MAX)
            {
                continue;
            }
            world_matrix_array_[k] =
                mesh_container->m_boneOffsetMatrices[dw_bone_id] *
                (*mesh_container->m_frameCombinedMatrix[dw_bone_id]);
        }
        m_D3DEffect->SetMatrixArray("g_world_matrix_array",
                                    &world_matrix_array_[0],
                                    dw_palette_size);

        DWORD bone_id = bone_combination[i].AttribId;
        D3DXVECTOR4 vec4_color{
            mesh_container->pMaterials[bone_id].MatD3D.Diffuse.r,
            mesh_container->pMaterials[bone_id].MatD3D.Diffuse.g,
            mesh_container->pMaterials[bone_id].MatD3D.Diffuse.b,
            mesh_container->pMaterials[bone_id].MatD3D.Diffuse.a};

        m_D3DEffect->SetVector("g_diffuse", &vec4_color);

        if (bone_id < mesh_container->m_textureList.size())
        {
            m_D3DEffect->SetTexture("g_mesh_texture",
                                    mesh_container->m_textureList.at(bone_id));
        }

        m_D3DEffect->Begin(nullptr, 0);

        if (FAILED(m_D3DEffect->BeginPass(0)))
        {
            m_D3DEffect->End();
            throw std::exception("Failed 'BeginPass' function.");
        }
        m_D3DEffect->CommitChanges();
        mesh_container->MeshData.pMesh->DrawSubset(i);
        m_D3DEffect->EndPass();
        m_D3DEffect->End();
    }
}

HRESULT SkinAnimMesh::allocate_bone_matrix(LPD3DXMESHCONTAINER mesh_container)
{
    SkinAnimMeshFrame *frame = nullptr;

    auto skinned_mesh_container = (SkinAnimMeshContainer*)mesh_container;

    DWORD bone_count = skinned_mesh_container->pSkinInfo->GetNumBones();
    skinned_mesh_container->m_frameCombinedMatrix.resize(bone_count);

    // TODO Improve.
    DWORD MAX_MATRICES = 26;
    world_matrix_array_.resize((std::min)(MAX_MATRICES, bone_count));

    m_D3DEffect->SetInt("current_bone_numbers", skinned_mesh_container->m_influenceCount - 1);

    for (DWORD i = 0; i < bone_count; ++i)
    {
        LPD3DXFRAME p = D3DXFrameFind(frame_root_.get(),
                                      skinned_mesh_container->pSkinInfo->GetBoneName(i));

        frame = (SkinAnimMeshFrame*)p;

        if (frame == nullptr)
        {
            return E_FAIL;
        }
        LPD3DXMATRIX p_matrix = &frame->m_combinedMatrix;
        skinned_mesh_container->m_frameCombinedMatrix.at(i) = p_matrix;
    }
    return S_OK;
}

HRESULT SkinAnimMesh::allocate_all_bone_matrices(LPD3DXFRAME frame)
{
    if (frame->pMeshContainer != NULL)
    {
        // TODO why frame->pMeshContainer->pNextMeshContainer is unnecessary?
        if (FAILED(allocate_bone_matrix(frame->pMeshContainer)))
        {
            return E_FAIL;
        }
    }

    if (frame->pFrameSibling != NULL)
    {
        if (FAILED(allocate_all_bone_matrices(frame->pFrameSibling)))
        {
            return E_FAIL;
        }
    }

    if (frame->pFrameFirstChild != NULL)
    {
        if (FAILED(allocate_all_bone_matrices(frame->pFrameFirstChild)))
        {
            return E_FAIL;
        }
    }
    return S_OK;
}

void SkinAnimMesh::OnDeviceLost()
{
    HRESULT hr = m_D3DEffect->OnLostDevice();
    assert(hr == S_OK);
}

void SkinAnimMesh::OnDeviceReset()
{
    HRESULT hr = m_D3DEffect->OnResetDevice();
    assert(hr == S_OK);
}

}

