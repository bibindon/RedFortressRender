#include "MeshMixAnimNoBone.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <exception>
#include <fstream>
#include <string>
#include "Camera.h"
#include "Common.h"
#include "Light.h"
#include "Util.h"

namespace NSRender
{

MeshMixAnimNoBone::MeshMixAnimNoBone(const std::wstring& filename,
    const D3DXVECTOR3& pos, const D3DXVECTOR3& rotate, const float scale,
    const stMeshParam& param, const AnimSetMap& animSetMap,
    const MeshMixSkinAnimLoadMode loadMode)
    : m_meshName(filename), m_allocator(filename), m_animationAllocator(L""),
      m_pos(pos), m_rotate(rotate), m_scale(scale), m_param(param),
      m_animSetMap(animSetMap), m_loadMode(loadMode) {}

MeshMixAnimNoBone::~MeshMixAnimNoBone()
{
    ReleaseMeshAllocatorRecursive(m_frameRoot, m_allocator);
    ReleaseMeshAllocatorRecursive(m_animationFrameRoot, m_animationAllocator);
    SAFE_RELEASE(m_D3DEffect);
    Common::RemoveDeviceLostResource(this);
}

void MeshMixAnimNoBone::Initialize(bool) { m_loadThread = std::thread([this]() {
    HRESULT hr; 
    std::wstring path = Util::GetExeDir() + SHADER_FILENAME;
    hr = D3DXCreateEffectFromFile(Common::D3DDevice(), path.c_str(), nullptr, nullptr, 0, nullptr, &m_D3DEffect, nullptr);
    if (FAILED(hr)) throw std::exception("Failed to create effect");
    hr = D3DXLoadMeshHierarchyFromX(m_meshName.c_str(), D3DXMESH_MANAGED, Common::D3DDevice(),
        &m_allocator, nullptr, &m_frameRoot, nullptr);
    if (FAILED(hr)) throw std::exception("Failed to load mesh");
    m_bLoaded = true;
}); WaitForLoad(); }

void MeshMixAnimNoBone::WaitForLoad() { if (m_loadThread.joinable()) m_loadThread.join(); }

void MeshMixAnimNoBone::UpdateAnimation()
{
    if (!m_bLoaded) return;
    D3DXMATRIX w = BuildWorldMatrix();
    UpdateFrameMatrix(m_frameRoot, &w);
}

void MeshMixAnimNoBone::Render()
{
    if (!m_bLoaded || !m_enabled || !m_D3DEffect) return;
    const D3DXVECTOR4 lightDir = Light::GetLightDir();
    m_D3DEffect->SetVector("g_lightDir", &lightDir);
    D3DXVECTOR4 lc(Light::GetLightColor()); m_D3DEffect->SetVector("g_lightColor", &lc);
    m_D3DEffect->SetFloat("g_fSunLightIntensity", Light::GetBrightness());
    m_D3DEffect->SetFloat("g_fAmbientIntensity", Light::GetAmbientBrightness());
    D3DXVECTOR4 cp(Camera::GetEyePos(), 1.0f); m_D3DEffect->SetVector("g_cameraPos", &cp);
    m_D3DEffect->SetBool("g_treatTextureAsWhite", (m_damageFlash||m_yellowFlash)?TRUE:FALSE);
    m_D3DEffect->SetBool("g_damageFlash", m_damageFlash?TRUE:FALSE);
    m_D3DEffect->SetBool("g_yellowFlash", m_yellowFlash?TRUE:FALSE);
    m_D3DEffect->SetBool("g_alphaClipEnabled", m_alphaClipEnabled?TRUE:FALSE);
    m_D3DEffect->SetTechnique("TechniqueNoSkin");
    m_D3DEffect->Begin(nullptr, 0);
    RenderFrameHierarchy(m_frameRoot, m_D3DEffect);
    m_D3DEffect->End();
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
        LPD3DXMESH m = c->MeshData.pMesh;
        DWORD ns = c->NumMaterials; if (ns == 0) ns = 1;
        for (DWORD i = 0; i < ns; ++i) {
            if (i < c->m_textureList.size() && c->m_textureList[i])
                e->SetTexture("g_textureSampler", c->m_textureList[i]);
            D3DXVECTOR4 d(1,1,1,1);
            if (i < c->NumMaterials) {
                const D3DMATERIAL9& mat = c->pMaterials[i].MatD3D;
                d = D3DXVECTOR4(mat.Diffuse.r, mat.Diffuse.g, mat.Diffuse.b, mat.Diffuse.a);
            }
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
    f->m_combinedMatrix = mp ? f->TransformationMatrix * (*mp) : f->TransformationMatrix;
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
bool MeshMixAnimNoBone::PlayAnimation(const std::wstring&) { return false; }
void MeshMixAnimNoBone::OnDeviceLost() { if (m_D3DEffect) m_D3DEffect->OnLostDevice(); }
void MeshMixAnimNoBone::OnDeviceReset() { if (m_D3DEffect) m_D3DEffect->OnResetDevice(); }

// Stubs for skin-compatibility
void MeshMixAnimNoBone::SetSharedMirrorClipPlane(bool, const D3DXVECTOR4&) {}
HRESULT MeshMixAnimNoBone::AllocateAllBoneMatrix(LPD3DXFRAME) { return S_OK; }
HRESULT MeshMixAnimNoBone::AllocateBoneMatrix(LPD3DXMESHCONTAINER) { return S_OK; }
void MeshMixAnimNoBone::InvalidateBonePaletteCache() {}
bool MeshMixAnimNoBone::LoadAnimationCsv() { return true; }
bool MeshMixAnimNoBone::LoadAnimationClip(const std::wstring&, AnimationClip&) { return false; }
HRESULT MeshMixAnimNoBone::LoadMeshHierarchy(const std::wstring&, AnimNoBoneMeshAlloc&, LPD3DXFRAME*, LPD3DXANIMATIONCONTROLLER*) { return E_NOTIMPL; }
HRESULT MeshMixAnimNoBone::LoadMeshHierarchyWithDirectX(const std::wstring&, AnimNoBoneMeshAlloc&, LPD3DXFRAME&, LPD3DXANIMATIONCONTROLLER&) { return E_NOTIMPL; }
HRESULT MeshMixAnimNoBone::LoadMeshHierarchyWithCustomLoader(const std::wstring&, AnimNoBoneMeshAlloc&, LPD3DXFRAME&) { return E_NOTIMPL; }
void MeshMixAnimNoBone::UpdateActiveAnimationClip() {}

}
