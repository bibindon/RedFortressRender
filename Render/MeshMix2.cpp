#include "MeshMix2.h"

#include "Camera.h"
#include "CustomXLoader.h"
#include "Light.h"
#include "Util.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
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
    return (std::max)(0.0f, (std::min)(material.Power / 500.0f, 1.0f));
}

float ConvertMaterialSpecularPower(const D3DMATERIAL9& material)
{
    return GetMaterialSpecularIntensity(material) * 128.0f;
}

std::wstring ResolveRuntimePath(const std::wstring& path)
{
    if (PathIsRelative(path.c_str()))
    {
        return Util::GetExeDir() + path;
    }

    return path;
}

void ThrowIfEffectCallFailed(const HRESULT result, const char* operation)
{
    if (FAILED(result))
    {
        const std::string message = std::string(operation) +
                                    " HRESULT=" +
                                    std::to_string(static_cast<unsigned long>(result));
        throw std::runtime_error(message);
    }
}

float PointLightShapeToShaderValue(const PointLightShape shape)
{
    return static_cast<float>(static_cast<int>(shape));
}

LPDIRECT3DTEXTURE9 g_meshMix2ThicknessTexture = nullptr;
LPDIRECT3DTEXTURE9 g_meshMix2MirrorTexture = nullptr;
D3DXMATRIX g_meshMix2MirrorViewProjection;
bool g_meshMix2MirrorClipEnabled = false;
D3DXVECTOR4 g_meshMix2MirrorClipPlane(0.0f, 1.0f, 0.0f, 0.0f);

}

void MeshMix2::SetSharedThicknessTexture(LPDIRECT3DTEXTURE9 texture)
{
    g_meshMix2ThicknessTexture = texture;
}

void MeshMix2::SetSharedMirrorTexture(LPDIRECT3DTEXTURE9 texture)
{
    g_meshMix2MirrorTexture = texture;
}

void MeshMix2::SetSharedMirrorViewProj(const D3DXMATRIX& matrix)
{
    g_meshMix2MirrorViewProjection = matrix;
}

void MeshMix2::SetSharedMirrorClipPlane(const bool enabled, const D3DXVECTOR4& plane)
{
    g_meshMix2MirrorClipEnabled = enabled;
    g_meshMix2MirrorClipPlane = plane;
}

