#include "MeshMix2.h"

#include "Camera.h"
#include "CustomXLoader.h"
#include "Light.h"
#include "Util.h"

#include <algorithm>
#include <cmath>
#include <exception>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace NSRender
{
namespace
{

float GetMaterialSpecularIntensity(const D3DMATERIAL9& material)
{
    return (std::max)(material.Specular.r,
           (std::max)(material.Specular.g, material.Specular.b));
}

float ClampSpecularPower(const float power)
{
    return (std::max)(1.0f, (std::min)(power, 128.0f));
}

std::wstring ResolveRuntimePath(const std::wstring& path)
{
    if (PathIsRelative(path.c_str()))
    {
        return Util::GetExeDir() + path;
    }

    return path;
}

}

MeshMix2::MeshMix2(const std::wstring& filename,
                   const D3DXVECTOR3& pos,
                   const D3DXVECTOR3& rotate,
                   const float scale,
                   const MeshMix2Param& param)
    : m_meshName(filename)
    , m_allocator(filename)
    , m_pos(pos)
    , m_rotate(rotate)
    , m_scale(scale)
    , m_param(param)
{
    D3DXMatrixIdentity(&m_matrixOverride);
}

MeshMix2::~MeshMix2()
{
    Finalize();
}

void MeshMix2::Initialize(const bool async)
{
    if (m_loaded)
    {
        return;
    }

    const std::wstring shaderPath = ResolveRuntimePath(kShaderFilename);
    const HRESULT effectResult = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                                           shaderPath.c_str(),
                                                           nullptr,
                                                           nullptr,
                                                           0,
                                                           nullptr,
                                                           &m_D3DEffect,
                                                           nullptr);
    if (FAILED(effectResult) || m_D3DEffect == nullptr)
    {
        throw std::runtime_error("MeshMix2 failed to create its effect.");
    }

    if (async)
    {
        if (m_loadThread.joinable())
        {
            m_loadThread.join();
        }
        m_loadThread = std::thread([this]() { InitializeInternal(); });
    }
    else
    {
        InitializeInternal();
    }
}

