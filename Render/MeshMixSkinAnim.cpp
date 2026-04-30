#include "MeshMixSkinAnim.h"

#include <exception>

#include "Camera.h"
#include "Common.h"
#include "Light.h"
#include "Util.h"

namespace NSRender
{

MeshMixSkinAnim::MeshMixSkinAnim(const std::wstring& filename,
                                 const D3DXVECTOR3& pos,
                                 const D3DXVECTOR3& rotate,
                                 const float scale,
                                 const stMeshParam& param)
    : m_meshName(filename)
    , m_allocator(filename)
    , m_pos(pos)
    , m_rotate(rotate)
    , m_scale(scale)
    , m_param(param)
{
}

MeshMixSkinAnim::~MeshMixSkinAnim()
{
    SAFE_RELEASE(m_D3DEffect);
    m_animController.Finalize();

    if (m_frameRoot != nullptr)
    {
        ReleaseMeshAllocator(m_frameRoot);
        m_frameRoot = nullptr;
    }
}

void MeshMixSkinAnim::Initialize()
{
    HRESULT hr = E_FAIL;
    std::wstring tempPath = Util::GetExeDir() + SHADER_FILENAME;

    hr = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                  tempPath.c_str(),
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

    LPD3DXANIMATIONCONTROLLER tempAnimController = nullptr;

    if (PathIsRelative(m_meshName.c_str()))
    {
        tempPath = Util::GetExeDir() + m_meshName;
    }
    else
    {
        tempPath = m_meshName;
    }

    hr = D3DXLoadMeshHierarchyFromX(tempPath.c_str(),
                                    D3DXMESH_MANAGED,
                                    Common::D3DDevice(),
                                    &m_allocator,
                                    nullptr,
                                    &m_frameRoot,
                                    &tempAnimController);
    if (FAILED(hr) || tempAnimController == nullptr)
    {
        SAFE_RELEASE(tempAnimController);
        throw std::exception("Failed to load skin animation mesh.");
    }

    BuildDefaultAnimSetMap(tempAnimController);
    AllocateAllBoneMatrix(m_frameRoot);

    Common::AddDeviceLostResource(this);
    m_bLoaded = true;
}

void MeshMixSkinAnim::BuildDefaultAnimSetMap(const LPD3DXANIMATIONCONTROLLER animationController)
{
    AnimSetMap animSetMap;

    if (animationController != nullptr && animationController->GetNumAnimationSets() > 0)
    {
        LPD3DXANIMATIONSET animationSet = nullptr;
        const HRESULT hr = animationController->GetAnimationSet(0, &animationSet);
        if (SUCCEEDED(hr) && animationSet != nullptr)
        {
            const char* animationName = animationSet->GetName();

            AnimSetting animSetting;
            animSetting.m_startPos = 0.0f;
            animSetting.m_duration = static_cast<float>(animationSet->GetPeriod());
            animSetting.m_loop = true;
            animSetting.m_stopEnd = false;

            const std::wstring animName = (animationName != nullptr && animationName[0] != '\0')
                ? Util::Utf8ToWstring(animationName)
                : L"0";
            animSetMap[animName] = animSetting;
        }

        SAFE_RELEASE(animationSet);
    }

    if (animSetMap.empty())
    {
        AnimSetting animSetting;
        animSetting.m_startPos = 0.0f;
        animSetting.m_duration = 1.0f;
        animSetting.m_loop = true;
        animSetting.m_stopEnd = false;
        animSetMap[L"0"] = animSetting;
    }

    m_animController.Init(animationController, animSetMap);
}

void MeshMixSkinAnim::Render()
{
    if (!m_bLoaded || !m_enabled)
    {
        return;
    }

    const D3DXVECTOR4 lightDir = Light::GetLightDir();
    m_D3DEffect->SetVector("g_lightNormal", &lightDir);
    m_D3DEffect->SetFloat("g_lightBrightness", Light::GetBrightness());

    D3DXMATRIX viewProjectionMatrix = Camera::GetViewMatrix() * Camera::GetProjMatrix();
    m_D3DEffect->SetMatrix("g_matViewProj", &viewProjectionMatrix);

    m_animController.Update();

    D3DXMATRIX worldMatrix;
    D3DXMatrixIdentity(&worldMatrix);
    {
        D3DXMATRIX mat;
        D3DXMatrixTranslation(&mat, -m_centerPos.x, -m_centerPos.y, -m_centerPos.z);
        worldMatrix *= mat;

        D3DXMatrixScaling(&mat, m_scale, m_scale, m_scale);
        worldMatrix *= mat;

        D3DXMatrixRotationYawPitchRoll(&mat, m_rotate.y, m_rotate.x, m_rotate.z);
        worldMatrix *= mat;

        D3DXMatrixTranslation(&mat, m_pos.x, m_pos.y, m_pos.z);
        worldMatrix *= mat;
    }

    UpdateFrameMatrix(m_frameRoot, &worldMatrix);
    RenderFrame(m_frameRoot);
}

void MeshMixSkinAnim::UpdateFrameMatrix(const LPD3DXFRAME frameBase, const LPD3DXMATRIX matParent)
{
    auto frame = reinterpret_cast<SkinAnimMeshFrame*>(frameBase);

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

void MeshMixSkinAnim::RenderFrame(const LPD3DXFRAME frame)
{
    LPD3DXMESHCONTAINER container = frame->pMeshContainer;
    while (container != nullptr)
    {
        RenderMeshContainer(container);
        container = container->pNextMeshContainer;
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

void MeshMixSkinAnim::RenderMeshContainer(const LPD3DXMESHCONTAINER containerBase)
{
    auto container = reinterpret_cast<SkinAnimMeshContainer*>(containerBase);
    auto boneCombination = reinterpret_cast<LPD3DXBONECOMBINATION>(container->m_boneBuffer->GetBufferPointer());
    const DWORD paletteSize = container->m_paletteSize;

    for (DWORD i = 0; i < container->m_boneCount; ++i)
    {
        for (DWORD k = 0; k < paletteSize; ++k)
        {
            const DWORD boneId = boneCombination[i].BoneId[k];
            if (boneId == UINT_MAX)
            {
                continue;
            }

            m_matWorldArray[k] = container->m_boneOffsetMatrices[boneId] *
                                 (*container->m_frameCombinedMatrix[boneId]);
        }

        m_D3DEffect->SetMatrixArray("g_matWorldArray", &m_matWorldArray[0], paletteSize);
        m_D3DEffect->SetInt("g_currentBoneIndex", container->m_influenceCount - 1);

        const DWORD materialIndex = boneCombination[i].AttribId;
        const D3DMATERIAL9& material = container->pMaterials[materialIndex].MatD3D;
        D3DXVECTOR4 diffuse(material.Diffuse.r,
                            material.Diffuse.g,
                            material.Diffuse.b,
                            material.Diffuse.a);
        m_D3DEffect->SetVector("g_diffuse", &diffuse);

        if (materialIndex < container->m_textureList.size())
        {
            m_D3DEffect->SetTexture("g_texture", container->m_textureList[materialIndex]);
        }
        else
        {
            m_D3DEffect->SetTexture("g_texture", nullptr);
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

HRESULT MeshMixSkinAnim::AllocateBoneMatrix(LPD3DXMESHCONTAINER containerBase)
{
    auto container = reinterpret_cast<SkinAnimMeshContainer*>(containerBase);
    const DWORD boneCount = container->pSkinInfo->GetNumBones();
    container->m_frameCombinedMatrix.resize(boneCount);

    const DWORD maxMatrices = 8;
    m_matWorldArray.resize((boneCount > maxMatrices) ? maxMatrices : boneCount);

    for (DWORD i = 0; i < boneCount; ++i)
    {
        auto frame = reinterpret_cast<SkinAnimMeshFrame*>(D3DXFrameFind(m_frameRoot,
                                                                        container->pSkinInfo->GetBoneName(i)));
        if (frame == nullptr)
        {
            return E_FAIL;
        }

        container->m_frameCombinedMatrix[i] = &frame->m_combinedMatrix;
    }

    return S_OK;
}

HRESULT MeshMixSkinAnim::AllocateAllBoneMatrix(LPD3DXFRAME frame)
{
    if (frame->pMeshContainer != nullptr && FAILED(AllocateBoneMatrix(frame->pMeshContainer)))
    {
        return E_FAIL;
    }

    if (frame->pFrameSibling != nullptr && FAILED(AllocateAllBoneMatrix(frame->pFrameSibling)))
    {
        return E_FAIL;
    }

    if (frame->pFrameFirstChild != nullptr && FAILED(AllocateAllBoneMatrix(frame->pFrameFirstChild)))
    {
        return E_FAIL;
    }

    return S_OK;
}

void MeshMixSkinAnim::ReleaseMeshAllocator(const LPD3DXFRAME frame)
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

void MeshMixSkinAnim::SetPos(const D3DXVECTOR3& pos)
{
    m_pos = pos;
}

void MeshMixSkinAnim::SetSaturateShadow(const bool enabled)
{
    m_param.saturateShadow = enabled;
}

void MeshMixSkinAnim::SetSaturateShadowIntensity(const float intensity)
{
    m_param.saturateShadowIntensity = intensity;
}

void MeshMixSkinAnim::SetShadowDarkness(const float darkness)
{
    m_param.shadowDarkness = darkness;
}

void MeshMixSkinAnim::SetSpecularIntensity(const float intensity)
{
    m_param.specularIntensity = intensity;
}

void MeshMixSkinAnim::SetSpecularEdge(const float edge)
{
    m_param.specularEdge = edge;
}

void MeshMixSkinAnim::SetRotY(const float rotY)
{
    m_rotate.y = rotY;
}

D3DXVECTOR3 MeshMixSkinAnim::GetRot() const
{
    return m_rotate;
}

D3DXVECTOR3 MeshMixSkinAnim::GetPos() const
{
    return m_pos;
}

float MeshMixSkinAnim::GetScale() const
{
    return m_scale;
}

bool MeshMixSkinAnim::IsEnabled() const
{
    return m_enabled;
}

void MeshMixSkinAnim::SetEnabled(const bool enabled)
{
    m_enabled = enabled;
}

std::wstring MeshMixSkinAnim::GetMeshName() const
{
    return m_meshName;
}

void MeshMixSkinAnim::OnDeviceLost()
{
    const HRESULT hr = m_D3DEffect->OnLostDevice();
    assert(hr == S_OK);
}

void MeshMixSkinAnim::OnDeviceReset()
{
    const HRESULT hr = m_D3DEffect->OnResetDevice();
    assert(hr == S_OK);
}

}