MeshMix2::MeshMix2(const std::wstring& filename,
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
    D3DXMatrixIdentity(&m_matrixOverride);
    D3DXMatrixIdentity(&g_meshMix2MirrorViewProjection);
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
    CUSTOM_X_LOADER_LOG(L"MeshMix2 load start. Path=" + meshPath);
    std::ifstream file(meshPath, std::ios::binary);
    if (!file)
    {
        CUSTOM_X_LOADER_LOG(L"MeshMix2 failed to open file. Path=" + meshPath);
        throw std::runtime_error("MeshMix2 failed to open a Blender 5.1.2 DirectX X file.");
    }

    const std::string fileText((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
    const HRESULT loadResult = LoadCustomXFrameHierarchyFromText(fileText,
                                                                 &m_allocator,
                                                                 &m_frameRoot,
                                                                 nullptr,
                                                                 CustomXLoadPurpose::MeshAndAnimation);
    CUSTOM_X_LOADER_LOG(L"MeshMix2 load result. Path=" + meshPath +
                        L" HR=" + FormatHRESULT(loadResult) +
                        L" FrameRoot=" +
                        std::to_wstring(reinterpret_cast<std::uintptr_t>(m_frameRoot)));
    if (FAILED(loadResult) || m_frameRoot == nullptr)
    {
        throw std::runtime_error("MeshMix2 failed to load a Blender 5.1.2 DirectX X hierarchy.");
    }

    bool hasSyntheticRoot = false;
    if (m_frameRoot->Name != nullptr &&
        m_frameRoot->Name[0] == '\0' &&
        m_frameRoot->pMeshContainer == nullptr)
    {
        hasSyntheticRoot = true;
    }
    CorrectBlenderOfficialAxisTransforms(m_frameRoot, hasSyntheticRoot);

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

void MeshMix2::CorrectBlenderOfficialAxisTransforms(LPD3DXFRAME frame,
                                                     const bool skipCurrentFrame)
{
    if (frame == nullptr)
    {
        return;
    }

    if (!skipCurrentFrame)
    {
        // Blender's DirectX exporter writes vertices and Frame transforms as
        // (X, Z, -Y). Convert both to this renderer's (X, Z, Y) convention.
        // Pre-multiplication fixes the Frame basis but preserves its translation
        // row, so the exported negative Z translation must be restored separately.
        D3DXMATRIX blenderAxisConversion;
        D3DXMatrixIdentity(&blenderAxisConversion);
        blenderAxisConversion._22 = 0.0f;
        blenderAxisConversion._23 = 1.0f;
        blenderAxisConversion._32 = 1.0f;
        blenderAxisConversion._33 = 0.0f;
        frame->TransformationMatrix = blenderAxisConversion * frame->TransformationMatrix;
        frame->TransformationMatrix._43 = -frame->TransformationMatrix._43;
    }

    CorrectBlenderOfficialAxisTransforms(frame->pFrameSibling, false);
    CorrectBlenderOfficialAxisTransforms(frame->pFrameFirstChild, false);
}

void MeshMix2::Render(const bool renderAsMirrorSurface)
{
    if (!m_loaded || !m_enabled || m_D3DEffect == nullptr)
    {
        return;
    }

    if (m_param.autoHide)
    {
        const D3DXVECTOR3 cameraDistance = Camera::GetEyePos() - m_pos;
        if (D3DXVec3LengthSq(&cameraDistance) > (30.0f * 30.0f))
        {
            return;
        }
    }

    const D3DXVECTOR4 lightDirection = Light::GetLightDir();
    const D3DXVECTOR4 lightColor(Light::GetLightColor());
    const D3DXVECTOR4 ambientColor(Light::GetAmbientColor());
    const D3DXVECTOR4 cameraPosition(Camera::GetEyePos(), 1.0f);
    ThrowIfEffectCallFailed(m_D3DEffect->SetVector("g_lightDir", &lightDirection),
                            "MeshMix2 failed to set g_lightDir.");
    ThrowIfEffectCallFailed(m_D3DEffect->SetVector("g_lightColor", &lightColor),
                            "MeshMix2 failed to set g_lightColor.");
    ThrowIfEffectCallFailed(m_D3DEffect->SetVector("g_ambient", &ambientColor),
                            "MeshMix2 failed to set g_ambient.");
    ThrowIfEffectCallFailed(m_D3DEffect->SetVector("g_cameraPos", &cameraPosition),
                            "MeshMix2 failed to set g_cameraPos.");
    ThrowIfEffectCallFailed(m_D3DEffect->SetFloat("g_fSunLightIntensity", Light::GetBrightness()),
                            "MeshMix2 failed to set g_fSunLightIntensity.");
    ThrowIfEffectCallFailed(m_D3DEffect->SetFloat("g_fAmbientIntensity", Light::GetAmbientBrightness()),
                            "MeshMix2 failed to set g_fAmbientIntensity.");
    ThrowIfEffectCallFailed(m_D3DEffect->SetBool("g_damageFlash", m_damageFlash),
                            "MeshMix2 failed to set g_damageFlash.");
    ThrowIfEffectCallFailed(m_D3DEffect->SetBool("g_bSaturateShadow", m_param.saturateShadow),
                            "MeshMix2 failed to set g_bSaturateShadow.");
    ThrowIfEffectCallFailed(m_D3DEffect->SetFloat("g_fSaturateShadowIntensity", m_param.saturateShadowIntensity),
                            "MeshMix2 failed to set g_fSaturateShadowIntensity.");
    ThrowIfEffectCallFailed(m_D3DEffect->SetFloat("g_fShadowDarkness", m_param.shadowDarkness),
                            "MeshMix2 failed to set g_fShadowDarkness.");
    ThrowIfEffectCallFailed(m_D3DEffect->SetBool("g_fresnelEnable", m_param.fresnel),
                            "MeshMix2 failed to set g_fresnelEnable.");
    ThrowIfEffectCallFailed(m_D3DEffect->SetFloat("g_fresnelIntensity", m_param.fresnelIntensity),
                            "MeshMix2 failed to set g_fresnelIntensity.");
    ThrowIfEffectCallFailed(m_D3DEffect->SetBool("g_bPOM", m_param.parallaxOcclusionMapping),
                            "MeshMix2 failed to set g_bPOM.");
    ThrowIfEffectCallFailed(m_D3DEffect->SetBool("g_bNormalMapping", m_param.normalMapping),
                            "MeshMix2 failed to set g_bNormalMapping.");
    ThrowIfEffectCallFailed(m_D3DEffect->SetBool("g_bSSS", m_param.sss),
                            "MeshMix2 failed to set g_bSSS.");
    ThrowIfEffectCallFailed(m_D3DEffect->SetFloat("g_sssIntensity", m_param.sssIntensity),
                            "MeshMix2 failed to set g_sssIntensity.");
    D3DXVECTOR4 sssColor;
    sssColor.x = static_cast<float>((m_param.sssColor >> 16) & 0xff) / 255.0f;
    sssColor.y = static_cast<float>((m_param.sssColor >> 8) & 0xff) / 255.0f;
    sssColor.z = static_cast<float>(m_param.sssColor & 0xff) / 255.0f;
    sssColor.w = static_cast<float>((m_param.sssColor >> 24) & 0xff) / 255.0f;
    ThrowIfEffectCallFailed(m_D3DEffect->SetVector("g_sssColor", &sssColor),
                            "MeshMix2 failed to set g_sssColor.");
    ThrowIfEffectCallFailed(m_D3DEffect->SetFloat("g_cubeMappingRate", m_param.cubeMappingRate),
                            "MeshMix2 failed to set g_cubeMappingRate.");
    ThrowIfEffectCallFailed(m_D3DEffect->SetFloat("g_cubeMappingGauss", m_param.cubeMappingGauss),
                            "MeshMix2 failed to set g_cubeMappingGauss.");
    ThrowIfEffectCallFailed(m_D3DEffect->SetBool("g_waterMirrorEnable", m_param.waterMirror),
                            "MeshMix2 failed to set g_waterMirrorEnable.");
    ThrowIfEffectCallFailed(m_D3DEffect->SetFloat("g_emitIntensity", m_param.emitIntensity),
                            "MeshMix2 failed to set g_emitIntensity.");
    D3DXVECTOR4 emitColor;
    emitColor.x = static_cast<float>((m_param.emitColor >> 16) & 0xff) / 255.0f;
    emitColor.y = static_cast<float>((m_param.emitColor >> 8) & 0xff) / 255.0f;
    emitColor.z = static_cast<float>(m_param.emitColor & 0xff) / 255.0f;
    emitColor.w = 1.0f;
    ThrowIfEffectCallFailed(m_D3DEffect->SetVector("g_emitColor", &emitColor),
                            "MeshMix2 failed to set g_emitColor.");
    static float effectTime = 0.0f;
    effectTime += 0.01f;
    ThrowIfEffectCallFailed(m_D3DEffect->SetFloat("g_time", effectTime),
                            "MeshMix2 failed to set g_time.");
    ThrowIfEffectCallFailed(m_D3DEffect->SetBool("g_swayEnable", m_param.sway),
                            "MeshMix2 failed to set g_swayEnable.");
    ThrowIfEffectCallFailed(m_D3DEffect->SetFloat("g_swayAmount", m_param.swayIntensity),
                            "MeshMix2 failed to set g_swayAmount.");
    ThrowIfEffectCallFailed(m_D3DEffect->SetFloat("g_swaySpeed", 1.0f),
                            "MeshMix2 failed to set g_swaySpeed.");
    ThrowIfEffectCallFailed(m_D3DEffect->SetBool("g_waveEnable", m_param.wave),
                            "MeshMix2 failed to set g_waveEnable.");
    ThrowIfEffectCallFailed(m_D3DEffect->SetFloat("g_waveAmount", m_param.waveIntensity),
                            "MeshMix2 failed to set g_waveAmount.");
    ThrowIfEffectCallFailed(m_D3DEffect->SetFloat("g_waveSpeed", 5.0f),
                            "MeshMix2 failed to set g_waveSpeed.");
    ThrowIfEffectCallFailed(m_D3DEffect->SetFloat("g_waveDensity", 20.0f),
                            "MeshMix2 failed to set g_waveDensity.");
    const float screenSize[2] =
    {
        static_cast<float>(Common::ScreenW()),
        static_cast<float>(Common::ScreenH())
    };
    ThrowIfEffectCallFailed(m_D3DEffect->SetFloatArray("g_screenSize", screenSize, 2),
                            "MeshMix2 failed to set g_screenSize.");
    ThrowIfEffectCallFailed(m_D3DEffect->SetTexture("g_texThickness", g_meshMix2ThicknessTexture),
                            "MeshMix2 failed to set g_texThickness.");
    ThrowIfEffectCallFailed(m_D3DEffect->SetTexture("g_texMirror", g_meshMix2MirrorTexture),
                            "MeshMix2 failed to set g_texMirror.");
    ThrowIfEffectCallFailed(m_D3DEffect->SetMatrix("g_matMirrorViewProj", &g_meshMix2MirrorViewProjection),
                            "MeshMix2 failed to set g_matMirrorViewProj.");
    ThrowIfEffectCallFailed(m_D3DEffect->SetBool("g_mirrorClipEnable", g_meshMix2MirrorClipEnabled),
                            "MeshMix2 failed to set g_mirrorClipEnable.");
    ThrowIfEffectCallFailed(m_D3DEffect->SetVector("g_mirrorClipPlane", &g_meshMix2MirrorClipPlane),
                            "MeshMix2 failed to set g_mirrorClipPlane.");
    ThrowIfEffectCallFailed(m_D3DEffect->SetTexture("g_texCubeMap", nullptr),
                            "MeshMix2 failed to clear g_texCubeMap.");
    ThrowIfEffectCallFailed(m_D3DEffect->SetTexture("g_texNormalMap", nullptr),
                            "MeshMix2 failed to clear g_texNormalMap.");
    ThrowIfEffectCallFailed(m_D3DEffect->SetTexture("g_texHeightMap", nullptr),
                            "MeshMix2 failed to clear g_texHeightMap.");

    const std::deque<PointLightInfo> pointLightList = Light::GetPointLightList();
    D3DXVECTOR4 pointLightPositions[16];
    float pointLightBrightness[16] { };
    float pointLightShapes[16] { };
    float pointLightLineLengths[16] { };
    float pointLightSquareWidths[16] { };
    float pointLightSquareHeights[16] { };
    D3DXVECTOR4 pointLightRotations[16];
    D3DXVECTOR4 pointLightColors[16];
    ZeroMemory(pointLightPositions, sizeof(pointLightPositions));
    ZeroMemory(pointLightRotations, sizeof(pointLightRotations));
    ZeroMemory(pointLightColors, sizeof(pointLightColors));

    for (std::size_t index = 0; index < 16; ++index)
    {
        if (!m_param.pointLight)
        {
            continue;
        }

        if (index >= pointLightList.size())
        {
            continue;
        }

        const PointLightInfo& pointLight = pointLightList.at(index);
        pointLightPositions[index].x = pointLight.m_pos.x;
        pointLightPositions[index].y = pointLight.m_pos.y;
        pointLightPositions[index].z = pointLight.m_pos.z;
        pointLightBrightness[index] = pointLight.m_brightness;
        pointLightShapes[index] = PointLightShapeToShaderValue(pointLight.m_shape);
        pointLightLineLengths[index] = pointLight.m_lineLength;
        pointLightSquareWidths[index] = pointLight.m_squareWidth;
        pointLightSquareHeights[index] = pointLight.m_squareHeight;
        pointLightRotations[index].x = pointLight.m_rotation.x;
        pointLightRotations[index].y = pointLight.m_rotation.y;
        pointLightRotations[index].z = pointLight.m_rotation.z;
        pointLightColors[index].x = pointLight.m_color.r;
        pointLightColors[index].y = pointLight.m_color.g;
        pointLightColors[index].z = pointLight.m_color.b;
    }

    ThrowIfEffectCallFailed(m_D3DEffect->SetVectorArray("g_pointLightPos",
                                                         pointLightPositions,
                                                         16),
                            "MeshMix2 failed to set g_pointLightPos.");
    ThrowIfEffectCallFailed(m_D3DEffect->SetFloatArray("g_pointLightBrightness",
                                                        pointLightBrightness,
                                                        16),
                            "MeshMix2 failed to set g_pointLightBrightness.");
    ThrowIfEffectCallFailed(m_D3DEffect->SetFloatArray("g_pointLightShape",
                                                        pointLightShapes,
                                                        16),
                            "MeshMix2 failed to set g_pointLightShape.");
    ThrowIfEffectCallFailed(m_D3DEffect->SetFloatArray("g_pointLightLineLength",
                                                        pointLightLineLengths,
                                                        16),
                            "MeshMix2 failed to set g_pointLightLineLength.");
    ThrowIfEffectCallFailed(m_D3DEffect->SetFloatArray("g_pointLightSquareWidth",
                                                        pointLightSquareWidths,
                                                        16),
                            "MeshMix2 failed to set g_pointLightSquareWidth.");
    ThrowIfEffectCallFailed(m_D3DEffect->SetFloatArray("g_pointLightSquareHeight",
                                                        pointLightSquareHeights,
                                                        16),
                            "MeshMix2 failed to set g_pointLightSquareHeight.");
    ThrowIfEffectCallFailed(m_D3DEffect->SetVectorArray("g_pointLightRotation",
                                                         pointLightRotations,
                                                         16),
                            "MeshMix2 failed to set g_pointLightRotation.");
    ThrowIfEffectCallFailed(m_D3DEffect->SetVectorArray("g_pointLightColor",
                                                         pointLightColors,
                                                         16),
                            "MeshMix2 failed to set g_pointLightColor.");
    ThrowIfEffectCallFailed(m_D3DEffect->SetTechnique("Technique1"),
                            "MeshMix2 failed to set Technique1.");

    const D3DXMATRIX viewProjectionMatrix = Camera::GetViewMatrix() * Camera::GetProjMatrix();
    RenderFrameHierarchy(m_frameRoot,
                         m_D3DEffect,
                         viewProjectionMatrix,
                         true,
                         renderAsMirrorSurface);
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

    RenderFrameHierarchy(m_frameRoot, effect, viewProjectionMatrix, false, false);
}

void MeshMix2::RenderFrameHierarchy(LPD3DXFRAME frame,
                                    LPD3DXEFFECT effect,
                                    const D3DXMATRIX& viewProjectionMatrix,
                                    const bool configureMaterial,
                                    const bool renderAsMirrorSurface)
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
                            configureMaterial,
                            renderAsMirrorSurface);
        containerBase = containerBase->pNextMeshContainer;
    }

    RenderFrameHierarchy(frame->pFrameSibling,
                         effect,
                         viewProjectionMatrix,
                         configureMaterial,
                         renderAsMirrorSurface);
    RenderFrameHierarchy(frame->pFrameFirstChild,
                         effect,
                         viewProjectionMatrix,
                         configureMaterial,
                         renderAsMirrorSurface);
}

