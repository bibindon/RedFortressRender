#include "MeshMixNoSkinAnim.h"
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

MeshMixNoSkinAnim::MeshMixNoSkinAnim(const std::wstring& filename,
    const D3DXVECTOR3& pos, const D3DXVECTOR3& rotate, const float scale,
    const stMeshParam& param, const AnimSetMap& animSetMap,
    const MeshMixSkinAnimLoadMode loadMode)
    : m_meshName(filename), m_allocator(filename), m_animationAllocator(L""),
      m_pos(pos), m_rotate(rotate), m_scale(scale), m_param(param),
      m_animSetMap(animSetMap), m_loadMode(loadMode) {}

MeshMixNoSkinAnim::~MeshMixNoSkinAnim()
{
    ReleaseMeshAllocatorRecursive(m_frameRoot, m_allocator);
    ReleaseMeshAllocatorRecursive(m_animationFrameRoot, m_animationAllocator);
    SAFE_RELEASE(m_D3DEffect);
    Common::RemoveDeviceLostResource(this);
}

void MeshMixNoSkinAnim::Initialize(bool) { m_loadThread = std::thread([this]() {
    HRESULT hr; 
    std::wstring path = Util::GetExeDir() + SHADER_FILENAME;
    hr = D3DXCreateEffectFromFile(Common::D3DDevice(), path.c_str(), nullptr, nullptr, 0, nullptr, &m_D3DEffect, nullptr);
    if (FAILED(hr)) throw std::exception("Failed to create effect");
    hr = D3DXLoadMeshHierarchyFromX(m_meshName.c_str(), D3DXMESH_MANAGED, Common::D3DDevice(),
        &m_allocator, nullptr, &m_frameRoot, nullptr);
    if (FAILED(hr)) throw std::exception("Failed to load mesh");
    m_bLoaded = true;
}); WaitForLoad(); }

void MeshMixNoSkinAnim::WaitForLoad() { if (m_loadThread.joinable()) m_loadThread.join(); }

void MeshMixNoSkinAnim::UpdateAnimation()
{
    if (!m_bLoaded) return;
    D3DXMATRIX w = BuildWorldMatrix();
    UpdateFrameMatrix(m_frameRoot, &w);
}

void MeshMixNoSkinAnim::Render()
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

void MeshMixNoSkinAnim::RenderToEffect(LPD3DXEFFECT e)
{
    if (!m_bLoaded || !m_enabled) return;
    RenderFrameHierarchy(m_frameRoot, e);
}

