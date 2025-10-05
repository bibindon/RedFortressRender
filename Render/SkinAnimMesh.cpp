
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
        m_allocator->DestroyMeshContainer(frame->pMeshContainer);
    }

    if (frame->pFrameSibling != nullptr)
    {
        release_mesh_allocator(frame->pFrameSibling);
    }

    if (frame->pFrameFirstChild != nullptr)
    {
        release_mesh_allocator(frame->pFrameFirstChild);
    }

    m_allocator->DestroyFrame(frame);
}

SkinAnimMesh::SkinAnimMesh(const std::wstring &x_filename,
                           const D3DXVECTOR3 &position,
                           const D3DXVECTOR3 &rotation,
                           const float &scale,
                           const AnimSetMap& animSetMap)
    : m_allocator{NEW SkinAnimMeshAlloc(x_filename)},
      frame_root_{nullptr, frame_root_deleter_object{m_allocator}},
      m_matRotate{D3DMATRIX{}},
      m_position(position),
      m_rotate(rotation),
      m_scale(scale)
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

    LPD3DXFRAME temp_frame_root = NULL;
    LPD3DXANIMATIONCONTROLLER temp_animation_controller = NULL;

    if (FAILED(D3DXLoadMeshHierarchyFromX(x_filename.c_str(),
                                          D3DXMESH_MANAGED,
                                          Common::D3DDevice(),
                                          m_allocator.get(),
                                          nullptr,
                                          &temp_frame_root,
                                          &temp_animation_controller)))
    {
        auto msg = L"Failed to load a x-file.: " + x_filename;
        auto msg2 = Util::WstringToUtf8(msg);
        throw std::exception(msg2.c_str());
    }

    frame_root_.reset(temp_frame_root);
    m_animCtrlr.Init(temp_animation_controller, animSetMap);

    AllocateAllBoneMatrix(frame_root_.get());

    m_scale = scale;
}

SkinAnimMesh::~SkinAnimMesh()
{
}

void SkinAnimMesh::Render(const D3DXMATRIX& view_matrix,
                          const D3DXMATRIX& projection_matrix,
                          const D3DXVECTOR4& light_normal,
                          const float& brightness)
{
    m_D3DEffect->SetVector("g_lightNormal", &light_normal);
    m_D3DEffect->SetFloat("g_lightBrightness", brightness);

    RenderImpl(view_matrix, projection_matrix);
}

void SkinAnimMesh::RenderImpl(const D3DXMATRIX &view_matrix,
                               const D3DXMATRIX &projection_matrix)
{
    D3DXMATRIX view_projection_matrix = view_matrix * projection_matrix;

    m_D3DEffect->SetMatrix("g_matViewProj", &view_projection_matrix);

    m_animCtrlr.Update();

    D3DXMATRIX world_matrix;
    D3DXMatrixIdentity(&world_matrix);
    {
        D3DXMATRIX mat;
        D3DXMatrixTranslation(
            &mat, -m_centerPos.x, -m_centerPos.y, -m_centerPos.z);
        world_matrix *= mat;

        D3DXMatrixScaling(&mat, m_scale, m_scale, m_scale);
        world_matrix *= mat;

        D3DXMatrixRotationYawPitchRoll(&mat, m_rotate.y, m_rotate.x, m_rotate.z);
        world_matrix *= mat;

        D3DXMatrixTranslation(&mat, m_position.x, m_position.y, m_position.z);
        world_matrix *= mat;
    }

    UpdateFrameMatrix(frame_root_.get(), &world_matrix);
    RenderFrame(frame_root_.get());
}

void SkinAnimMesh::UpdateFrameMatrix(const LPD3DXFRAME frame_base,
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
        UpdateFrameMatrix(frame->pFrameSibling, parent_matrix);
    }

    if (frame->pFrameFirstChild != nullptr)
    {
        UpdateFrameMatrix(frame->pFrameFirstChild, &frame->m_combinedMatrix);
    }
}

void SkinAnimMesh::RenderFrame(const LPD3DXFRAME frame)
{
    {
        LPD3DXMESHCONTAINER mesh_container{frame->pMeshContainer};
        while (mesh_container != nullptr)
        {
            RenderMeshContainer(mesh_container);
            mesh_container = mesh_container->pNextMeshContainer;
        }
    }

    if (frame->pFrameSibling != nullptr)
    {
        RenderFrame(frame->pFrameSibling);
    }

    if (frame->pFrameFirstChild != nullptr)
    {
        RenderFrame(frame->pFrameFirstChild);
    }
}

void SkinAnimMesh::RenderMeshContainer(const LPD3DXMESHCONTAINER mesh_container_base)
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
            m_matWorldArray[k] =
                mesh_container->m_boneOffsetMatrices[dw_bone_id] *
                (*mesh_container->m_frameCombinedMatrix[dw_bone_id]);
        }
        m_D3DEffect->SetMatrixArray("g_matWorldArray",
                                    &m_matWorldArray[0],
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
            m_D3DEffect->SetTexture("g_texture",
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

HRESULT SkinAnimMesh::AllocateBoneMatrix(LPD3DXMESHCONTAINER mesh_container)
{
    SkinAnimMeshFrame *frame = nullptr;

    auto skinned_mesh_container = (SkinAnimMeshContainer*)mesh_container;

    DWORD bone_count = skinned_mesh_container->pSkinInfo->GetNumBones();
    skinned_mesh_container->m_frameCombinedMatrix.resize(bone_count);

    // TODO Improve.
    DWORD MAX_MATRICES = 26;
    m_matWorldArray.resize((std::min)(MAX_MATRICES, bone_count));

    m_D3DEffect->SetInt("g_currentBoneIndex", skinned_mesh_container->m_influenceCount - 1);

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

HRESULT SkinAnimMesh::AllocateAllBoneMatrix(LPD3DXFRAME frame)
{
    if (frame->pMeshContainer != NULL)
    {
        // TODO why frame->pMeshContainer->pNextMeshContainer is unnecessary?
        if (FAILED(AllocateBoneMatrix(frame->pMeshContainer)))
        {
            return E_FAIL;
        }
    }

    if (frame->pFrameSibling != NULL)
    {
        if (FAILED(AllocateAllBoneMatrix(frame->pFrameSibling)))
        {
            return E_FAIL;
        }
    }

    if (frame->pFrameFirstChild != NULL)
    {
        if (FAILED(AllocateAllBoneMatrix(frame->pFrameFirstChild)))
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