void MeshMix2::RenderMeshContainer(const MeshMix2Frame& frame,
                                   MeshMix2MeshContainer& container,
                                   LPD3DXEFFECT effect,
                                   const D3DXMATRIX& viewProjectionMatrix,
                                   const bool configureMaterial,
                                   const bool renderAsMirrorSurface)
{
    LPD3DXMESH mesh = container.MeshData.pMesh;
    if (mesh == nullptr)
    {
        throw std::runtime_error("MeshMix2 encountered a null mesh container.");
    }

    const D3DXMATRIX worldViewProjection = frame.m_combinedMatrix * viewProjectionMatrix;
    ThrowIfEffectCallFailed(effect->SetMatrix("g_matWorld", &frame.m_combinedMatrix),
                            "MeshMix2 failed to set g_matWorld.");

    const D3DXHANDLE worldViewProjectionHandle =
        effect->GetParameterByName(nullptr, "g_matWorldViewProj");
    if (worldViewProjectionHandle != nullptr)
    {
        ThrowIfEffectCallFailed(effect->SetMatrix(worldViewProjectionHandle,
                                                  &worldViewProjection),
                                "MeshMix2 failed to set g_matWorldViewProj.");
    }

    const D3DXHANDLE viewProjectionHandle = effect->GetParameterByName(nullptr, "g_matViewProj");
    if (viewProjectionHandle != nullptr)
    {
        ThrowIfEffectCallFailed(effect->SetMatrix(viewProjectionHandle, &viewProjectionMatrix),
                                "MeshMix2 failed to set g_matViewProj.");
    }
    else if (configureMaterial)
    {
        throw std::runtime_error(
            "MeshMix2 display effect does not define the required g_matViewProj matrix.");
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
                specularPower = ConvertMaterialSpecularPower(material);
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
                ThrowIfEffectCallFailed(
                    effect->SetTexture("g_texture", container.m_textureList[subsetIndex]),
                    "MeshMix2 failed to set g_texture.");
                hasTexture = true;
                diffuse = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f);
            }
            else
            {
                ThrowIfEffectCallFailed(effect->SetTexture("g_texture", nullptr),
                                        "MeshMix2 failed to clear g_texture.");
            }

            BOOL treatTextureAsWhite = FALSE;
            if (m_param.treatTextureAsWhite || !hasTexture)
            {
                treatTextureAsWhite = TRUE;
            }
            ThrowIfEffectCallFailed(effect->SetBool("g_treatTextureAsWhite", treatTextureAsWhite),
                                    "MeshMix2 failed to set g_treatTextureAsWhite.");
            ThrowIfEffectCallFailed(effect->SetVector("g_diffuse", &diffuse),
                                    "MeshMix2 failed to set g_diffuse.");
            ThrowIfEffectCallFailed(effect->SetFloat("g_specularIntensity", specularIntensity),
                                    "MeshMix2 failed to set g_specularIntensity.");
            ThrowIfEffectCallFailed(effect->SetFloat("g_specularPower", specularPower),
                                    "MeshMix2 failed to set g_specularPower.");
        }

        ThrowIfEffectCallFailed(effect->CommitChanges(),
                                "MeshMix2 failed to commit effect changes.");
        UINT passCount = 0;
        const HRESULT beginResult = effect->Begin(&passCount, 0);
        if (FAILED(beginResult))
        {
            throw std::runtime_error("MeshMix2 failed to begin an effect.");
        }
        for (UINT passIndex = 0; passIndex < passCount; ++passIndex)
        {
            bool drawPass = true;
            if (configureMaterial)
            {
                drawPass = false;
                if (renderAsMirrorSurface && m_param.mirror)
                {
                    drawPass = passIndex == 5;
                }
                else if (m_param.emit)
                {
                    drawPass = passIndex == 4;
                }
                else if (passIndex == 0 || passIndex == 3)
                {
                    drawPass = true;
                }
                else if (passIndex == 1 && m_param.cubeMapping)
                {
                    drawPass = true;
                }
                else if (passIndex == 2 && m_param.glass)
                {
                    drawPass = true;
                }
            }

            if (!drawPass)
            {
                continue;
            }

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
            ThrowIfEffectCallFailed(effect->EndPass(),
                                    "MeshMix2 failed to end an effect pass.");
        }
        ThrowIfEffectCallFailed(effect->End(),
                                "MeshMix2 failed to end an effect.");
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
            const DWORD vertexStride = mesh->GetNumBytesPerVertex();
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
void MeshMix2::SetFresnelIntensity(const float intensity) { m_param.fresnelIntensity = intensity; }
void MeshMix2::SetCubeMappingRate(const float rate) { m_param.cubeMappingRate = rate; }
void MeshMix2::SetSpecularIntensityOverrideEnabled(const bool enabled) { m_param.specularIntensityOverrideEnabled = enabled; }
void MeshMix2::SetSpecularEdgeOverrideEnabled(const bool enabled) { m_param.specularEdgeOverrideEnabled = enabled; }
void MeshMix2::SetTreatTextureAsWhite(const bool enabled) { m_param.treatTextureAsWhite = enabled; }
void MeshMix2::SetSSS(const bool enabled) { m_param.sss = enabled; }
void MeshMix2::SetSSSIntensity(const float intensity) { m_param.sssIntensity = intensity; }
void MeshMix2::SetSSSColor(const DWORD color) { m_param.sssColor = color; }
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