void MeshMixNoSkinAnim::RenderFrameHierarchy(LPD3DXFRAME frame, LPD3DXEFFECT e)
{
    if (!frame) return;
    auto nf = reinterpret_cast<NoSkinAnimFrame*>(frame);
    if (frame->pMeshContainer) {
        auto c = reinterpret_cast<NoSkinAnimMeshContainer*>(frame->pMeshContainer);
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

void MeshMixNoSkinAnim::UpdateFrameMatrix(const LPD3DXFRAME fb, const LPD3DXMATRIX mp)
{
    auto f = reinterpret_cast<NoSkinAnimFrame*>(fb);
    if (!f) return;
    f->m_combinedMatrix = mp ? f->TransformationMatrix * (*mp) : f->TransformationMatrix;
    if (f->pFrameSibling) UpdateFrameMatrix(f->pFrameSibling, mp);
    if (f->pFrameFirstChild) UpdateFrameMatrix(f->pFrameFirstChild, &f->m_combinedMatrix);
}

D3DXMATRIX MeshMixNoSkinAnim::BuildWorldMatrix() const
{
    D3DXMATRIX w; D3DXMatrixIdentity(&w); D3DXMATRIX m;
    D3DXMatrixTranslation(&m, -m_centerPos.x, -m_centerPos.y, -m_centerPos.z); w *= m;
    D3DXMatrixScaling(&m, m_scale, m_scale, m_scale); w *= m;
    D3DXMatrixRotationYawPitchRoll(&m, m_rotate.y, m_rotate.x, m_rotate.z); w *= m;
    D3DXMatrixTranslation(&m, m_pos.x, m_pos.y, m_pos.z); w *= m;
    return w;
}

void MeshMixNoSkinAnim::ReleaseMeshAllocatorRecursive(LPD3DXFRAME f, NoSkinAnimMeshAlloc& a)
{
    if (!f) return;
    ReleaseMeshAllocatorRecursive(f->pFrameSibling, a);
    ReleaseMeshAllocatorRecursive(f->pFrameFirstChild, a);
    a.DestroyFrame(f);
}

// Simple setters / getters
void MeshMixNoSkinAnim::SetPos(const D3DXVECTOR3& p) { m_pos = p; }
void MeshMixNoSkinAnim::SetRotY(float r) { m_rotate.y = r; }
void MeshMixNoSkinAnim::SetScale(float s) { m_scale = s; }
void MeshMixNoSkinAnim::SetEnabled(bool e) { m_enabled = e; }
void MeshMixNoSkinAnim::SetAnimationSpeed(float s) { m_animationSpeed = s; }
void MeshMixNoSkinAnim::SetDamageFlash(bool v) { m_damageFlash = v; }
void MeshMixNoSkinAnim::SetYellowFlash(bool v) { m_yellowFlash = v; }
void MeshMixNoSkinAnim::SetAlphaClipEnabled(bool v) { m_alphaClipEnabled = v; }
void MeshMixNoSkinAnim::SetIgnoreTransparentMaterial(bool v) { m_ignoreTransparentMaterial = v; }
void MeshMixNoSkinAnim::SetTreatTextureAsWhite(bool v) { m_param.treatTextureAsWhite = v; }
void MeshMixNoSkinAnim::SetSaturateShadow(bool v) { m_param.saturateShadow = v; }
void MeshMixNoSkinAnim::SetSaturateShadowIntensity(float v) { m_param.saturateShadowIntensity = v; }
void MeshMixNoSkinAnim::SetShadowDarkness(float v) { m_param.shadowDarkness = v; }
void MeshMixNoSkinAnim::SetSpecularIntensity(float v) { m_param.specularIntensity = v; }
void MeshMixNoSkinAnim::SetSpecularEdge(float v) { m_param.specularEdge = v; }
void MeshMixNoSkinAnim::SetFresnelIntensity(float v) { m_param.fresnelIntensity = v; }
void MeshMixNoSkinAnim::SetSpecularIntensityOverrideEnabled(bool v) { m_param.specularIntensityOverrideEnabled = v; }
void MeshMixNoSkinAnim::SetSpecularEdgeOverrideEnabled(bool v) { m_param.specularEdgeOverrideEnabled = v; }
D3DXVECTOR3 MeshMixNoSkinAnim::GetPos() const { return m_pos; }
D3DXVECTOR3 MeshMixNoSkinAnim::GetRot() const { return m_rotate; }
float MeshMixNoSkinAnim::GetScale() const { return m_scale; }
bool MeshMixNoSkinAnim::IsEnabled() const { return m_enabled; }
bool MeshMixNoSkinAnim::IsLoaded() const { return m_bLoaded; }
std::wstring MeshMixNoSkinAnim::GetMeshName() const { return m_meshName; }
const std::vector<MeshMixNoSkinAnim::AnimationInfo>& MeshMixNoSkinAnim::GetAnimationInfoList() const { return m_animationInfoList; }
bool MeshMixNoSkinAnim::PlayAnimation(const std::wstring&) { return false; }
void MeshMixNoSkinAnim::OnDeviceLost() { if (m_D3DEffect) m_D3DEffect->OnLostDevice(); }
void MeshMixNoSkinAnim::OnDeviceReset() { if (m_D3DEffect) m_D3DEffect->OnResetDevice(); }

// Stubs for skin-compatibility
void MeshMixNoSkinAnim::SetSharedMirrorClipPlane(bool, const D3DXVECTOR4&) {}
HRESULT MeshMixNoSkinAnim::AllocateAllBoneMatrix(LPD3DXFRAME) { return S_OK; }
HRESULT MeshMixNoSkinAnim::AllocateBoneMatrix(LPD3DXMESHCONTAINER) { return S_OK; }
void MeshMixNoSkinAnim::InvalidateBonePaletteCache() {}
bool MeshMixNoSkinAnim::LoadAnimationCsv() { return true; }
bool MeshMixNoSkinAnim::LoadAnimationClip(const std::wstring&, AnimationClip&) { return false; }
HRESULT MeshMixNoSkinAnim::LoadMeshHierarchy(const std::wstring&, NoSkinAnimMeshAlloc&, LPD3DXFRAME*, LPD3DXANIMATIONCONTROLLER*) { return E_NOTIMPL; }
HRESULT MeshMixNoSkinAnim::LoadMeshHierarchyWithDirectX(const std::wstring&, NoSkinAnimMeshAlloc&, LPD3DXFRAME&, LPD3DXANIMATIONCONTROLLER&) { return E_NOTIMPL; }
HRESULT MeshMixNoSkinAnim::LoadMeshHierarchyWithCustomLoader(const std::wstring&, NoSkinAnimMeshAlloc&, LPD3DXFRAME&) { return E_NOTIMPL; }
void MeshMixNoSkinAnim::UpdateActiveAnimationClip() {}

}