void MeshMix2::InitializeInternal()
{
    const std::wstring meshPath = ResolveRuntimePath(m_meshName);
    std::ifstream file(meshPath, std::ios::binary);
    if (!file)
    {
        throw std::runtime_error("MeshMix2 failed to open a Blender 5.1.2 DirectX X file.");
    }

    const std::string fileText((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
    const HRESULT loadResult = LoadCustomXFrameHierarchyFromText(fileText,
                                                                 &m_allocator,
                                                                 &m_frameRoot,
                                                                 nullptr,
                                                                 CustomXLoadPurpose::MeshAndAnimation);
    if (FAILED(loadResult) || m_frameRoot == nullptr)
    {
        throw std::runtime_error("MeshMix2 failed to load a Blender 5.1.2 DirectX X hierarchy.");
    }

    const D3DXMATRIX worldMatrix = BuildWorldMatrix();
    UpdateFrameMatrices(m_frameRoot, &worldMatrix);

    float maxDistanceSquared = 0.0f;
    CalculateRadius(m_frameRoot, maxDistanceSquared);
    m_radius = std::sqrt(maxDistanceSquared);

    Common::AddDeviceLostResource(this);
    m_deviceResourceRegistered = true;
    m_loaded = true;
}

void MeshMix2::WaitForLoad()
{
    if (m_loadThread.joinable())
    {
        m_loadThread.join();
    }
}

void MeshMix2::Finalize()
{
    if (m_loadThread.joinable())
    {
        m_loadThread.join();
    }

    ReleaseOwnedResources();
}

void MeshMix2::ReleaseOwnedResources()
{
    m_loaded = false;

    if (m_deviceResourceRegistered)
    {
        Common::RemoveDeviceLostResource(this);
        m_deviceResourceRegistered = false;
    }

    if (m_frameRoot != nullptr)
    {
        D3DXFrameDestroy(m_frameRoot, &m_allocator);
        m_frameRoot = nullptr;
    }

    SAFE_RELEASE(m_D3DEffect);
}

void MeshMix2::Render()
{
    if (!m_loaded || !m_enabled || m_D3DEffect == nullptr)
    {
        return;
    }

    const D3DXVECTOR4 lightDirection = Light::GetLightDir();
    const D3DXVECTOR4 lightColor(Light::GetLightColor());
    const D3DXVECTOR4 cameraPosition(Camera::GetEyePos(), 1.0f);
    m_D3DEffect->SetVector("g_lightDir", &lightDirection);
    m_D3DEffect->SetVector("g_lightColor", &lightColor);
    m_D3DEffect->SetVector("g_cameraPos", &cameraPosition);
    m_D3DEffect->SetFloat("g_fSunLightIntensity", Light::GetBrightness());
    m_D3DEffect->SetFloat("g_fAmbientIntensity", Light::GetAmbientBrightness());
    m_D3DEffect->SetBool("g_damageFlash", m_damageFlash);
    m_D3DEffect->SetTechnique("TechniqueNoSkin");

    const D3DXMATRIX viewProjectionMatrix = Camera::GetViewMatrix() * Camera::GetProjMatrix();
    RenderFrameHierarchy(m_frameRoot, m_D3DEffect, viewProjectionMatrix, true);
}

void MeshMix2::RenderToEffect(LPD3DXEFFECT effect)
{
    const D3DXMATRIX viewProjectionMatrix = Camera::GetViewMatrix() * Camera::GetProjMatrix();
    RenderToEffect(effect, viewProjectionMatrix);
}

void MeshMix2::RenderToEffect(LPD3DXEFFECT effect, const D3DXMATRIX& viewProjectionMatrix)
{
    if (!m_loaded || !m_enabled || effect == nullptr)
    {
        return;
    }

    RenderFrameHierarchy(m_frameRoot, effect, viewProjectionMatrix, false);
}

void MeshMix2::RenderFrameHierarchy(LPD3DXFRAME frame,
                                    LPD3DXEFFECT effect,
                                    const D3DXMATRIX& viewProjectionMatrix,
                                    const bool configureMaterial)
{
    if (frame == nullptr)
    {
        return;
    }

    auto meshMix2Frame = reinterpret_cast<MeshMix2Frame*>(frame);
    LPD3DXMESHCONTAINER containerBase = frame->pMeshContainer;
    while (containerBase != nullptr)
    {
        auto container = reinterpret_cast<MeshMix2MeshContainer*>(containerBase);
        RenderMeshContainer(*meshMix2Frame,
                            *container,
                            effect,
                            viewProjectionMatrix,
                            configureMaterial);
        containerBase = containerBase->pNextMeshContainer;
    }

    RenderFrameHierarchy(frame->pFrameSibling, effect, viewProjectionMatrix, configureMaterial);
    RenderFrameHierarchy(frame->pFrameFirstChild, effect, viewProjectionMatrix, configureMaterial);
}

void MeshMix2::RenderMeshContainer(const MeshMix2Frame& frame,
                                   MeshMix2MeshContainer& container,
                                   LPD3DXEFFECT effect,
                                   const D3DXMATRIX& viewProjectionMatrix,
                                   const bool configureMaterial)
{
    LPD3DXMESH mesh = container.MeshData.pMesh;
    if (mesh == nullptr)
    {
        throw std::runtime_error("MeshMix2 encountered a null mesh container.");
    }

    const D3DXMATRIX worldViewProjection = frame.m_combinedMatrix * viewProjectionMatrix;
    effect->SetMatrix("g_matWorld", &frame.m_combinedMatrix);
    effect->SetMatrix("g_matWorldViewProj", &worldViewProjection);
    const D3DXHANDLE viewProjectionHandle = effect->GetParameterByName(nullptr, "g_matViewProj");
    if (viewProjectionHandle != nullptr)
    {
        effect->SetMatrix(viewProjectionHandle, &viewProjectionMatrix);
    }

    DWORD subsetCount = container.NumMaterials;
    if (subsetCount == 0)
    {
        subsetCount = 1;
    }

    for (DWORD subsetIndex = 0; subsetIndex < subsetCount; ++subsetIndex)
    {
        if (configureMaterial)
        {
            D3DXVECTOR4 diffuse(1.0f, 1.0f, 1.0f, 1.0f);
            float specularIntensity = 0.0f;
            float specularPower = 1.0f;
            if (subsetIndex < container.NumMaterials)
            {
                const D3DMATERIAL9& material = container.pMaterials[subsetIndex].MatD3D;
                diffuse = D3DXVECTOR4(material.Diffuse.r,
                                      material.Diffuse.g,
                                      material.Diffuse.b,
                                      material.Diffuse.a);
                specularIntensity = GetMaterialSpecularIntensity(material);
                specularPower = ClampSpecularPower(material.Power);
            }

            if (m_param.specularIntensityOverrideEnabled)
            {
                specularIntensity = m_param.specularIntensity;
            }
            if (m_param.specularEdgeOverrideEnabled)
            {
                specularPower = 1.0f + (m_param.specularEdge * 127.0f);
            }

            bool hasTexture = false;
            if (subsetIndex < container.m_textureList.size() &&
                container.m_textureList[subsetIndex] != nullptr)
            {
                effect->SetTexture("g_textureSampler", container.m_textureList[subsetIndex]);
                hasTexture = true;
            }
            else
            {
                effect->SetTexture("g_textureSampler", nullptr);
            }

            BOOL treatTextureAsWhite = FALSE;
            if (m_param.treatTextureAsWhite || !hasTexture)
            {
                treatTextureAsWhite = TRUE;
            }
            effect->SetBool("g_treatTextureAsWhite", treatTextureAsWhite);
            effect->SetVector("g_diffuse", &diffuse);
            effect->SetFloat("g_specularIntensity", specularIntensity);
            effect->SetFloat("g_specularPower", specularPower);
        }

        effect->CommitChanges();
        UINT passCount = 0;
        const HRESULT beginResult = effect->Begin(&passCount, 0);
        if (FAILED(beginResult))
        {
            throw std::runtime_error("MeshMix2 failed to begin an effect.");
        }
        for (UINT passIndex = 0; passIndex < passCount; ++passIndex)
        {
            if (FAILED(effect->BeginPass(passIndex)))
            {
                effect->End();
                throw std::runtime_error("MeshMix2 failed to begin an effect pass.");
            }
            if (FAILED(mesh->DrawSubset(subsetIndex)))
            {
                effect->EndPass();
                effect->End();
                throw std::runtime_error("MeshMix2 failed to draw a mesh subset.");
            }
            effect->EndPass();
        }
        effect->End();
    }
}

void MeshMix2::UpdateFrameMatrices(LPD3DXFRAME frame, const D3DXMATRIX* parentMatrix)
{
    if (frame == nullptr)
    {
        return;
    }

    auto meshMix2Frame = reinterpret_cast<MeshMix2Frame*>(frame);
    if (parentMatrix != nullptr)
    {
        meshMix2Frame->m_combinedMatrix = frame->TransformationMatrix * (*parentMatrix);
    }
    else
    {
        meshMix2Frame->m_combinedMatrix = frame->TransformationMatrix;
    }

    UpdateFrameMatrices(frame->pFrameSibling, parentMatrix);
    UpdateFrameMatrices(frame->pFrameFirstChild, &meshMix2Frame->m_combinedMatrix);
}

void MeshMix2::CalculateRadius(LPD3DXFRAME frame, float& maxDistanceSquared) const
{
    if (frame == nullptr)
    {
        return;
    }

    const auto meshMix2Frame = reinterpret_cast<const MeshMix2Frame*>(frame);
    LPD3DXMESHCONTAINER containerBase = frame->pMeshContainer;
    while (containerBase != nullptr)
    {
        const auto container = reinterpret_cast<const MeshMix2MeshContainer*>(containerBase);
        LPD3DXMESH mesh = container->MeshData.pMesh;
        if (mesh != nullptr)
        {
            const DWORD vertexCount = mesh->GetNumVertices();
            const DWORD vertexStride = D3DXGetFVFVertexSize(mesh->GetFVF());
            const BYTE* vertexData = nullptr;
            if (SUCCEEDED(mesh->LockVertexBuffer(D3DLOCK_READONLY,
                                                reinterpret_cast<void**>(const_cast<BYTE**>(&vertexData)))))
            {
                for (DWORD vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
                {
                    const auto position = reinterpret_cast<const D3DXVECTOR3*>(vertexData + (vertexIndex * vertexStride));
                    D3DXVECTOR3 transformedPosition;
                    D3DXVec3TransformCoord(&transformedPosition,
                                           position,
                                           &meshMix2Frame->m_combinedMatrix);
                    const float distanceSquared = D3DXVec3LengthSq(&transformedPosition);
                    maxDistanceSquared = (std::max)(maxDistanceSquared, distanceSquared);
                }
                mesh->UnlockVertexBuffer();
            }
        }
        containerBase = containerBase->pNextMeshContainer;
    }

    CalculateRadius(frame->pFrameSibling, maxDistanceSquared);
    CalculateRadius(frame->pFrameFirstChild, maxDistanceSquared);
}

D3DXMATRIX MeshMix2::BuildWorldMatrix() const
{
    if (m_useMatrixOverride)
    {
        return m_matrixOverride;
    }

    D3DXMATRIX worldMatrix;
    D3DXMATRIX workMatrix;
    D3DXMatrixScaling(&worldMatrix, m_scale, m_scale, m_scale);
    D3DXMatrixRotationYawPitchRoll(&workMatrix, m_rotate.y, m_rotate.x, m_rotate.z);
    worldMatrix *= workMatrix;
    D3DXMatrixTranslation(&workMatrix, m_pos.x, m_pos.y, m_pos.z);
    worldMatrix *= workMatrix;
    return worldMatrix;
}

void MeshMix2::SetPos(const D3DXVECTOR3& pos)
{
    m_pos = pos;
    m_useMatrixOverride = false;
    if (m_frameRoot != nullptr)
    {
        const D3DXMATRIX worldMatrix = BuildWorldMatrix();
        UpdateFrameMatrices(m_frameRoot, &worldMatrix);
    }
}

void MeshMix2::SetRotY(const float rotY)
{
    m_rotate.y = rotY;
    m_useMatrixOverride = false;
    if (m_frameRoot != nullptr)
    {
        const D3DXMATRIX worldMatrix = BuildWorldMatrix();
        UpdateFrameMatrices(m_frameRoot, &worldMatrix);
    }
}

void MeshMix2::SetWorldMatrix(const D3DXMATRIX& matrix)
{
    m_matrixOverride = matrix;
    m_useMatrixOverride = true;
    if (m_frameRoot != nullptr)
    {
        UpdateFrameMatrices(m_frameRoot, &m_matrixOverride);
    }
}

void MeshMix2::SetEnabled(const bool enabled) { m_enabled = enabled; }
void MeshMix2::SetDamageFlash(const bool enabled) { m_damageFlash = enabled; }
void MeshMix2::SetSaturateShadow(const bool enabled) { m_param.saturateShadow = enabled; }
void MeshMix2::SetSaturateShadowIntensity(const float intensity) { m_param.saturateShadowIntensity = intensity; }
void MeshMix2::SetShadowDarkness(const float darkness) { m_param.shadowDarkness = darkness; }
void MeshMix2::SetSpecularIntensity(const float intensity) { m_param.specularIntensity = intensity; }
void MeshMix2::SetSpecularEdge(const float edge) { m_param.specularEdge = edge; }
void MeshMix2::SetSpecularIntensityOverrideEnabled(const bool enabled) { m_param.specularIntensityOverrideEnabled = enabled; }
void MeshMix2::SetSpecularEdgeOverrideEnabled(const bool enabled) { m_param.specularEdgeOverrideEnabled = enabled; }
void MeshMix2::SetTreatTextureAsWhite(const bool enabled) { m_param.treatTextureAsWhite = enabled; }
D3DXVECTOR3 MeshMix2::GetPos() const { return m_pos; }
D3DXVECTOR3 MeshMix2::GetRot() const { return m_rotate; }
D3DXMATRIX MeshMix2::GetWorldMatrix() const { return BuildWorldMatrix(); }
float MeshMix2::GetScale() const { return m_scale; }
float MeshMix2::GetRadius() const { return m_radius; }
bool MeshMix2::IsEnabled() const { return m_enabled; }
bool MeshMix2::IsLoaded() const { return m_loaded; }
bool MeshMix2::IsSsaoEnabled() const { return m_param.ssao; }
bool MeshMix2::IsDepthBufferShadowEnabled() const { return m_param.shadow; }
std::wstring MeshMix2::GetMeshName() const { return m_meshName; }

void MeshMix2::OnDeviceLost()
{
    if (m_D3DEffect != nullptr)
    {
        m_D3DEffect->OnLostDevice();
    }
}

void MeshMix2::OnDeviceReset()
{
    if (m_D3DEffect != nullptr)
    {
        m_D3DEffect->OnResetDevice();
    }
}

}
