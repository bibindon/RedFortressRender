#include "MeshMixSkinAnim.h"

#include <algorithm>
#include <exception>

#include "Camera.h"
#include "Common.h"
#include "Light.h"
#include "Util.h"

namespace NSRender
{
namespace
{
float ClampSpecularEdge(const float edge)
{
    return (std::max)(0.0f, (std::min)(edge, 1.0f));
}

float ConvertSpecularEdgeToShaderPower(const float edge)
{
    return 1.0f + (ClampSpecularEdge(edge) * 127.0f);
}

float ConvertXMaterialPowerToShaderPower(const float materialPower)
{
    const float clampedPower = (std::max)(0.0f, (std::min)(materialPower, 255.0f));
    return clampedPower;
}

float ConvertXMaterialPowerToSpecularIntensity(const float materialPower)
{
    const float clampedPower = (std::max)(0.0f, (std::min)(materialPower, 255.0f));
    return (clampedPower / 255.0f) * 2.0f;
}

float GetMaterialSpecularIntensity(const D3DMATERIAL9& material)
{
    if (true)
    {
        return ConvertXMaterialPowerToSpecularIntensity(material.Power);
    }
    else
    {
        return (std::max)(material.Specular.r,
               (std::max)(material.Specular.g, material.Specular.b));
    }
}
}

MeshMixSkinAnim::MeshMixSkinAnim(const std::wstring& filename,
                                 const D3DXVECTOR3& pos,
                                 const D3DXVECTOR3& rotate,
                                 const float scale,
                                 const stMeshParam& param,
                                 const AnimSetMap& animSetMap)
    : m_meshName(filename)
    , m_allocator(filename)
    , m_pos(pos)
    , m_rotate(rotate)
    , m_scale(scale)
    , m_param(param)
    , m_animSetMap(animSetMap)
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
                                    D3DXMESH_MANAGED | D3DXMESH_32BIT,
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

    m_animController.Init(tempAnimController, m_animSetMap);
    AllocateAllBoneMatrix(m_frameRoot);

    Common::AddDeviceLostResource(this);
    m_bLoaded = true;
}

void MeshMixSkinAnim::Render()
{
    if (!m_bLoaded || !m_enabled)
    {
        return;
    }

    const D3DXVECTOR4 lightDir = Light::GetLightDir();
    const D3DXVECTOR4 lightColor = D3DXVECTOR4(Light::GetLightColor());
    const D3DXVECTOR4 ambientColor = D3DXVECTOR4(Light::GetAmbientColor());
    m_D3DEffect->SetVector("g_lightDir", &lightDir);
    m_D3DEffect->SetVector("g_lightColor", &lightColor);
    m_D3DEffect->SetVector("g_ambient", &ambientColor);
    m_D3DEffect->SetFloat("g_fSunLightIntensity", Light::GetBrightness());
    m_D3DEffect->SetFloat("g_fAmbientIntensity", Light::GetAmbientBrightness());

    const D3DXVECTOR4 cameraPos = D3DXVECTOR4(Camera::GetEyePos(), 1.0f);
    m_D3DEffect->SetVector("g_cameraPos", &cameraPos);
    BOOL useSaturateShadow = FALSE;
    if (m_param.saturateShadow)
    {
        useSaturateShadow = TRUE;
    }
    m_D3DEffect->SetBool("g_bSaturateShadow", useSaturateShadow);
    m_D3DEffect->SetFloat("g_fSaturateShadowIntensity", m_param.saturateShadowIntensity);
    m_D3DEffect->SetFloat("g_fShadowDarkness", m_param.shadowDarkness);
    m_D3DEffect->SetFloat("g_specularIntensity", m_param.specularIntensity);

    D3DXMATRIX viewProjectionMatrix = Camera::GetViewMatrix() * Camera::GetProjMatrix();
    m_D3DEffect->SetMatrix("g_matViewProj", &viewProjectionMatrix);
    m_D3DEffect->SetTechnique("Technique1");

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

void MeshMixSkinAnim::RenderToEffect(LPD3DXEFFECT effect)
{
    if (!m_bLoaded || !m_enabled || effect == nullptr)
    {
        return;
    }

    RenderFrameToEffect(m_frameRoot, effect);
}

void MeshMixSkinAnim::RenderFrameToEffect(const LPD3DXFRAME frame, LPD3DXEFFECT effect)
{
    LPD3DXMESHCONTAINER container = frame->pMeshContainer;
    while (container != nullptr)
    {
        RenderMeshContainerToEffect(container, effect);
        container = container->pNextMeshContainer;
    }

    if (frame->pFrameSibling != nullptr)
    {
        RenderFrameToEffect(frame->pFrameSibling, effect);
    }

    if (frame->pFrameFirstChild != nullptr)
    {
        RenderFrameToEffect(frame->pFrameFirstChild, effect);
    }
}

void MeshMixSkinAnim::RenderMeshContainerToEffect(const LPD3DXMESHCONTAINER containerBase, LPD3DXEFFECT effect)
{
    auto container = reinterpret_cast<SkinAnimMeshContainer*>(containerBase);
    auto boneCombination = reinterpret_cast<LPD3DXBONECOMBINATION>(container->m_boneBuffer->GetBufferPointer());
    const DWORD paletteSize = container->m_paletteSize;

    effect->SetInt("g_currentBoneIndex", container->m_influenceCount - 1);

    effect->Begin(nullptr, 0);
    effect->BeginPass(0);

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

        effect->SetMatrixArray("g_matWorldArray", &m_matWorldArray[0], paletteSize);
        effect->CommitChanges();
        container->MeshData.pMesh->DrawSubset(i);
    }

    effect->EndPass();
    effect->End();
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

        const DWORD materialIndex = boneCombination[i].AttribId;
        const D3DMATERIAL9& material = container->pMaterials[materialIndex].MatD3D;
        D3DXVECTOR4 diffuse(material.Diffuse.r,
                            material.Diffuse.g,
                            material.Diffuse.b,
                            material.Diffuse.a);
        m_D3DEffect->SetVector("g_diffuse", &diffuse);

        float specularIntensity = GetMaterialSpecularIntensity(material);
        if (m_param.specularIntensityOverrideEnabled)
        {
            specularIntensity = m_param.specularIntensity;
        }
        m_D3DEffect->SetFloat("g_specularIntensity", specularIntensity);

        float specularPower = ConvertXMaterialPowerToShaderPower(material.Power);
        if (m_param.specularEdgeOverrideEnabled)
        {
            specularPower = ConvertSpecularEdgeToShaderPower(m_param.specularEdge);
        }
        m_D3DEffect->SetFloat("g_specularPower", specularPower);

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
    SkinAnimMeshFrame *frame = nullptr;

    auto container = reinterpret_cast<SkinAnimMeshContainer*>(containerBase);

    DWORD boneCount = container->pSkinInfo->GetNumBones();
    container->m_frameCombinedMatrix.resize(boneCount);

    DWORD MAX_MATRICES = 8;
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

        frame = reinterpret_cast<SkinAnimMeshFrame*>(p);
        if (frame == nullptr)
        {
            return E_FAIL;
        }

        LPD3DXMATRIX pMat = &frame->m_combinedMatrix;
        container->m_frameCombinedMatrix.at(i) = pMat;
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

void MeshMixSkinAnim::SetSpecularIntensityOverrideEnabled(const bool enabled)
{
    m_param.specularIntensityOverrideEnabled = enabled;
}

void MeshMixSkinAnim::SetSpecularEdgeOverrideEnabled(const bool enabled)
{
    m_param.specularEdgeOverrideEnabled = enabled;
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
