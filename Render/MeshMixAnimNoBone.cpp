#include "MeshMixAnimNoBone.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <fstream>
#include <string>
#include <Shlwapi.h>
#include "Camera.h"
#include "Common.h"
#include "CustomXLoader.h"
#include "Light.h"
#include "Util.h"

namespace NSRender
{
namespace
{
constexpr double D3DX64_ANIMATION_TIME_SCALE = 160.0;

std::wstring GetResolvedPath(const std::wstring& path)
{
    if (PathIsRelative(path.c_str()))
    {
        return Util::GetExeDir() + path;
    }

    return path;
}

bool FileExists(const std::wstring& filePath)
{
    const DWORD attributes = GetFileAttributesW(filePath.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES)
    {
        return false;
    }

    return (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

std::wstring GetCurrentDirectoryPath()
{
    wchar_t currentDirectory[MAX_PATH] { };
    const DWORD length = GetCurrentDirectoryW(_countof(currentDirectory), currentDirectory);
    if (length == 0 || length >= _countof(currentDirectory))
    {
        return L"";
    }

    std::wstring result(currentDirectory);
    if (!result.empty() && result.back() != L'\\' && result.back() != L'/')
    {
        result += L"\\";
    }

    return result;
}

std::wstring ResolveRuntimeFilePath(const std::wstring& fileName)
{
    const std::wstring exePath = Util::GetExeDir() + fileName;
    if (FileExists(exePath))
    {
        return exePath;
    }

    const std::wstring currentDirectory = GetCurrentDirectoryPath();
    if (!currentDirectory.empty())
    {
        const std::wstring currentDirectoryPath = currentDirectory + fileName;
        if (FileExists(currentDirectoryPath))
        {
            return currentDirectoryPath;
        }
    }

    return exePath;
}

std::wstring GetDirectoryPath(const std::wstring& path)
{
    const std::size_t pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos)
    {
        return L"";
    }

    return path.substr(0, pos);
}

std::wstring GetPathWithoutExtension(const std::wstring& path)
{
    const std::size_t slashPos = path.find_last_of(L"\\/");
    const std::size_t dotPos = path.find_last_of(L'.');
    if (dotPos == std::wstring::npos ||
        (slashPos != std::wstring::npos && dotPos < slashPos))
    {
        return path;
    }

    return path.substr(0, dotPos);
}

std::wstring Utf8BytesToWideString(const std::string& bytes)
{
    if (bytes.empty())
    {
        return L"";
    }

    const char* data = bytes.data();
    int size = static_cast<int>(bytes.size());
    if (size >= 3 &&
        static_cast<unsigned char>(data[0]) == 0xef &&
        static_cast<unsigned char>(data[1]) == 0xbb &&
        static_cast<unsigned char>(data[2]) == 0xbf)
    {
        data += 3;
        size -= 3;
    }

    const int required = MultiByteToWideChar(CP_UTF8, 0, data, size, nullptr, 0);
    if (required <= 0)
    {
        return L"";
    }

    std::wstring result(required, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, data, size, &result[0], required);
    return result;
}

double GetAnimationControllerDuration(LPD3DXANIMATIONCONTROLLER controller)
{
    if (controller == nullptr || controller->GetMaxNumAnimationSets() == 0)
    {
        return 1.0;
    }

    LPD3DXANIMATIONSET animationSet = nullptr;
    if (FAILED(controller->GetAnimationSet(0, &animationSet)) || animationSet == nullptr)
    {
        return 1.0;
    }

    const double duration = (std::max)(animationSet->GetPeriod(), 0.0001);
    SAFE_RELEASE(animationSet);
    return duration;
}

std::wstring TrimText(const std::wstring& text)
{
    const std::wstring whitespace = L" \t\r\n";
    const std::size_t first = text.find_first_not_of(whitespace);
    if (first == std::wstring::npos)
    {
        return L"";
    }

    const std::size_t last = text.find_last_not_of(whitespace);
    return text.substr(first, last - first + 1);
}

std::wstring UnquoteText(const std::wstring& text)
{
    const std::wstring trimmed = TrimText(text);
    if (trimmed.size() >= 2 && trimmed.front() == L'"' && trimmed.back() == L'"')
    {
        return trimmed.substr(1, trimmed.size() - 2);
    }

    return trimmed;
}

std::vector<std::wstring> SplitCsvLineText(const std::wstring& line)
{
    std::vector<std::wstring> fields;
    std::wstring currentField;
    bool inQuotes = false;

    for (std::size_t i = 0; i < line.size(); ++i)
    {
        const wchar_t ch = line[i];
        if (ch == L'"')
        {
            if (inQuotes && (i + 1) < line.size() && line[i + 1] == L'"')
            {
                currentField += L'"';
                ++i;
            }
            else
            {
                inQuotes = !inQuotes;
            }
        }
        else if (ch == L',' && !inQuotes)
        {
            fields.push_back(currentField);
            currentField.clear();
        }
        else
        {
            currentField += ch;
        }
    }
    fields.push_back(currentField);
    return fields;
}
}

MeshMixAnimNoBone::MeshMixAnimNoBone(const std::wstring& filename,
    const D3DXVECTOR3& pos, const D3DXVECTOR3& rotate, const float scale,
    const stMeshParam& param, const AnimSetMap& animSetMap,
    const MeshMixSkinAnimLoadMode loadMode)
    : m_meshName(filename), m_allocator(filename), m_animationAllocator(L""),
      m_pos(pos), m_rotate(rotate), m_scale(scale), m_param(param),
      m_animSetMap(animSetMap), m_loadMode(loadMode) {}

MeshMixAnimNoBone::~MeshMixAnimNoBone()
{
    Common::RemoveDeviceLostResource(this);

    if (m_loadThread.joinable())
    {
        m_loadThread.join();
    }

    SAFE_RELEASE(m_D3DEffect);
    SAFE_RELEASE(m_tempAnimController);
    m_animController.Finalize();
    ReleaseAnimationClips();

    if (m_frameRoot != nullptr)
    {
        ReleaseMeshAllocatorRecursive(m_frameRoot, m_allocator);
        m_frameRoot = nullptr;
    }

    if (m_animationFrameRoot != nullptr)
    {
        ReleaseMeshAllocatorRecursive(m_animationFrameRoot, m_animationAllocator);
        m_animationFrameRoot = nullptr;
    }
}

void MeshMixAnimNoBone::Initialize(bool) { m_loadThread = std::thread([this]() {
    HRESULT hr = E_FAIL;
    std::wstring path = ResolveRuntimeFilePath(SHADER_FILENAME);

    OutputDebugStringW((L"[MeshMixAnimNoBone] Shader path: " + path + L"\n").c_str());

    hr = D3DXCreateEffectFromFile(Common::D3DDevice(), path.c_str(), nullptr, nullptr, 0, nullptr, &m_D3DEffect, nullptr);
    if (FAILED(hr))
    {
        OutputDebugStringW((L"[MeshMixAnimNoBone] ERROR: Failed to create effect. HR=" + FormatHRESULT(hr) + L"\n").c_str());
        return;
    }

    LPD3DXANIMATIONCONTROLLER tempAnimController = nullptr;

    if (PathIsRelative(m_meshName.c_str()))
    {
        path = Util::GetExeDir() + m_meshName;
    }
    else
    {
        path = m_meshName;
    }

    OutputDebugStringW((L"[MeshMixAnimNoBone] Mesh path: " + path + L"\n").c_str());
    if (!FileExists(path))
    {
        OutputDebugStringW(std::wstring(L"[MeshMixAnimNoBone] ERROR: Mesh file does not exist\n").c_str());
    }

    hr = LoadMeshHierarchy(path,
                           m_allocator,
                           &m_frameRoot,
                           &tempAnimController);
    OutputDebugStringW((L"[MeshMixAnimNoBone] LoadMeshHierarchy HR=" + FormatHRESULT(hr) +
                        L" FrameRoot=" + std::to_wstring(reinterpret_cast<std::uintptr_t>(m_frameRoot)) +
                        L" AnimCtrl=" + std::to_wstring(reinterpret_cast<std::uintptr_t>(tempAnimController)) + L"\n").c_str());
    if (FAILED(hr) || m_frameRoot == nullptr)
    {
        SAFE_RELEASE(tempAnimController);
        OutputDebugStringW(std::wstring(L"[MeshMixAnimNoBone] ERROR: Failed to load mesh\n").c_str());
        OutputDebugStringW(std::wstring(L"[MeshMixAnimNoBone] HINT: Check .x mesh data consistency, especially MeshNormals face counts.\n").c_str());
        return;
    }

    if (tempAnimController == nullptr)
    {
        OutputDebugStringW(std::wstring(L"[MeshMixAnimNoBone] WARNING: tempAnimController is NULL - no animation in file\n").c_str());
    }

    const bool loadedAnimationCsv = LoadAnimationCsv();
    OutputDebugStringW((L"[MeshMixAnimNoBone] LoadAnimationCsv=" + std::to_wstring(loadedAnimationCsv) + L"\n").c_str());

    if (loadedAnimationCsv)
    {
        OutputDebugStringW(std::wstring(L"[MeshMixAnimNoBone] Using CSV animation clips\n").c_str());
        SAFE_RELEASE(tempAnimController);
        m_useExternalAnimation = true;
    }
    else if (m_useExternalAnimation)
    {
        SAFE_RELEASE(tempAnimController);

        if (PathIsRelative(m_animationMeshName.c_str()))
        {
            path = Util::GetExeDir() + m_animationMeshName;
        }
        else
        {
            path = m_animationMeshName;
        }

        OutputDebugStringW((L"[MeshMixAnimNoBone] Animation mesh path: " + path + L"\n").c_str());

        hr = LoadMeshHierarchy(path,
                               m_animationAllocator,
                               &m_animationFrameRoot,
                               &tempAnimController);
        OutputDebugStringW((L"[MeshMixAnimNoBone] Anim mesh load HR=" + FormatHRESULT(hr) +
                            L" FrameRoot=" + std::to_wstring(reinterpret_cast<std::uintptr_t>(m_animationFrameRoot)) +
                            L" AnimCtrl=" + std::to_wstring(reinterpret_cast<std::uintptr_t>(tempAnimController)) + L"\n").c_str());
        if (FAILED(hr) || m_animationFrameRoot == nullptr || tempAnimController == nullptr)
        {
            SAFE_RELEASE(tempAnimController);
            OutputDebugStringW(std::wstring(L"[MeshMixAnimNoBone] ERROR: Failed to load animation mesh\n").c_str());
            return;
        }

        ReleaseMeshContainersRecursive(m_animationFrameRoot, m_animationAllocator);
    }
    else if (tempAnimController != nullptr)
    {
        OutputDebugStringW(std::wstring(L"[MeshMixAnimNoBone] Using embedded animation controller\n").c_str());
        if (m_animSetMap.empty())
        {
            OutputDebugStringW(std::wstring(L"[MeshMixAnimNoBone] Embedded animation uses direct controller update\n").c_str());
            m_tempAnimController = tempAnimController;
            tempAnimController = nullptr;
            m_embeddedAnimationDuration = GetAnimationControllerDuration(m_tempAnimController);
        }
        else
        {
            m_animController.Init(tempAnimController, m_animSetMap);
            tempAnimController = nullptr;
        }
        m_hasAnimationController = true;
    }
    else
    {
        OutputDebugStringW(std::wstring(L"[MeshMixAnimNoBone] No animation controller (static mesh mode)\n").c_str());
        SAFE_RELEASE(tempAnimController);
    }

    Common::AddDeviceLostResource(this);
    m_bLoaded = true;
    OutputDebugStringW(std::wstring(L"[MeshMixAnimNoBone] Initialize complete\n").c_str());
}); WaitForLoad(); }

void MeshMixAnimNoBone::WaitForLoad() { if (m_loadThread.joinable()) m_loadThread.join(); }

void MeshMixAnimNoBone::UpdateAnimation()
{
    if (!m_bLoaded || !m_enabled)
    {
        return;
    }

    if (!m_animationClips.empty())
    {
        UpdateActiveAnimationClip();
    }
    else if (m_hasAnimationController)
    {
        if (m_tempAnimController != nullptr)
        {
            const double deltaTime = m_animationSpeed * Common::ANIMATION_SPEED / D3DX64_ANIMATION_TIME_SCALE;
            m_embeddedAnimationTime += deltaTime;
            if (m_embeddedAnimationTime >= m_embeddedAnimationDuration)
            {
                m_embeddedAnimationTime = 0.0;
                m_tempAnimController->SetTrackPosition(0, 0.0);
            }
            m_tempAnimController->AdvanceTime(deltaTime, nullptr);
        }
        else
        {
            m_animController.Update();
        }
    }

    if (!m_animationClips.empty())
    {
        const AnimationClip& clip = m_animationClips.at(m_activeAnimationClipIndex);
        ApplyAnimationFrameTransformsToMeshHierarchy(m_frameRoot, clip.frameRoot);
    }
    else if (m_useExternalAnimation)
    {
        ApplyAnimationFrameTransformsToMeshHierarchy(m_frameRoot, m_animationFrameRoot);
    }

    D3DXMATRIX worldMatrix = BuildWorldMatrix();
    UpdateFrameMatrix(m_frameRoot, &worldMatrix);
}

void MeshMixAnimNoBone::Render()
{
    if (!m_bLoaded || !m_enabled || !m_D3DEffect)
    {
        return;
    }

    BOOL useWhiteTexture = FALSE;
    if (m_damageFlash || m_yellowFlash)
    {
        useWhiteTexture = TRUE;
    }

    BOOL damageFlash = FALSE;
    if (m_damageFlash)
    {
        damageFlash = TRUE;
    }

    BOOL yellowFlash = FALSE;
    if (m_yellowFlash)
    {
        yellowFlash = TRUE;
    }

    BOOL alphaClipEnabled = FALSE;
    if (m_alphaClipEnabled)
    {
        alphaClipEnabled = TRUE;
    }

    const D3DXVECTOR4 lightDir = Light::GetLightDir();
    m_D3DEffect->SetVector("g_lightDir", &lightDir);
    D3DXVECTOR4 lc(Light::GetLightColor()); m_D3DEffect->SetVector("g_lightColor", &lc);
    m_D3DEffect->SetFloat("g_fSunLightIntensity", Light::GetBrightness());
    m_D3DEffect->SetFloat("g_fAmbientIntensity", Light::GetAmbientBrightness());
    D3DXVECTOR4 cp(Camera::GetEyePos(), 1.0f); m_D3DEffect->SetVector("g_cameraPos", &cp);
    m_D3DEffect->SetBool("g_treatTextureAsWhite", useWhiteTexture);
    m_D3DEffect->SetBool("g_damageFlash", damageFlash);
    m_D3DEffect->SetBool("g_yellowFlash", yellowFlash);
    m_D3DEffect->SetBool("g_alphaClipEnabled", alphaClipEnabled);

    D3DXMATRIX viewProjectionMatrix = Camera::GetViewMatrix() * Camera::GetProjMatrix();
    m_D3DEffect->SetMatrix("g_matViewProj", &viewProjectionMatrix);

    m_D3DEffect->SetTechnique("TechniqueNoSkin");
    RenderFrameHierarchy(m_frameRoot, m_D3DEffect);
}

void MeshMixAnimNoBone::RenderToEffect(LPD3DXEFFECT e)
{
    if (!m_bLoaded || !m_enabled) return;
    RenderFrameHierarchy(m_frameRoot, e);
}

void MeshMixAnimNoBone::RenderFrameHierarchy(LPD3DXFRAME frame, LPD3DXEFFECT e)
{
    if (!frame) return;
    auto nf = reinterpret_cast<AnimNoBoneFrame*>(frame);
    if (frame->pMeshContainer) {
        auto c = reinterpret_cast<AnimNoBoneMeshContainer*>(frame->pMeshContainer);
        D3DXMATRIX wvp = nf->m_combinedMatrix;
        wvp *= Camera::GetViewMatrix(); wvp *= Camera::GetProjMatrix();
        e->SetMatrix("g_matWorld", &nf->m_combinedMatrix);
        e->SetMatrix("g_matWorldViewProj", &wvp);
        D3DXHANDLE viewProjectionHandle = e->GetParameterByName(nullptr, "g_matViewProj");
        if (viewProjectionHandle != nullptr)
        {
            D3DXMATRIX viewProjectionMatrix = Camera::GetViewMatrix() * Camera::GetProjMatrix();
            e->SetMatrix(viewProjectionHandle, &viewProjectionMatrix);
        }
        LPD3DXMESH m = c->MeshData.pMesh;
        DWORD ns = c->NumMaterials; if (ns == 0) ns = 1;
        for (DWORD i = 0; i < ns; ++i) {
            BOOL subsetTreatTextureAsWhite = FALSE;
            if (m_damageFlash || m_yellowFlash || m_param.treatTextureAsWhite)
            {
                subsetTreatTextureAsWhite = TRUE;
            }

            BOOL subsetAlphaClipEnabled = FALSE;
            if (m_alphaClipEnabled)
            {
                subsetAlphaClipEnabled = TRUE;
            }

            bool subsetHasTexture = false;
            if (i < c->m_textureList.size() && c->m_textureList[i])
            {
                e->SetTexture("g_textureSampler", c->m_textureList[i]);
                subsetHasTexture = true;
            }

            if (!subsetHasTexture)
            {
                e->SetTexture("g_textureSampler", nullptr);
                subsetTreatTextureAsWhite = TRUE;
                subsetAlphaClipEnabled = FALSE;
            }

            D3DXVECTOR4 d(1,1,1,1);
            if (i < c->NumMaterials) {
                const D3DMATERIAL9& mat = c->pMaterials[i].MatD3D;
                d = D3DXVECTOR4(mat.Diffuse.r, mat.Diffuse.g, mat.Diffuse.b, mat.Diffuse.a);
            }
            e->SetBool("g_treatTextureAsWhite", subsetTreatTextureAsWhite);
            e->SetBool("g_alphaClipEnabled", subsetAlphaClipEnabled);
            e->SetVector("g_diffuse", &d); e->CommitChanges();
            UINT pn = 0; e->Begin(&pn, 0);
            for (UINT p = 0; p < pn; ++p) { e->BeginPass(p); m->DrawSubset(i); e->EndPass(); }
            e->End();
        }
    }
    if (frame->pFrameSibling) RenderFrameHierarchy(frame->pFrameSibling, e);
    if (frame->pFrameFirstChild) RenderFrameHierarchy(frame->pFrameFirstChild, e);
}

void MeshMixAnimNoBone::UpdateFrameMatrix(const LPD3DXFRAME fb, const LPD3DXMATRIX mp)
{
    auto f = reinterpret_cast<AnimNoBoneFrame*>(fb);
    if (!f) return;
    if (mp != nullptr)
    {
        f->m_combinedMatrix = f->TransformationMatrix * (*mp);
    }
    else
    {
        f->m_combinedMatrix = f->TransformationMatrix;
    }
    if (f->pFrameSibling) UpdateFrameMatrix(f->pFrameSibling, mp);
    if (f->pFrameFirstChild) UpdateFrameMatrix(f->pFrameFirstChild, &f->m_combinedMatrix);
}

D3DXMATRIX MeshMixAnimNoBone::BuildWorldMatrix() const
{
    D3DXMATRIX w; D3DXMatrixIdentity(&w); D3DXMATRIX m;
    D3DXMatrixTranslation(&m, -m_centerPos.x, -m_centerPos.y, -m_centerPos.z); w *= m;
    D3DXMatrixScaling(&m, m_scale, m_scale, m_scale); w *= m;
    D3DXMatrixRotationYawPitchRoll(&m, m_rotate.y, m_rotate.x, m_rotate.z); w *= m;
    D3DXMatrixTranslation(&m, m_pos.x, m_pos.y, m_pos.z); w *= m;
    return w;
}

void MeshMixAnimNoBone::ReleaseMeshAllocatorRecursive(LPD3DXFRAME f, AnimNoBoneMeshAlloc& a)
{
    if (!f) return;
    ReleaseMeshAllocatorRecursive(f->pFrameSibling, a);
    ReleaseMeshAllocatorRecursive(f->pFrameFirstChild, a);
    a.DestroyFrame(f);
}

// Simple setters / getters
void MeshMixAnimNoBone::SetPos(const D3DXVECTOR3& p) { m_pos = p; }
void MeshMixAnimNoBone::SetRotY(float r) { m_rotate.y = r; }
void MeshMixAnimNoBone::SetScale(float s) { m_scale = s; }
void MeshMixAnimNoBone::SetEnabled(bool e) { m_enabled = e; }
void MeshMixAnimNoBone::SetAnimationSpeed(float s) { m_animationSpeed = s; }
void MeshMixAnimNoBone::SetDamageFlash(bool v) { m_damageFlash = v; }
void MeshMixAnimNoBone::SetYellowFlash(bool v) { m_yellowFlash = v; }
void MeshMixAnimNoBone::SetAlphaClipEnabled(bool v) { m_alphaClipEnabled = v; }
void MeshMixAnimNoBone::SetIgnoreTransparentMaterial(bool v) { m_ignoreTransparentMaterial = v; }
void MeshMixAnimNoBone::SetTreatTextureAsWhite(bool v) { m_param.treatTextureAsWhite = v; }
void MeshMixAnimNoBone::SetSaturateShadow(bool v) { m_param.saturateShadow = v; }
void MeshMixAnimNoBone::SetSaturateShadowIntensity(float v) { m_param.saturateShadowIntensity = v; }
void MeshMixAnimNoBone::SetShadowDarkness(float v) { m_param.shadowDarkness = v; }
void MeshMixAnimNoBone::SetSpecularIntensity(float v) { m_param.specularIntensity = v; }
void MeshMixAnimNoBone::SetSpecularEdge(float v) { m_param.specularEdge = v; }
void MeshMixAnimNoBone::SetFresnelIntensity(float v) { m_param.fresnelIntensity = v; }
void MeshMixAnimNoBone::SetSpecularIntensityOverrideEnabled(bool v) { m_param.specularIntensityOverrideEnabled = v; }
void MeshMixAnimNoBone::SetSpecularEdgeOverrideEnabled(bool v) { m_param.specularEdgeOverrideEnabled = v; }
D3DXVECTOR3 MeshMixAnimNoBone::GetPos() const { return m_pos; }
D3DXVECTOR3 MeshMixAnimNoBone::GetRot() const { return m_rotate; }
float MeshMixAnimNoBone::GetScale() const { return m_scale; }
bool MeshMixAnimNoBone::IsEnabled() const { return m_enabled; }
bool MeshMixAnimNoBone::IsLoaded() const { return m_bLoaded; }
std::wstring MeshMixAnimNoBone::GetMeshName() const { return m_meshName; }
const std::vector<MeshMixAnimNoBone::AnimationInfo>& MeshMixAnimNoBone::GetAnimationInfoList() const { return m_animationInfoList; }
void MeshMixAnimNoBone::OnDeviceLost() { if (m_D3DEffect) m_D3DEffect->OnLostDevice(); }
void MeshMixAnimNoBone::OnDeviceReset() { if (m_D3DEffect) m_D3DEffect->OnResetDevice(); }

void MeshMixAnimNoBone::SetSharedMirrorClipPlane(bool, const D3DXVECTOR4&) {}

bool MeshMixAnimNoBone::PlayAnimation(const std::wstring& name)
{
    for (int i = 0; i < static_cast<int>(m_animationClips.size()); ++i)
    {
        if (m_animationClips.at(i).info.name != name)
        {
            continue;
        }

        m_activeAnimationClipIndex = i;
        m_animationClips.at(i).currentTime = 0.0;
        if (m_animationClips.at(i).controller != nullptr)
        {
            m_animationClips.at(i).controller->SetTrackPosition(0, 0.0);
            m_animationClips.at(i).controller->AdvanceTime(0.0, nullptr);
        }
        return true;
    }

    return false;
}

void MeshMixAnimNoBone::UpdateActiveAnimationClip()
{
    if (m_activeAnimationClipIndex < 0 ||
        m_activeAnimationClipIndex >= static_cast<int>(m_animationClips.size()))
    {
        return;
    }

    AnimationClip& clip = m_animationClips.at(m_activeAnimationClipIndex);
    if (clip.controller == nullptr)
    {
        return;
    }

    const double deltaTime = m_animationSpeed * Common::ANIMATION_SPEED / D3DX64_ANIMATION_TIME_SCALE;
    clip.currentTime += deltaTime;

    if (clip.currentTime >= clip.duration)
    {
        if (clip.stopWhenEnd)
        {
            clip.currentTime = clip.duration;
        }
        else
        {
            clip.currentTime = std::fmod(clip.currentTime, clip.duration);
        }
    }

    clip.controller->SetTrackPosition(0, clip.currentTime);
    clip.controller->AdvanceTime(0.0, nullptr);
}

void MeshMixAnimNoBone::ApplyAnimationFrameTransformsToMeshHierarchy(const LPD3DXFRAME meshFrameBase,
                                                                      const LPD3DXFRAME animationFrameRoot)
{
    if (meshFrameBase == nullptr || animationFrameRoot == nullptr)
    {
        return;
    }

    auto meshFrame = reinterpret_cast<AnimNoBoneFrame*>(meshFrameBase);
    if (meshFrame->Name != nullptr)
    {
        LPD3DXFRAME animationFrameBase = D3DXFrameFind(animationFrameRoot, meshFrame->Name);
        if (animationFrameBase != nullptr)
        {
            auto animationFrame = reinterpret_cast<AnimNoBoneFrame*>(animationFrameBase);
            meshFrame->TransformationMatrix = animationFrame->TransformationMatrix;
        }
    }

    if (meshFrame->pFrameSibling != nullptr)
    {
        ApplyAnimationFrameTransformsToMeshHierarchy(meshFrame->pFrameSibling, animationFrameRoot);
    }

    if (meshFrame->pFrameFirstChild != nullptr)
    {
        ApplyAnimationFrameTransformsToMeshHierarchy(meshFrame->pFrameFirstChild, animationFrameRoot);
    }
}

bool MeshMixAnimNoBone::LoadAnimationCsv()
{
    const std::wstring meshPath = GetResolvedPath(m_meshName);
    const std::wstring csvPath = GetPathWithoutExtension(meshPath) + L".csv";
    if (!PathFileExistsW(csvPath.c_str()))
    {
        return false;
    }

    std::ifstream file(csvPath, std::ios::binary);
    if (!file)
    {
        return false;
    }

    const std::string bytes((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
    const std::wstring csvText = Utf8BytesToWideString(bytes);
    const std::wstring csvDirectory = GetDirectoryPath(csvPath);

    std::vector<AnimationInfo> animationInfoList;
    std::wstring line;
    for (std::size_t i = 0; i <= csvText.size(); ++i)
    {
        if (i < csvText.size() && csvText[i] != L'\n')
        {
            line += csvText[i];
            continue;
        }

        const std::wstring trimmedLine = TrimText(line);
        line.clear();
        if (trimmedLine.empty())
        {
            continue;
        }

        const std::vector<std::wstring> fields = SplitCsvLineText(trimmedLine);
        if (fields.size() < 4 || TrimText(fields[0]) != L"Anim")
        {
            continue;
        }

        AnimationInfo info;
        info.name = UnquoteText(fields[1]);
        std::wstring filePath = UnquoteText(fields[2]);
        if (PathIsRelative(filePath.c_str()))
        {
            filePath = csvDirectory + L"\\" + filePath;
        }
        info.filePath = filePath;
        info.mode = UnquoteText(fields[3]);
        info.isDefault = (info.mode == L"default");

        if (!info.name.empty() && PathFileExistsW(info.filePath.c_str()))
        {
            animationInfoList.push_back(info);
        }
    }

    if (animationInfoList.empty())
    {
        return false;
    }

    int defaultIndex = 0;
    for (int i = 0; i < static_cast<int>(animationInfoList.size()); ++i)
    {
        if (!LoadAnimationClip(animationInfoList.at(i)))
        {
            continue;
        }

        if (animationInfoList.at(i).isDefault)
        {
            defaultIndex = static_cast<int>(m_animationClips.size()) - 1;
        }
    }

    if (m_animationClips.empty())
    {
        m_animationInfoList.clear();
        return false;
    }

    m_animationInfoList.clear();
    for (const auto& clip : m_animationClips)
    {
        m_animationInfoList.push_back(clip.info);
    }

    m_activeAnimationClipIndex = defaultIndex;
    return true;
}

bool MeshMixAnimNoBone::LoadAnimationClip(const AnimationInfo& info)
{
    AnimationClip clip;
    clip.info = info;
    clip.loop = (info.mode == L"loop" || info.mode == L"default");
    clip.stopWhenEnd = (info.mode == L"stopWhenEnd");
    clip.allocator = NEW AnimNoBoneMeshAlloc(info.filePath);

    HRESULT hr = LoadMeshHierarchy(info.filePath,
                                   *clip.allocator,
                                   &clip.frameRoot,
                                   &clip.controller);
    if (FAILED(hr) || clip.frameRoot == nullptr || clip.controller == nullptr)
    {
        SAFE_RELEASE(clip.controller);
        if (clip.frameRoot != nullptr)
        {
            ReleaseMeshAllocatorRecursive(clip.frameRoot, *clip.allocator);
            clip.frameRoot = nullptr;
        }
        SAFE_DELETE(clip.allocator);
        return false;
    }

    ReleaseMeshContainersRecursive(clip.frameRoot, *clip.allocator);
    if (clip.controller != nullptr)
    {
        clip.duration = GetAnimationControllerDuration(clip.controller);
    }
    m_animationClips.push_back(clip);
    return true;
}

bool MeshMixAnimNoBone::LoadAnimationClip(const std::wstring& filePath, AnimationClip& outClip)
{
    outClip.allocator = NEW AnimNoBoneMeshAlloc(filePath);

    HRESULT hr = LoadMeshHierarchy(filePath,
                                   *outClip.allocator,
                                   &outClip.frameRoot,
                                   &outClip.controller);
    if (FAILED(hr) || outClip.frameRoot == nullptr || outClip.controller == nullptr)
    {
        SAFE_RELEASE(outClip.controller);
        if (outClip.frameRoot != nullptr)
        {
            ReleaseMeshAllocatorRecursive(outClip.frameRoot, *outClip.allocator);
            outClip.frameRoot = nullptr;
        }
        SAFE_DELETE(outClip.allocator);
        return false;
    }

    ReleaseMeshContainersRecursive(outClip.frameRoot, *outClip.allocator);
    if (outClip.controller != nullptr)
    {
        outClip.duration = GetAnimationControllerDuration(outClip.controller);
    }
    return true;
}

HRESULT MeshMixAnimNoBone::LoadMeshHierarchy(const std::wstring& filePath,
                                              AnimNoBoneMeshAlloc& allocator,
                                              LPD3DXFRAME* outRoot,
                                              LPD3DXANIMATIONCONTROLLER* outController)
{
    return LoadMeshHierarchyWithDirectX(filePath, allocator, *outRoot, *outController);
}

HRESULT MeshMixAnimNoBone::LoadMeshHierarchyWithDirectX(const std::wstring& filePath,
                                                          AnimNoBoneMeshAlloc& allocator,
                                                          LPD3DXFRAME& outRoot,
                                                          LPD3DXANIMATIONCONTROLLER& outController)
{
    return D3DXLoadMeshHierarchyFromX(filePath.c_str(),
                                      D3DXMESH_MANAGED,
                                      Common::D3DDevice(),
                                      &allocator,
                                      nullptr,
                                      &outRoot,
                                      &outController);
}

HRESULT MeshMixAnimNoBone::LoadMeshHierarchyWithCustomLoader(const std::wstring&,
                                                               AnimNoBoneMeshAlloc&,
                                                               LPD3DXFRAME&)
{
    return E_NOTIMPL;
}

void MeshMixAnimNoBone::ReleaseMeshContainersRecursive(const LPD3DXFRAME frame,
                                                         AnimNoBoneMeshAlloc& allocator)
{
    if (frame == nullptr)
    {
        return;
    }

    LPD3DXFRAME mutableFrame = const_cast<LPD3DXFRAME>(frame);
    LPD3DXMESHCONTAINER container = mutableFrame->pMeshContainer;
    mutableFrame->pMeshContainer = nullptr;

    while (container != nullptr)
    {
        LPD3DXMESHCONTAINER nextContainer = container->pNextMeshContainer;
        container->pNextMeshContainer = nullptr;
        allocator.DestroyMeshContainer(container);
        container = nextContainer;
    }

    if (frame->pFrameSibling != nullptr)
    {
        ReleaseMeshContainersRecursive(frame->pFrameSibling, allocator);
    }

    if (frame->pFrameFirstChild != nullptr)
    {
        ReleaseMeshContainersRecursive(frame->pFrameFirstChild, allocator);
    }
}

HRESULT MeshMixAnimNoBone::AllocateAllBoneMatrix(LPD3DXFRAME) { return S_OK; }
HRESULT MeshMixAnimNoBone::AllocateBoneMatrix(LPD3DXMESHCONTAINER) { return S_OK; }
void MeshMixAnimNoBone::InvalidateBonePaletteCache() {}

void MeshMixAnimNoBone::ReleaseAnimationClips()
{
    for (auto& clip : m_animationClips)
    {
        SAFE_RELEASE(clip.controller);
        if (clip.frameRoot != nullptr && clip.allocator != nullptr)
        {
            ReleaseMeshAllocatorRecursive(clip.frameRoot, *clip.allocator);
            clip.frameRoot = nullptr;
        }
        SAFE_DELETE(clip.allocator);
    }

    m_animationClips.clear();
    m_animationInfoList.clear();
    m_activeAnimationClipIndex = -1;
}
}
