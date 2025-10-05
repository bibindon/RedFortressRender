
#include "SkinAnimMesh.h"

#include <exception>

#include "Common.h"
#include "Util.h"
#include "SkinAnimMeshAlloc.h"

namespace NSRender
{

const std::wstring SkinAnimMesh::SHADER_FILENAME = L"res\\shader\\SkinAnimMeshShader.fx";

SkinAnimMesh::SkinAnimMesh(const std::wstring &x_filename,
                           const D3DXVECTOR3 &position,
                           const D3DXVECTOR3 &rotation,
                           const float &scale,
                           const AnimSetMap& animSetMap)
    : m_allocator(x_filename),
      m_matRotate(),
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

    LPD3DXANIMATIONCONTROLLER tempAnimController = NULL;

    hr = D3DXLoadMeshHierarchyFromX(x_filename.c_str(),
                                    D3DXMESH_MANAGED,
                                    Common::D3DDevice(),
                                    &m_allocator,
                                    nullptr,
                                    &m_frameRoot,
                                    &tempAnimController);

    assert(hr == S_OK);
    assert(tempAnimController != NULL);

    m_animController.Init(tempAnimController, animSetMap);

    AllocateAllBoneMatrix(m_frameRoot);
}

SkinAnimMesh::~SkinAnimMesh()
{
    SAFE_RELEASE(m_D3DEffect);

    m_animController.Finalize();
    ReleaseMeshAllocator(m_frameRoot);
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

    m_animController.Update();

    D3DXMATRIX world_matrix;
    D3DXMatrixIdentity(&world_matrix);
    {
        D3DXMATRIX mat;
        D3DXMatrixTranslation(&mat, -m_centerPos.x, -m_centerPos.y, -m_centerPos.z);
        world_matrix *= mat;

        D3DXMatrixScaling(&mat, m_scale, m_scale, m_scale);
        world_matrix *= mat;

        D3DXMatrixRotationYawPitchRoll(&mat, m_rotate.y, m_rotate.x, m_rotate.z);
        world_matrix *= mat;

        D3DXMatrixTranslation(&mat, m_position.x, m_position.y, m_position.z);
        world_matrix *= mat;
    }

    UpdateFrameMatrix(m_frameRoot, &world_matrix);
    RenderFrame(m_frameRoot);
}

void SkinAnimMesh::UpdateFrameMatrix(const LPD3DXFRAME frameBase,
                                       const LPD3DXMATRIX matParent)
{
    auto frame = (SkinAnimMeshFrame*)frameBase;
    
    if (matParent != nullptr)
    {
        frame->m_combinedMatrix = frame->TransformationMatrix * (*matParent);
    }
    else
    {
        frame->m_combinedMatrix = frame->TransformationMatrix;
    }

    if (frame->pFrameSibling != nullptr)
    {
        UpdateFrameMatrix(frame->pFrameSibling, matParent);
    }

    if (frame->pFrameFirstChild != nullptr)
    {
        UpdateFrameMatrix(frame->pFrameFirstChild, &frame->m_combinedMatrix);
    }
}

void SkinAnimMesh::RenderFrame(const LPD3DXFRAME frame)
{
    {
        LPD3DXMESHCONTAINER container = frame->pMeshContainer;

        while (container != nullptr)
        {
            RenderMeshContainer(container);
            container = container->pNextMeshContainer;
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

void SkinAnimMesh::RenderMeshContainer(const LPD3DXMESHCONTAINER containerBase)
{
    auto container = (SkinAnimMeshContainer*)containerBase;

    LPD3DXBONECOMBINATION bone_combination = NULL;

    bone_combination = (LPD3DXBONECOMBINATION)container->m_boneBuffer->GetBufferPointer();

    const DWORD dw_palette_size = container->m_paletteSize;

    for (DWORD i = 0; i < container->m_boneCount; ++i)
    {
        for (DWORD k = 0; k < dw_palette_size; ++k)
        {
            DWORD dw_bone_id = bone_combination[i].BoneId[k];
            if (dw_bone_id == UINT_MAX)
            {
                continue;
            }

            m_matWorldArray[k] = container->m_boneOffsetMatrices[dw_bone_id] *
                                 (*container->m_frameCombinedMatrix[dw_bone_id]);
        }
        m_D3DEffect->SetMatrixArray("g_matWorldArray",
                                    &m_matWorldArray[0],
                                    dw_palette_size);

        DWORD bone_id = bone_combination[i].AttribId;
        D3DXVECTOR4 d3dColor;

        d3dColor.x = container->pMaterials[bone_id].MatD3D.Diffuse.r;
        d3dColor.y = container->pMaterials[bone_id].MatD3D.Diffuse.g;
        d3dColor.z = container->pMaterials[bone_id].MatD3D.Diffuse.b;
        d3dColor.w = container->pMaterials[bone_id].MatD3D.Diffuse.a;

        m_D3DEffect->SetVector("g_diffuse", &d3dColor);

        if (bone_id < container->m_textureList.size())
        {
            m_D3DEffect->SetTexture("g_texture",
                                    container->m_textureList.at(bone_id));
        }

        m_D3DEffect->Begin(nullptr, 0);

        if (FAILED(m_D3DEffect->BeginPass(0)))
        {
            m_D3DEffect->End();
            throw std::exception("Failed 'BeginPass' function.");
        }
        m_D3DEffect->CommitChanges();
        container->MeshData.pMesh->DrawSubset(i);
        m_D3DEffect->EndPass();
        m_D3DEffect->End();
    }
}

HRESULT SkinAnimMesh::AllocateBoneMatrix(LPD3DXMESHCONTAINER containerBase)
{
    SkinAnimMeshFrame *frame = nullptr;

    auto container = (SkinAnimMeshContainer*)containerBase;

    DWORD boneCount = container->pSkinInfo->GetNumBones();
    container->m_frameCombinedMatrix.resize(boneCount);

    DWORD MAX_MATRICES = 26;
    if (boneCount > MAX_MATRICES)
    {
        m_matWorldArray.resize(MAX_MATRICES);
    }
    else
    {
        m_matWorldArray.resize(boneCount);
    }

    m_D3DEffect->SetInt("g_currentBoneIndex", container->m_influenceCount - 1);

    for (DWORD i = 0; i < boneCount; ++i)
    {
        LPD3DXFRAME p = D3DXFrameFind(m_frameRoot,
                                      container->pSkinInfo->GetBoneName(i));

        frame = (SkinAnimMeshFrame*)p;

        if (frame == nullptr)
        {
            return E_FAIL;
        }

        LPD3DXMATRIX pMat = &frame->m_combinedMatrix;
        container->m_frameCombinedMatrix.at(i) = pMat;
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

void SkinAnimMesh::ReleaseMeshAllocator(const LPD3DXFRAME frame)
{
    if (frame->pMeshContainer != nullptr)
    {
        m_allocator.DestroyMeshContainer(frame->pMeshContainer);
    }

    if (frame->pFrameSibling != nullptr)
    {
        ReleaseMeshAllocator(frame->pFrameSibling);
    }

    if (frame->pFrameFirstChild != nullptr)
    {
        ReleaseMeshAllocator(frame->pFrameFirstChild);
    }

    m_allocator.DestroyFrame(frame);
}

}

