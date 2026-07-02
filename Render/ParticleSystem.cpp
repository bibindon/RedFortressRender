#include "ParticleSystem.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <stdexcept>
#include <string>

#include "Camera.h"
#include "Common.h"
#include "Util.h"

namespace
{
#define SAFE_RELEASE_LOCAL(p) { if ((p) != NULL) { (p)->Release(); (p) = NULL; } }
constexpr float kDustFixedScreenReferenceDistance = 6.0f;
constexpr float kDustFixedScreenMinScale = 0.15f;
constexpr float kDustFixedScreenMaxScale = 1.0f;
constexpr float kRainSpawnHalfWidth = 8.0f;
constexpr float kRainSpawnForward = 12.0f;
constexpr float kRainSpawnTopOffset = 5.0f;
constexpr float kRainKillBelowCamera = 4.0f;

std::string NarrowAscii(const std::wstring& wide)
{
    std::string narrow;
    narrow.reserve(wide.size());
    for (const wchar_t ch : wide)
    {
        narrow.push_back(static_cast<char>(ch));
    }
    return narrow;
}
}

namespace NSRender
{
void ParticleSystem::Initialize()
{
    if (m_initialized)
    {
        return;
    }

    if (m_effects.empty())
    {
        m_effects.resize(MAX_EFFECT_INSTANCES);
        for (auto& effect : m_effects)
        {
            effect.particles.resize(MAX_PARTICLES);
            ClearParticles(effect);
        }
    }

    if (m_vertices.empty())
    {
        m_vertices.resize(MAX_PARTICLES * PARTICLE_VERTEX_COUNT);
    }

    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    m_initialized = TryInitializeResources();
}

void ParticleSystem::Finalize()
{
    SAFE_RELEASE_LOCAL(m_smokeTexture);
    SAFE_RELEASE_LOCAL(m_fireTexture);
    SAFE_RELEASE_LOCAL(m_dustTexture);
    SAFE_RELEASE_LOCAL(m_dustTexture2);
    SAFE_RELEASE_LOCAL(m_fogTexture);
    SAFE_RELEASE_LOCAL(m_rainTexture);
    SAFE_RELEASE_LOCAL(m_damageCoreTexture);
    SAFE_RELEASE_LOCAL(m_damageRingTexture);
    SAFE_RELEASE_LOCAL(m_damageSpikeTexture);
    SAFE_RELEASE_LOCAL(m_effect);

    for (auto& effect : m_effects)
    {
        ClearParticles(effect);
        effect.preset = ParticleEffectPreset::None;
        effect.generation = 0;
    }

    m_effects.clear();
    m_vertices.clear();
    m_lastPlacedPreset = ParticleEffectPreset::None;
    m_nextGeneration = 1;
    m_initialized = false;
}

void ParticleSystem::OnDeviceLost()
{
    if (m_effect != NULL)
    {
        m_effect->OnLostDevice();
    }
}

void ParticleSystem::OnDeviceReset()
{
    if (m_effect != NULL)
    {
        m_effect->OnResetDevice();
    }
}

void ParticleSystem::PlaceEffect(const ParticleEffectPreset preset, const D3DXVECTOR3& origin)
{
    Initialize();
    if (!m_initialized || preset == ParticleEffectPreset::None)
    {
        return;
    }

    EffectInstance* target = nullptr;
    for (auto& effect : m_effects)
    {
        if (effect.preset == ParticleEffectPreset::None)
        {
            target = &effect;
            break;
        }
    }

    if (target == nullptr)
    {
        target = &m_effects.front();
        for (auto& effect : m_effects)
        {
            if (effect.generation < target->generation)
            {
                target = &effect;
            }
        }
    }

    target->preset = preset;
    target->origin = origin;
    target->generation = m_nextGeneration++;
    ClearParticles(*target);

    if (preset == ParticleEffectPreset::Fog)
    {
        EmitFog(*target, 2.0f);
    }
    else if (preset == ParticleEffectPreset::Dust)
    {
        EmitDust(*target, 3.0f);
    }
    else if (preset == ParticleEffectPreset::Rain)
    {
        target->origin = Camera::GetEyePos();
        EmitRain(*target, 2.0f);
    }
    else if (preset == ParticleEffectPreset::Explosion)
    {
        EmitExplosion(*target);
    }
    else if (preset == ParticleEffectPreset::Damage)
    {
        EmitDamage(*target);
    }

    m_lastPlacedPreset = preset;
}

void ParticleSystem::ClearEffect()
{
    for (auto& effect : m_effects)
    {
        ClearParticles(effect);
        effect.preset = ParticleEffectPreset::None;
        effect.generation = 0;
    }

    m_lastPlacedPreset = ParticleEffectPreset::None;
}

void ParticleSystem::SetDustFixedScreenSize(const bool enabled)
{
    m_dustFixedScreenSizeEnabled = enabled;
}

void ParticleSystem::SetExplosionScale(const float scale)
{
    m_explosionScale = scale;
}

float ParticleSystem::GetExplosionScale() const
{
    return m_explosionScale;
}

void ParticleSystem::Update(const float deltaTime)
{
    if (!m_initialized)
    {
        Initialize();
    }

    if (!m_initialized)
    {
        return;
    }

    const float dt = ClampFloat(deltaTime, 0.0f, 0.033f);
    for (auto& effect : m_effects)
    {
        UpdateEffect(effect, dt);
    }
}

void ParticleSystem::Draw(const D3DXMATRIX& view, const D3DXMATRIX& proj)
{
    if (!m_initialized)
    {
        Initialize();
    }

    if (!m_initialized || m_effect == NULL)
    {
        return;
    }

    for (const auto& effect : m_effects)
    {
        DrawEffect(effect, view, proj);
    }
}

int ParticleSystem::FillDustVertices(const EffectInstance& effectInstance,
                                     LPDIRECT3DTEXTURE9 batchTexture,
                                     const D3DXMATRIX& view)
{
    if (effectInstance.preset != ParticleEffectPreset::Dust || batchTexture == NULL)
    {
        return 0;
    }

    D3DXMATRIX invView;
    D3DXMatrixInverse(&invView, NULL, &view);
    const D3DXVECTOR3 cameraPos(invView._41, invView._42, invView._43);

    D3DXVECTOR3 cameraRight(invView._11, invView._12, invView._13);
    D3DXVECTOR3 cameraUp(invView._21, invView._22, invView._23);
    D3DXVec3Normalize(&cameraRight, &cameraRight);
    D3DXVec3Normalize(&cameraUp, &cameraUp);

    int activeCount = 0;

    for (const auto& particle : effectInstance.particles)
    {
        if (!particle.active)
        {
            continue;
        }

        const bool usesAltTexture = (batchTexture == m_dustTexture2);
        if (particle.useAltTexture != usesAltTexture)
        {
            continue;
        }

        D3DXVECTOR3 toParticle = particle.pos - cameraPos;
        if (!m_dustFixedScreenSizeEnabled)
        {
            if (D3DXVec3LengthSq(&toParticle) < 30.0f)
            {
                continue;
            }
        }

        const float cosValue = cosf(particle.rotation);
        const float sinValue = sinf(particle.rotation);
        float halfWidth = particle.size * 0.5f;
        float halfHeight = particle.size * 0.5f;
        float fixedScreenScale = 1.0f;

        if (m_dustFixedScreenSizeEnabled)
        {
            D3DXVECTOR3 viewPos;
            D3DXVec3TransformCoord(&viewPos, &particle.pos, &view);
            fixedScreenScale = ClampFloat(fabsf(viewPos.z) / kDustFixedScreenReferenceDistance,
                                          kDustFixedScreenMinScale,
                                          kDustFixedScreenMaxScale);
        }

        D3DXVECTOR3 rotatedRight;
        rotatedRight.x = cameraRight.x * cosValue + cameraUp.x * sinValue;
        rotatedRight.y = cameraRight.y * cosValue + cameraUp.y * sinValue;
        rotatedRight.z = cameraRight.z * cosValue + cameraUp.z * sinValue;

        D3DXVECTOR3 rotatedUp;
        rotatedUp.x = cameraUp.x * cosValue - cameraRight.x * sinValue;
        rotatedUp.y = cameraUp.y * cosValue - cameraRight.y * sinValue;
        rotatedUp.z = cameraUp.z * cosValue - cameraRight.z * sinValue;

        halfWidth *= 0.50f * fixedScreenScale;
        halfHeight *= 0.50f * fixedScreenScale;

        const D3DXVECTOR3 halfRight(rotatedRight.x * halfWidth,
                                    rotatedRight.y * halfWidth,
                                    rotatedRight.z * halfWidth);
        const D3DXVECTOR3 halfUp(rotatedUp.x * halfHeight,
                                 rotatedUp.y * halfHeight,
                                 rotatedUp.z * halfHeight);

        const D3DXVECTOR3 topLeft(particle.pos.x - halfRight.x + halfUp.x,
                                  particle.pos.y - halfRight.y + halfUp.y,
                                  particle.pos.z - halfRight.z + halfUp.z);
        const D3DXVECTOR3 topRight(particle.pos.x + halfRight.x + halfUp.x,
                                   particle.pos.y + halfRight.y + halfUp.y,
                                   particle.pos.z + halfRight.z + halfUp.z);
        const D3DXVECTOR3 bottomLeft(particle.pos.x - halfRight.x - halfUp.x,
                                     particle.pos.y - halfRight.y - halfUp.y,
                                     particle.pos.z - halfRight.z - halfUp.z);
        const D3DXVECTOR3 bottomRight(particle.pos.x + halfRight.x - halfUp.x,
                                      particle.pos.y + halfRight.y - halfUp.y,
                                      particle.pos.z + halfRight.z - halfUp.z);

        const int vertexIndex = activeCount * PARTICLE_VERTEX_COUNT;
        m_vertices[vertexIndex + 0] = { topLeft, particle.color, 0.0f, 0.0f };
        m_vertices[vertexIndex + 1] = { topRight, particle.color, 1.0f, 0.0f };
        m_vertices[vertexIndex + 2] = { bottomLeft, particle.color, 0.0f, 1.0f };
        m_vertices[vertexIndex + 3] = { bottomLeft, particle.color, 0.0f, 1.0f };
        m_vertices[vertexIndex + 4] = { topRight, particle.color, 1.0f, 0.0f };
        m_vertices[vertexIndex + 5] = { bottomRight, particle.color, 1.0f, 1.0f };
        ++activeCount;
    }

    return activeCount;
}

void ParticleSystem::RenderDustToGBufferEffect(LPD3DXEFFECT effect,
                                               const D3DXMATRIX& view,
                                               const D3DXMATRIX& proj,
                                               const char* techniqueName)
{
    if (!m_initialized || effect == NULL || techniqueName == NULL)
    {
        return;
    }

    D3DXMATRIX worldViewProj = view * proj;
    HRESULT hResult = effect->SetMatrix("g_matWorldViewProjParticle", &worldViewProj);
    assert(SUCCEEDED(hResult));

    for (const auto& effectInstance : m_effects)
    {
        if (effectInstance.preset != ParticleEffectPreset::Dust)
        {
            continue;
        }

        const LPDIRECT3DTEXTURE9 dustTextures[] = { m_dustTexture, m_dustTexture2 };
        for (const auto& dustTexture : dustTextures)
        {
            const int activeCount = FillDustVertices(effectInstance, dustTexture, view);
            if (activeCount <= 0)
            {
                continue;
            }

            hResult = effect->SetTechnique(techniqueName);
            assert(SUCCEEDED(hResult));

            hResult = effect->SetTexture("g_texParticleAlpha", dustTexture);
            assert(SUCCEEDED(hResult));

            UINT numPasses = 0;
            hResult = effect->Begin(&numPasses, 0);
            assert(SUCCEEDED(hResult));

            for (UINT passIndex = 0; passIndex < numPasses; ++passIndex)
            {
                hResult = effect->BeginPass(passIndex);
                assert(SUCCEEDED(hResult));

                hResult = effect->CommitChanges();
                assert(SUCCEEDED(hResult));

                hResult = Common::D3DDevice()->SetFVF(ParticleVertex::FVF);
                assert(SUCCEEDED(hResult));

                hResult = Common::D3DDevice()->DrawPrimitiveUP(D3DPT_TRIANGLELIST,
                                                               activeCount * 2,
                                                               m_vertices.data(),
                                                               sizeof(ParticleVertex));
                assert(SUCCEEDED(hResult));

                hResult = effect->EndPass();
                assert(SUCCEEDED(hResult));
            }

            hResult = effect->End();
            assert(SUCCEEDED(hResult));
        }
    }
}

ParticleEffectPreset ParticleSystem::GetPreset() const
{
    return m_lastPlacedPreset;
}

bool ParticleSystem::TryInitializeResources()
{
    if (m_effect != NULL)
    {
        return true;
    }

    const std::wstring smokePath = BuildAssetPath(L"particle_smoke.png");
    const std::wstring firePath = BuildAssetPath(L"particle_fire.png");
    const std::wstring dustPath = BuildAssetPath(L"particle_dust.png");
    const std::wstring dustPath2 = BuildAssetPath(L"particle_dust2.png");
    const std::wstring fogPath = BuildAssetPath(L"particle_fog.png");
    const std::wstring rainPath = BuildAssetPath(L"particle_rain.png");
    const std::wstring damageCorePath = BuildAssetPath(L"particle_damage_core.png");
    const std::wstring damageRingPath = BuildAssetPath(L"particle_damage_ring.png");
    const std::wstring damageSpikePath = BuildAssetPath(L"particle_damage_spike.png");
    const std::wstring effectPath = Util::GetExeDir() + L"Particle.cso";

    HRESULT hr = D3DXCreateTextureFromFile(Common::D3DDevice(), smokePath.c_str(), &m_smokeTexture);
    if (FAILED(hr))
    {
        throw std::runtime_error("ParticleSystem: failed to load texture 'particle_smoke.png' [" +
                                 NarrowAscii(smokePath) + "] HRESULT=" + std::to_string(hr));
    }

    hr = D3DXCreateTextureFromFile(Common::D3DDevice(), firePath.c_str(), &m_fireTexture);
    if (FAILED(hr))
    {
        throw std::runtime_error("ParticleSystem: failed to load texture 'particle_fire.png' [" +
                                 NarrowAscii(firePath) + "] HRESULT=" + std::to_string(hr));
    }

    hr = D3DXCreateTextureFromFile(Common::D3DDevice(), dustPath.c_str(), &m_dustTexture);
    if (FAILED(hr))
    {
        throw std::runtime_error("ParticleSystem: failed to load texture 'particle_dust.png' [" +
                                 NarrowAscii(dustPath) + "] HRESULT=" + std::to_string(hr));
    }

    hr = D3DXCreateTextureFromFile(Common::D3DDevice(), dustPath2.c_str(), &m_dustTexture2);
    if (FAILED(hr))
    {
        throw std::runtime_error("ParticleSystem: failed to load texture 'particle_dust2.png' [" +
                                 NarrowAscii(dustPath2) + "] HRESULT=" + std::to_string(hr));
    }

    hr = D3DXCreateTextureFromFile(Common::D3DDevice(), fogPath.c_str(), &m_fogTexture);
    if (FAILED(hr))
    {
        throw std::runtime_error("ParticleSystem: failed to load texture 'particle_fog.png' [" +
                                 NarrowAscii(fogPath) + "] HRESULT=" + std::to_string(hr));
    }

    hr = D3DXCreateTextureFromFile(Common::D3DDevice(), rainPath.c_str(), &m_rainTexture);
    if (FAILED(hr))
    {
        throw std::runtime_error("ParticleSystem: failed to load texture 'particle_rain.png' [" +
                                 NarrowAscii(rainPath) + "] HRESULT=" + std::to_string(hr));
    }

    hr = D3DXCreateTextureFromFile(Common::D3DDevice(), damageCorePath.c_str(), &m_damageCoreTexture);
    if (FAILED(hr))
    {
        throw std::runtime_error("ParticleSystem: failed to load texture 'particle_damage_core.png' [" +
                                 NarrowAscii(damageCorePath) + "] HRESULT=" + std::to_string(hr));
    }

    hr = D3DXCreateTextureFromFile(Common::D3DDevice(), damageRingPath.c_str(), &m_damageRingTexture);
    if (FAILED(hr))
    {
        throw std::runtime_error("ParticleSystem: failed to load texture 'particle_damage_ring.png' [" +
                                 NarrowAscii(damageRingPath) + "] HRESULT=" + std::to_string(hr));
    }

    hr = D3DXCreateTextureFromFile(Common::D3DDevice(), damageSpikePath.c_str(), &m_damageSpikeTexture);
    if (FAILED(hr))
    {
        throw std::runtime_error("ParticleSystem: failed to load texture 'particle_damage_spike.png' [" +
                                 NarrowAscii(damageSpikePath) + "] HRESULT=" + std::to_string(hr));
    }

    LPD3DXBUFFER compilationErrors = NULL;
    hr = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                  effectPath.c_str(),
                                  NULL,
                                  NULL,
                                  D3DXSHADER_DEBUG,
                                  NULL,
                                  &m_effect,
                                  &compilationErrors);
    if (FAILED(hr))
    {
        std::string message = "ParticleSystem: failed to load effect 'Particle.cso' [" +
                              NarrowAscii(effectPath) + "] HRESULT=" + std::to_string(hr);
        if (compilationErrors != NULL)
        {
            message += " - ";
            message += static_cast<const char*>(compilationErrors->GetBufferPointer());
            compilationErrors->Release();
            compilationErrors = NULL;
        }
        throw std::runtime_error(message);
    }

    return true;
}

void ParticleSystem::ClearParticles(EffectInstance& effect)
{
    effect.smokeEmitAccumulator = 0.0f;
    effect.fireEmitAccumulator = 0.0f;
    effect.fireSparkAccumulator = 0.0f;
    effect.fogEmitAccumulator = 0.0f;
    effect.dustEmitAccumulator = 0.0f;
    effect.rainEmitAccumulator = 0.0f;

    for (auto& particle : effect.particles)
    {
        particle.active = false;
        particle.life = 0.0f;
    }
}

void ParticleSystem::SpawnParticle(EffectInstance& effect,
                                   const D3DXVECTOR3& pos,
                                   const D3DXVECTOR3& velocity,
                                   const float maxLife,
                                   const float startSize,
                                   const float endSize,
                                   const float rotation,
                                   const float rotationSpeed,
                                   const D3DCOLOR color,
                                   const ParticleVisualType visualType)
{
    for (auto& particle : effect.particles)
    {
        if (particle.active)
        {
            continue;
        }

        particle.pos = pos;
        particle.velocity = velocity;
        particle.life = 0.0f;
        particle.maxLife = maxLife;
        particle.size = startSize;
        particle.startSize = startSize;
        particle.endSize = endSize;
        particle.rotation = rotation;
        particle.rotationSpeed = rotationSpeed;
        particle.basePos = pos;
        particle.swayPhase = RandomFloat(0.0f, D3DX_PI * 2.0f);
        particle.randomScale = RandomFloat(0.80f, 1.35f);
        particle.alphaBias = RandomFloat(0.65f, 1.35f);
        particle.useAltTexture = RandomFloat(0.0f, 1.0f) >= 0.5f;
        particle.visualType = visualType;
        particle.color = color;
        particle.active = true;
        return;
    }
}

void ParticleSystem::EmitSmoke(EffectInstance& effect, const float deltaTime)
{
    effect.smokeEmitAccumulator += deltaTime * 34.0f;

    while (effect.smokeEmitAccumulator >= 1.0f)
    {
        const D3DXVECTOR3 pos(effect.origin.x + RandomFloat(-0.45f, 0.45f),
                              effect.origin.y + RandomFloat(0.45f, 0.75f),
                              effect.origin.z + RandomFloat(-0.45f, 0.45f));
        const D3DXVECTOR3 velocity(RandomFloat(-0.18f, 0.18f),
                                   RandomFloat(0.52f, 0.90f),
                                   RandomFloat(-0.18f, 0.18f));
        const float life = RandomFloat(2.8f, 4.2f);
        const float startSize = RandomFloat(0.55f, 0.90f);
        const float endSize = startSize * RandomFloat(2.2f, 3.2f);
        const int gray = static_cast<int>(RandomFloat(90.0f, 150.0f));
        const D3DCOLOR color = D3DCOLOR_ARGB(static_cast<int>(RandomFloat(110.0f, 165.0f)),
                                             gray,
                                             gray,
                                             gray);

        SpawnParticle(effect,
                      pos,
                      velocity,
                      life,
                      startSize,
                      endSize,
                      RandomFloat(0.0f, D3DX_PI * 2.0f),
                      RandomFloat(-0.45f, 0.45f),
                      color);

        effect.smokeEmitAccumulator -= 1.0f;
    }
}

void ParticleSystem::EmitFire(EffectInstance& effect, const float deltaTime)
{
    effect.fireEmitAccumulator += deltaTime * 46.0f;
    effect.fireSparkAccumulator += deltaTime * 8.0f;

    while (effect.fireEmitAccumulator >= 1.0f)
    {
        const D3DXVECTOR3 pos(effect.origin.x + RandomFloat(-0.16f, 0.16f),
                              effect.origin.y + RandomFloat(1.02f, 1.14f),
                              effect.origin.z + RandomFloat(-0.16f, 0.16f));
        const D3DXVECTOR3 velocity(RandomFloat(-0.08f, 0.08f),
                                   RandomFloat(1.35f, 2.25f),
                                   RandomFloat(-0.08f, 0.08f));
        const float life = RandomFloat(0.42f, 0.72f);
        const float startSize = RandomFloat(0.34f, 0.52f);
        const float endSize = startSize * RandomFloat(1.35f, 1.85f);
        const D3DCOLOR color = D3DCOLOR_ARGB(static_cast<int>(RandomFloat(130.0f, 185.0f)),
                                             255,
                                             210,
                                             155);

        SpawnParticle(effect,
                      pos,
                      velocity,
                      life,
                      startSize,
                      endSize,
                      RandomFloat(-0.12f, 0.12f),
                      RandomFloat(-0.55f, 0.55f),
                      color);

        effect.fireEmitAccumulator -= 1.0f;
    }

    while (effect.fireSparkAccumulator >= 1.0f)
    {
        const D3DXVECTOR3 pos(effect.origin.x + RandomFloat(-0.10f, 0.10f),
                              effect.origin.y + RandomFloat(1.12f, 1.22f),
                              effect.origin.z + RandomFloat(-0.10f, 0.10f));
        const D3DXVECTOR3 velocity(RandomFloat(-0.25f, 0.25f),
                                   RandomFloat(1.8f, 3.0f),
                                   RandomFloat(-0.25f, 0.25f));
        const float life = RandomFloat(0.18f, 0.32f);
        const float startSize = RandomFloat(0.045f, 0.080f);
        const float endSize = startSize * RandomFloat(0.55f, 0.95f);
        const D3DCOLOR color = D3DCOLOR_ARGB(static_cast<int>(RandomFloat(90.0f, 150.0f)),
                                             255,
                                             static_cast<int>(RandomFloat(160.0f, 210.0f)),
                                             80);

        SpawnParticle(effect,
                      pos,
                      velocity,
                      life,
                      startSize,
                      endSize,
                      RandomFloat(0.0f, D3DX_PI * 2.0f),
                      RandomFloat(-3.5f, 3.5f),
                      color);

        effect.fireSparkAccumulator -= 1.0f;
    }
}

void ParticleSystem::EmitFog(EffectInstance& effect, const float deltaTime)
{
    constexpr int targetFogCount = 96;
    int activeFogCount = 0;

    for (const auto& particle : effect.particles)
    {
        if (particle.active)
        {
            ++activeFogCount;
        }
    }

    if (activeFogCount >= targetFogCount)
    {
        return;
    }

    effect.fogEmitAccumulator += deltaTime * 72.0f;

    while (effect.fogEmitAccumulator >= 1.0f && activeFogCount < targetFogCount)
    {
        const D3DXVECTOR3 pos(effect.origin.x + RandomCenteredFloat(5.0f),
                              effect.origin.y + RandomCenteredFloat(1.8f) + 1.0f,
                              effect.origin.z + RandomCenteredFloat(5.0f));
        const D3DXVECTOR3 velocity(RandomFloat(0.015f, 0.090f),
                                   RandomFloat(-0.035f, 0.035f),
                                   RandomFloat(-0.045f, 0.045f));
        const float life = RandomFloat(7.0f, 13.0f);
        const float startSize = RandomFloat(3.0f, 5.0f);
        const float endSize = startSize * RandomFloat(1.18f, 1.52f);
        const int gray = static_cast<int>(RandomFloat(205.0f, 224.0f));
        const D3DCOLOR color = D3DCOLOR_ARGB(static_cast<int>(RandomFloat(16.0f, 30.0f)),
                                             gray,
                                             gray,
                                             gray);

        SpawnParticle(effect,
                      pos,
                      velocity,
                      life,
                      startSize,
                      endSize,
                      RandomFloat(0.0f, D3DX_PI * 2.0f),
                      RandomFloat(-0.100f, 0.100f),
                      color);

        ++activeFogCount;
        effect.fogEmitAccumulator -= 1.0f;
    }
}

void ParticleSystem::EmitDust(EffectInstance& effect, const float deltaTime)
{
    constexpr int targetDustCount = 140;
    int activeDustCount = 0;

    for (const auto& particle : effect.particles)
    {
        if (particle.active)
        {
            ++activeDustCount;
        }
    }

    if (activeDustCount >= targetDustCount)
    {
        return;
    }

    effect.dustEmitAccumulator += deltaTime * 110.0f;

    while (effect.dustEmitAccumulator >= 1.0f && activeDustCount < targetDustCount)
    {
        const D3DXVECTOR3 pos(effect.origin.x + RandomCenteredFloat(4.8f),
                              effect.origin.y + RandomFloat(0.3f, 3.8f),
                              effect.origin.z + RandomCenteredFloat(4.8f));
        const D3DXVECTOR3 velocity(RandomFloat(0.004f, 0.018f),
                                   RandomFloat(-0.006f, 0.006f),
                                   RandomFloat(-0.010f, 0.010f));
        const float life = RandomFloat(8.0f, 15.0f);
        float startSize = 0.f;
        if (true)
        {
            startSize = RandomFloat(0.01f, 0.02f);
        }
        else
        {
            static float temp = 0.01f;
            startSize = temp;
        }
        const float endSize = startSize * RandomFloat(0.88f, 1.22f);
        const int alpha = static_cast<int>(RandomFloat(16.0f, 58.0f));
        const int gray = static_cast<int>(RandomFloat(220.0f, 255.0f));
        const D3DCOLOR color = D3DCOLOR_ARGB(alpha, gray, gray, gray);

        SpawnParticle(effect,
                      pos,
                      velocity,
                      life,
                      startSize,
                      endSize,
                      RandomFloat(0.0f, D3DX_PI * 2.0f),
                      RandomFloat(-0.18f, 0.18f),
                      color);

        ++activeDustCount;
        effect.dustEmitAccumulator -= 1.0f;
    }
}

void ParticleSystem::EmitRain(EffectInstance& effect, const float deltaTime)
{
    constexpr int targetRainCount = 520;
    int activeRainCount = 0;

    const D3DXVECTOR3 cameraPos = Camera::GetEyePos();
    const D3DXVECTOR3 cameraDelta = cameraPos - effect.origin;
    effect.origin = cameraPos;

    D3DXVECTOR3 horizontalForward = Camera::GetLookAtPos() - cameraPos;
    horizontalForward.y = 0.0f;
    if (D3DXVec3LengthSq(&horizontalForward) <= 0.0001f)
    {
        horizontalForward = D3DXVECTOR3(0.0f, 0.0f, 1.0f);
    }
    else
    {
        D3DXVec3Normalize(&horizontalForward, &horizontalForward);
    }

    const D3DXVECTOR3 worldUp(0.0f, 1.0f, 0.0f);
    D3DXVECTOR3 horizontalRight;
    D3DXVec3Cross(&horizontalRight, &worldUp, &horizontalForward);
    if (D3DXVec3LengthSq(&horizontalRight) <= 0.0001f)
    {
        horizontalRight = D3DXVECTOR3(1.0f, 0.0f, 0.0f);
    }
    else
    {
        D3DXVec3Normalize(&horizontalRight, &horizontalRight);
    }

    for (auto& particle : effect.particles)
    {
        if (!particle.active)
        {
            continue;
        }

        particle.pos += cameraDelta;
        particle.basePos += cameraDelta;

        if (particle.pos.y < cameraPos.y - kRainKillBelowCamera)
        {
            particle.active = false;
            continue;
        }

        ++activeRainCount;
    }

    if (activeRainCount >= targetRainCount)
    {
        return;
    }

    effect.rainEmitAccumulator += deltaTime * 760.0f;

    while (effect.rainEmitAccumulator >= 1.0f && activeRainCount < targetRainCount)
    {
        const float side = RandomCenteredFloat(kRainSpawnHalfWidth);
        const float depth = RandomFloat(-3.0f, kRainSpawnForward);
        const D3DXVECTOR3 pos = cameraPos +
                                horizontalRight * side +
                                horizontalForward * depth +
                                D3DXVECTOR3(0.0f, RandomFloat(1.2f, kRainSpawnTopOffset), 0.0f);
        const D3DXVECTOR3 velocity(RandomFloat(-0.25f, 0.25f),
                                   RandomFloat(-15.0f, -10.5f),
                                   RandomFloat(-0.20f, 0.20f));
        const float life = RandomFloat(0.75f, 1.15f);
        const float startSize = RandomFloat(0.020f, 0.040f);
        const float endSize = startSize;
        const int alpha = static_cast<int>(RandomFloat(95.0f, 150.0f));
        const int gray = static_cast<int>(RandomFloat(198.0f, 232.0f));
        const D3DCOLOR color = D3DCOLOR_ARGB(alpha, gray, gray, 255);

        SpawnParticle(effect,
                      pos,
                      velocity,
                      life,
                      startSize,
                      endSize,
                      0.0f,
                      0.0f,
                      color);

        ++activeRainCount;
        effect.rainEmitAccumulator -= 1.0f;
    }
}

void ParticleSystem::EmitExplosion(EffectInstance& effect)
{
    const float scale = m_explosionScale;

    for (int i = 0; i < 54; ++i)
    {
        D3DXVECTOR3 direction(RandomCenteredFloat(1.0f),
                              RandomFloat(0.05f, 1.0f),
                              RandomCenteredFloat(1.0f));
        if (D3DXVec3LengthSq(&direction) <= 0.0001f)
        {
            direction = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
        }
        else
        {
            D3DXVec3Normalize(&direction, &direction);
        }

        const float speed = RandomFloat(3.0f, 7.0f) * scale;
        const D3DXVECTOR3 pos(effect.origin.x + direction.x * RandomFloat(0.05f, 0.24f) * scale,
                              effect.origin.y + 0.35f * scale + direction.y * RandomFloat(0.04f, 0.28f) * scale,
                              effect.origin.z + direction.z * RandomFloat(0.05f, 0.24f) * scale);
        const D3DXVECTOR3 velocity(direction.x * speed,
                                   direction.y * speed + RandomFloat(0.8f, 2.4f),
                                   direction.z * speed);
        const float life = RandomFloat(0.22f, 0.52f);
        const float startSize = RandomFloat(0.40f, 0.85f) * scale;
        const float endSize = startSize * RandomFloat(1.4f, 2.4f);
        const D3DCOLOR color = D3DCOLOR_ARGB(static_cast<int>(RandomFloat(185.0f, 245.0f)),
                                             255,
                                             static_cast<int>(RandomFloat(155.0f, 230.0f)),
                                             static_cast<int>(RandomFloat(45.0f, 105.0f)));

        SpawnParticle(effect,
                      pos,
                      velocity,
                      life,
                      startSize,
                      endSize,
                      RandomFloat(0.0f, D3DX_PI * 2.0f),
                      RandomFloat(-4.5f, 4.5f),
                      color,
                      ParticleVisualType::ExplosionFire);
    }

    for (int i = 0; i < 38; ++i)
    {
        D3DXVECTOR3 direction(RandomCenteredFloat(1.0f),
                              RandomFloat(0.02f, 0.75f),
                              RandomCenteredFloat(1.0f));
        if (D3DXVec3LengthSq(&direction) <= 0.0001f)
        {
            direction = D3DXVECTOR3(1.0f, 0.0f, 0.0f);
        }
        else
        {
            D3DXVec3Normalize(&direction, &direction);
        }

        const float speed = RandomFloat(5.0f, 10.0f) * scale;
        const D3DXVECTOR3 pos(effect.origin.x,
                              effect.origin.y + RandomFloat(0.35f, 0.75f) * scale,
                              effect.origin.z);
        const D3DXVECTOR3 velocity(direction.x * speed,
                                   direction.y * speed + RandomFloat(0.8f, 2.0f),
                                   direction.z * speed);
        const float life = RandomFloat(0.35f, 0.95f);
        const float startSize = RandomFloat(0.045f, 0.095f) * scale;
        const float endSize = startSize * RandomFloat(0.45f, 0.85f);
        const D3DCOLOR color = D3DCOLOR_ARGB(static_cast<int>(RandomFloat(150.0f, 230.0f)),
                                             255,
                                             static_cast<int>(RandomFloat(185.0f, 235.0f)),
                                             95);

        SpawnParticle(effect,
                      pos,
                      velocity,
                      life,
                      startSize,
                      endSize,
                      RandomFloat(0.0f, D3DX_PI * 2.0f),
                      RandomFloat(-10.0f, 10.0f),
                      color,
                      ParticleVisualType::ExplosionSpark);
    }

    for (int i = 0; i < 70; ++i)
    {
        D3DXVECTOR3 direction(RandomCenteredFloat(1.0f),
                              RandomFloat(0.18f, 1.0f),
                              RandomCenteredFloat(1.0f));
        if (D3DXVec3LengthSq(&direction) <= 0.0001f)
        {
            direction = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
        }
        else
        {
            D3DXVec3Normalize(&direction, &direction);
        }

        const float speed = RandomFloat(0.7f, 2.6f) * scale;
        const D3DXVECTOR3 pos(effect.origin.x + RandomCenteredFloat(0.45f) * scale,
                              effect.origin.y + RandomFloat(0.30f, 0.95f) * scale,
                              effect.origin.z + RandomCenteredFloat(0.45f) * scale);
        const D3DXVECTOR3 velocity(direction.x * speed,
                                   direction.y * speed + RandomFloat(0.4f, 1.2f),
                                   direction.z * speed);
        const float life = RandomFloat(1.4f, 3.2f);
        const float startSize = RandomFloat(0.55f, 1.15f) * scale;
        const float endSize = startSize * RandomFloat(2.0f, 3.6f);
        const int gray = static_cast<int>(RandomFloat(72.0f, 132.0f));
        const D3DCOLOR color = D3DCOLOR_ARGB(static_cast<int>(RandomFloat(85.0f, 140.0f)),
                                             gray,
                                             gray,
                                             gray);

        SpawnParticle(effect,
                      pos,
                      velocity,
                      life,
                      startSize,
                      endSize,
                      RandomFloat(0.0f, D3DX_PI * 2.0f),
                      RandomFloat(-0.75f, 0.75f),
                      color,
                      ParticleVisualType::ExplosionSmoke);
    }

    for (int i = 0; i < 46; ++i)
    {
        D3DXVECTOR3 direction(RandomCenteredFloat(1.0f),
                              RandomFloat(-0.05f, 0.20f),
                              RandomCenteredFloat(1.0f));
        if (D3DXVec3LengthSq(&direction) <= 0.0001f)
        {
            direction = D3DXVECTOR3(1.0f, 0.0f, 0.0f);
        }
        else
        {
            D3DXVec3Normalize(&direction, &direction);
        }

        const float speed = RandomFloat(1.2f, 4.4f) * scale;
        const D3DXVECTOR3 pos(effect.origin.x + RandomCenteredFloat(0.35f) * scale,
                              effect.origin.y + RandomFloat(0.04f, 0.22f) * scale,
                              effect.origin.z + RandomCenteredFloat(0.35f) * scale);
        const D3DXVECTOR3 velocity(direction.x * speed,
                                   RandomFloat(0.04f, 0.38f),
                                   direction.z * speed);
        const float life = RandomFloat(0.85f, 1.9f);
        const float startSize = RandomFloat(0.25f, 0.62f) * scale;
        const float endSize = startSize * RandomFloat(2.0f, 3.0f);
        const int gray = static_cast<int>(RandomFloat(145.0f, 205.0f));
        const D3DCOLOR color = D3DCOLOR_ARGB(static_cast<int>(RandomFloat(65.0f, 115.0f)),
                                             gray,
                                             gray,
                                             gray);

        SpawnParticle(effect,
                      pos,
                      velocity,
                      life,
                      startSize,
                      endSize,
                      RandomFloat(0.0f, D3DX_PI * 2.0f),
                      RandomFloat(-1.4f, 1.4f),
                      color,
                      ParticleVisualType::ExplosionDust);
    }
}

void ParticleSystem::EmitDamage(EffectInstance& effect)
{
    const D3DCOLOR outlineColor = D3DCOLOR_ARGB(190, 52, 18, 42);
    const D3DXVECTOR3 center(effect.origin.x,
                             effect.origin.y + 0.42f,
                             effect.origin.z);

    SpawnParticle(effect,
                  center,
                  D3DXVECTOR3(0.0f, 0.32f, 0.0f),
                  0.22f,
                  0.86f,
                  1.18f,
                  0.0f,
                  RandomFloat(-1.4f, 1.4f),
                  D3DCOLOR_ARGB(230, 255, 92, 24),
                  ParticleVisualType::DamageRing);

    SpawnParticle(effect,
                  center,
                  D3DXVECTOR3(0.0f, 0.48f, 0.0f),
                  0.18f,
                  0.58f,
                  0.82f,
                  RandomFloat(0.0f, D3DX_PI * 2.0f),
                  RandomFloat(-4.5f, 4.5f),
                  D3DCOLOR_ARGB(255, 255, 247, 188),
                  ParticleVisualType::DamageCore);

    const float majorAngles[] =
    {
        -0.88f,
        0.22f,
        0.98f,
        2.36f,
        3.78f,
    };
    const float majorSizes[] =
    {
        2.18f,
        1.78f,
        2.45f,
        1.46f,
        1.76f,
    };

    for (int i = 0; i < 5; ++i)
    {
        const float angle = majorAngles[i] + RandomFloat(-0.10f, 0.10f);
        const float size = majorSizes[i] * RandomFloat(0.92f, 1.08f);
        const float life = RandomFloat(0.20f, 0.30f);

        SpawnParticle(effect,
                      center,
                      D3DXVECTOR3(0.0f, RandomFloat(0.24f, 0.54f), 0.0f),
                      life,
                      size * 1.16f,
                      size * 1.28f,
                      angle,
                      RandomFloat(-1.2f, 1.2f),
                      outlineColor,
                      ParticleVisualType::DamageOutline);

        SpawnParticle(effect,
                      center,
                      D3DXVECTOR3(0.0f, RandomFloat(0.32f, 0.62f), 0.0f),
                      life,
                      size,
                      size * 1.12f,
                      angle,
                      RandomFloat(-1.6f, 1.6f),
                      D3DCOLOR_ARGB(245, 255, 232, 98),
                      ParticleVisualType::DamageSpike);
    }

    for (int i = 0; i < 10; ++i)
    {
        const float angle = RandomFloat(0.0f, D3DX_PI * 2.0f);
        const float size = RandomFloat(0.50f, 1.05f);
        const float life = RandomFloat(0.18f, 0.34f);
        D3DCOLOR sparkColor = D3DCOLOR_ARGB(static_cast<int>(RandomFloat(160.0f, 230.0f)),
                                           255,
                                           245,
                                           172);
        if (i == 2 || i == 7)
        {
            sparkColor = D3DCOLOR_ARGB(static_cast<int>(RandomFloat(140.0f, 210.0f)),
                                       255,
                                       110,
                                       36);
        }

        SpawnParticle(effect,
                      center,
                      D3DXVECTOR3(0.0f, RandomFloat(0.12f, 0.42f), 0.0f),
                      life,
                      size,
                      size * 1.18f,
                      angle,
                      RandomFloat(-2.4f, 2.4f),
                      sparkColor,
                      ParticleVisualType::DamageSpark);
    }
}

void ParticleSystem::UpdateEffect(EffectInstance& effect, const float deltaTime)
{
    if (effect.preset == ParticleEffectPreset::None)
    {
        return;
    }

    switch (effect.preset)
    {
    case ParticleEffectPreset::Smoke:
        EmitSmoke(effect, deltaTime);
        break;
    case ParticleEffectPreset::Fire:
        EmitFire(effect, deltaTime);
        break;
    case ParticleEffectPreset::Fog:
        EmitFog(effect, deltaTime);
        break;
    case ParticleEffectPreset::Dust:
        EmitDust(effect, deltaTime);
        break;
    case ParticleEffectPreset::Rain:
        EmitRain(effect, deltaTime);
        break;
    case ParticleEffectPreset::Explosion:
        break;
    case ParticleEffectPreset::Damage:
        break;
    default:
        break;
    }

    bool hasActiveParticle = false;
    for (auto& particle : effect.particles)
    {
        if (!particle.active)
        {
            continue;
        }

        particle.life += deltaTime;

        if (particle.life >= particle.maxLife)
        {
            particle.active = false;
            continue;
        }

        float age = particle.life / particle.maxLife;
        age = ClampFloat(age, 0.0f, 1.0f);
        hasActiveParticle = true;

        if (effect.preset == ParticleEffectPreset::Smoke)
        {
            particle.velocity.x += RandomFloat(-0.12f, 0.12f) * deltaTime;
            particle.velocity.z += RandomFloat(-0.12f, 0.12f) * deltaTime;
            particle.velocity.y += 0.08f * deltaTime;
            particle.velocity.x *= 0.992f;
            particle.velocity.z *= 0.992f;
            particle.size = (particle.startSize + (particle.endSize - particle.startSize) * age) * particle.randomScale;

            int alpha = static_cast<int>(90.0f * (1.0f - age));
            int gray = static_cast<int>(110.0f + 70.0f * age);
            alpha = static_cast<int>(ClampFloat(static_cast<float>(alpha), 0.0f, 160.0f));
            gray = static_cast<int>(ClampFloat(static_cast<float>(gray), 0.0f, 255.0f));
            particle.color = D3DCOLOR_ARGB(alpha, gray, gray, gray);
        }
        else if (effect.preset == ParticleEffectPreset::Fire)
        {
            particle.velocity.x += RandomFloat(-0.10f, 0.10f) * deltaTime;
            particle.velocity.z += RandomFloat(-0.10f, 0.10f) * deltaTime;
            particle.velocity.y += 0.25f * deltaTime;
            particle.velocity.x *= 0.990f;
            particle.velocity.z *= 0.990f;
            particle.size = particle.startSize + (particle.endSize - particle.startSize) * sinf(age * D3DX_PI);

            const float intensity = 1.0f - age;
            const int alpha = static_cast<int>(ClampFloat(165.0f * intensity, 0.0f, 255.0f));
            int red = 255;
            int green = 0;
            int blue = 0;

            if (age < 0.18f)
            {
                red = 255;
                green = 244;
                blue = 215;
            }
            else if (age < 0.42f)
            {
                red = 255;
                green = 176;
                blue = 52;
            }
            else if (age < 0.72f)
            {
                red = 238;
                green = 88;
                blue = 20;
            }
            else
            {
                red = 165;
                green = 34;
                blue = 18;
            }

            particle.color = D3DCOLOR_ARGB(alpha, red, green, blue);
        }
        else if (effect.preset == ParticleEffectPreset::Fog)
        {
            particle.velocity.x += RandomFloat(-0.010f, 0.010f) * deltaTime;
            particle.velocity.y += RandomFloat(-0.008f, 0.008f) * deltaTime;
            particle.velocity.z += RandomFloat(-0.010f, 0.010f) * deltaTime;
            particle.velocity.x = ClampFloat(particle.velocity.x, 0.01f, 0.13f);
            particle.velocity.y = ClampFloat(particle.velocity.y, -0.045f, 0.045f);
            particle.velocity.z = ClampFloat(particle.velocity.z, -0.055f, 0.055f);
            particle.size = particle.startSize + (particle.endSize - particle.startSize) * age;

            const float fade = sinf(age * D3DX_PI);
            const int alpha = static_cast<int>(ClampFloat(24.0f * fade, 0.0f, 34.0f));
            particle.color = D3DCOLOR_ARGB(alpha, 214, 214, 214);
        }
        else if (effect.preset == ParticleEffectPreset::Dust)
        {
            particle.velocity.x += RandomFloat(-0.0010f, 0.0010f) * deltaTime;
            particle.velocity.y += RandomFloat(-0.0008f, 0.0008f) * deltaTime;
            particle.velocity.z += RandomFloat(-0.0012f, 0.0012f) * deltaTime;
            particle.velocity.x = ClampFloat(particle.velocity.x, 0.003f, 0.020f);
            particle.velocity.y = ClampFloat(particle.velocity.y, -0.009f, 0.009f);
            particle.velocity.z = ClampFloat(particle.velocity.z, -0.015f, 0.015f);
            particle.size = particle.startSize + (particle.endSize - particle.startSize) * age;

            particle.basePos.x += particle.velocity.x * deltaTime;
            particle.basePos.y += particle.velocity.y * deltaTime;
            particle.basePos.z += particle.velocity.z * deltaTime;

            const float swayTime = particle.life * 0.62f + particle.swayPhase;
            const float swayX = sinf(swayTime) * 0.055f;
            const float swayY = sinf(swayTime * 0.48f) * 0.022f;
            const float swayZ = cosf(swayTime * 0.58f) * 0.045f;
            particle.pos.x = particle.basePos.x + swayX;
            particle.pos.y = particle.basePos.y + swayY;
            particle.pos.z = particle.basePos.z + swayZ;

            const float fade = 0.25f + 0.75f * sinf(age * D3DX_PI);
            const float shimmer = 0.82f + 0.18f * sinf(particle.life * 2.4f + particle.swayPhase * 1.7f);
            const int alpha = static_cast<int>(ClampFloat(255.0f * fade * shimmer * particle.alphaBias, 255.0f, 255.0f));
            particle.color = D3DCOLOR_ARGB(alpha, 238, 238, 238);
            particle.rotation += particle.rotationSpeed * deltaTime;
            continue;
        }
        else if (effect.preset == ParticleEffectPreset::Rain)
        {
            particle.velocity.x += RandomFloat(-0.08f, 0.08f) * deltaTime;
            particle.velocity.z += RandomFloat(-0.08f, 0.08f) * deltaTime;
            particle.velocity.y = ClampFloat(particle.velocity.y, -18.0f, -9.0f);
            particle.size = particle.startSize;

            const float fade = sinf(age * D3DX_PI);
            const int alpha = static_cast<int>(ClampFloat(170.0f * fade * particle.alphaBias,
                                                          0.0f,
                                                          185.0f));
            particle.color = D3DCOLOR_ARGB(alpha, 210, 222, 255);
        }
        else if (effect.preset == ParticleEffectPreset::Explosion)
        {
            if (particle.visualType == ParticleVisualType::ExplosionFire)
            {
                particle.velocity.y -= 2.4f * deltaTime;
                particle.velocity.x *= 0.965f;
                particle.velocity.z *= 0.965f;
                particle.size = particle.startSize + (particle.endSize - particle.startSize) * sinf(age * D3DX_PI);

                const float intensity = 1.0f - age;
                const int alpha = static_cast<int>(ClampFloat(245.0f * intensity, 0.0f, 255.0f));
                int red = 255;
                int green = 205;
                int blue = 105;
                if (age >= 0.18f && age < 0.50f)
                {
                    green = 140;
                    blue = 38;
                }
                else if (age >= 0.50f)
                {
                    green = 72;
                    blue = 22;
                }
                particle.color = D3DCOLOR_ARGB(alpha, red, green, blue);
            }
            else if (particle.visualType == ParticleVisualType::ExplosionSpark)
            {
                particle.velocity.y -= 6.8f * deltaTime;
                particle.velocity.x *= 0.982f;
                particle.velocity.z *= 0.982f;
                particle.size = particle.startSize + (particle.endSize - particle.startSize) * age;

                const float intensity = 1.0f - age;
                const int alpha = static_cast<int>(ClampFloat(230.0f * intensity * particle.alphaBias, 0.0f, 255.0f));
                particle.color = D3DCOLOR_ARGB(alpha, 255, 210, 88);
            }
            else if (particle.visualType == ParticleVisualType::ExplosionSmoke)
            {
                particle.velocity.x += RandomFloat(-0.22f, 0.22f) * deltaTime;
                particle.velocity.z += RandomFloat(-0.22f, 0.22f) * deltaTime;
                particle.velocity.y += 0.24f * deltaTime;
                particle.velocity.x *= 0.990f;
                particle.velocity.z *= 0.990f;
                particle.size = (particle.startSize + (particle.endSize - particle.startSize) * age) * particle.randomScale;

                int alpha = static_cast<int>(118.0f * (1.0f - age) * particle.alphaBias);
                int gray = static_cast<int>(82.0f + 70.0f * age);
                alpha = static_cast<int>(ClampFloat(static_cast<float>(alpha), 0.0f, 145.0f));
                gray = static_cast<int>(ClampFloat(static_cast<float>(gray), 0.0f, 190.0f));
                particle.color = D3DCOLOR_ARGB(alpha, gray, gray, gray);
            }
            else if (particle.visualType == ParticleVisualType::ExplosionDust)
            {
                particle.velocity.x *= 0.972f;
                particle.velocity.z *= 0.972f;
                particle.velocity.y += RandomFloat(-0.10f, 0.10f) * deltaTime;
                particle.size = particle.startSize + (particle.endSize - particle.startSize) * age;

                const float fade = sinf(age * D3DX_PI);
                const int alpha = static_cast<int>(ClampFloat(120.0f * fade * particle.alphaBias, 0.0f, 150.0f));
                particle.color = D3DCOLOR_ARGB(alpha, 184, 178, 162);
            }
        }
        else if (effect.preset == ParticleEffectPreset::Damage)
        {
            particle.velocity.y -= 4.6f * deltaTime;
            particle.velocity.x *= 0.955f;
            particle.velocity.z *= 0.955f;

            const float fade = 1.0f - age;
            if (particle.visualType == ParticleVisualType::DamageOutline)
            {
                particle.size = particle.startSize + (particle.endSize - particle.startSize) * age;
                const int alpha = static_cast<int>(ClampFloat(180.0f * fade * particle.alphaBias,
                                                              0.0f,
                                                              190.0f));
                particle.color = D3DCOLOR_ARGB(alpha, 48, 14, 38);
            }
            else if (particle.visualType == ParticleVisualType::DamageCore)
            {
                particle.size = particle.startSize + (particle.endSize - particle.startSize) * sinf(age * D3DX_PI);
                const int alpha = static_cast<int>(ClampFloat(250.0f * fade, 0.0f, 255.0f));
                particle.color = D3DCOLOR_ARGB(alpha, 255, 248, 196);
            }
            else if (particle.visualType == ParticleVisualType::DamageRing)
            {
                particle.size = particle.startSize + (particle.endSize - particle.startSize) * age;
                const int alpha = static_cast<int>(ClampFloat(220.0f * fade, 0.0f, 230.0f));
                particle.color = D3DCOLOR_ARGB(alpha, 255, 100, 28);
            }
            else if (particle.visualType == ParticleVisualType::DamageSpike)
            {
                particle.size = particle.startSize + (particle.endSize - particle.startSize) * age;
                const int alpha = static_cast<int>(ClampFloat(245.0f * fade, 0.0f, 250.0f));
                particle.color = D3DCOLOR_ARGB(alpha, 255, 232, 98);
            }
            else if (particle.visualType == ParticleVisualType::DamageSpark)
            {
                particle.size = particle.startSize + (particle.endSize - particle.startSize) * age;
                const int alpha = static_cast<int>(ClampFloat(235.0f * fade * particle.alphaBias,
                                                              0.0f,
                                                              245.0f));
                const int red = static_cast<int>((particle.color >> 16) & 0xff);
                const int green = static_cast<int>((particle.color >> 8) & 0xff);
                const int blue = static_cast<int>(particle.color & 0xff);
                particle.color = D3DCOLOR_ARGB(alpha, red, green, blue);
            }
        }

        particle.rotation += particle.rotationSpeed * deltaTime;
        particle.pos.x += particle.velocity.x * deltaTime;
        particle.pos.y += particle.velocity.y * deltaTime;
        particle.pos.z += particle.velocity.z * deltaTime;
    }

    if ((effect.preset == ParticleEffectPreset::Explosion ||
         effect.preset == ParticleEffectPreset::Damage) &&
        !hasActiveParticle)
    {
        effect.preset = ParticleEffectPreset::None;
        effect.generation = 0;
    }
}

void ParticleSystem::DrawEffect(const EffectInstance& effectInstance, const D3DXMATRIX& view, const D3DXMATRIX& proj)
{
    if (effectInstance.preset == ParticleEffectPreset::None || m_effect == NULL)
    {
        return;
    }

    LPDIRECT3DTEXTURE9 texture = NULL;
    bool additive = false;
    if (effectInstance.preset == ParticleEffectPreset::Fire)
    {
        additive = true;
    }

    switch (effectInstance.preset)
    {
    case ParticleEffectPreset::Smoke:
        texture = m_smokeTexture;
        break;
    case ParticleEffectPreset::Fire:
        texture = m_fireTexture;
        break;
    case ParticleEffectPreset::Dust:
        texture = m_dustTexture;
        break;
    case ParticleEffectPreset::Fog:
        texture = m_fogTexture;
        break;
    case ParticleEffectPreset::Rain:
        texture = m_rainTexture;
        break;
    case ParticleEffectPreset::Explosion:
        texture = m_fireTexture;
        break;
    case ParticleEffectPreset::Damage:
        texture = m_damageCoreTexture;
        break;
    default:
        return;
    }

    if (texture == NULL)
    {
        return;
    }

    D3DXMATRIX invView;
    D3DXMatrixInverse(&invView, NULL, &view);
    const D3DXVECTOR3 cameraPos(invView._41, invView._42, invView._43);

    D3DXVECTOR3 cameraRight(invView._11, invView._12, invView._13);
    D3DXVECTOR3 cameraUp(invView._21, invView._22, invView._23);
    D3DXVec3Normalize(&cameraRight, &cameraRight);
    D3DXVec3Normalize(&cameraUp, &cameraUp);

    const D3DXVECTOR3 worldDown(0.0f, -1.0f, 0.0f);
    float rainX = D3DXVec3Dot(&worldDown, &cameraRight);
    float rainY = D3DXVec3Dot(&worldDown, &cameraUp);
    D3DXVECTOR3 rainDown = cameraRight * rainX + cameraUp * rainY;
    if (D3DXVec3LengthSq(&rainDown) <= 0.0001f)
    {
        rainDown = cameraUp * -1.0f;
        rainX = 0.0f;
        rainY = -1.0f;
    }
    else
    {
        D3DXVec3Normalize(&rainDown, &rainDown);
    }
    D3DXVECTOR3 rainUp = rainDown * -1.0f;
    D3DXVECTOR3 rainRight = cameraRight * (-rainY) + cameraUp * rainX;
    if (D3DXVec3LengthSq(&rainRight) <= 0.0001f)
    {
        rainRight = cameraRight;
    }
    else
    {
        D3DXVec3Normalize(&rainRight, &rainRight);
    }

    auto drawBatch = [&](LPDIRECT3DTEXTURE9 batchTexture,
                         const ParticleVisualType visualTypeFilter,
                         const char* techniqueName) -> void
    {
        HRESULT hResult = E_FAIL;
        int activeCount = 0;
        if (effectInstance.preset == ParticleEffectPreset::Dust)
        {
            activeCount = FillDustVertices(effectInstance, batchTexture, view);
        }
        if (activeCount <= 0)
        {
            if (effectInstance.preset == ParticleEffectPreset::Dust)
            {
                return;
            }
        }

        if (effectInstance.preset != ParticleEffectPreset::Dust)
        {
            int activeCountNonDust = 0;

            for (const auto& particle : effectInstance.particles)
            {
                if (!particle.active)
                {
                    continue;
                }

                if ((effectInstance.preset == ParticleEffectPreset::Explosion ||
                     effectInstance.preset == ParticleEffectPreset::Damage) &&
                    particle.visualType != visualTypeFilter)
                {
                    continue;
                }

                if (effectInstance.preset == ParticleEffectPreset::Rain)
                {
                    const D3DXVECTOR3 toParticle = particle.pos - cameraPos;
                    if (D3DXVec3LengthSq(&toParticle) < 4.0f)
                    {
                        continue;
                    }
                }

                const float cosValue = cosf(particle.rotation);
                const float sinValue = sinf(particle.rotation);
                float halfWidth = particle.size * 0.5f;
                float halfHeight = particle.size * 0.5f;
                D3DXVECTOR3 center = particle.pos;

                D3DXVECTOR3 rotatedRight;
                rotatedRight.x = cameraRight.x * cosValue + cameraUp.x * sinValue;
                rotatedRight.y = cameraRight.y * cosValue + cameraUp.y * sinValue;
                rotatedRight.z = cameraRight.z * cosValue + cameraUp.z * sinValue;

                D3DXVECTOR3 rotatedUp;
                rotatedUp.x = cameraUp.x * cosValue - cameraRight.x * sinValue;
                rotatedUp.y = cameraUp.y * cosValue - cameraRight.y * sinValue;
                rotatedUp.z = cameraUp.z * cosValue - cameraRight.z * sinValue;

                if (effectInstance.preset == ParticleEffectPreset::Fire)
                {
                    halfWidth = particle.size * 0.38f;
                    halfHeight = particle.size * 0.92f;
                    center.y += halfHeight * 0.18f;
                }
                else if (effectInstance.preset == ParticleEffectPreset::Explosion)
                {
                    if (particle.visualType == ParticleVisualType::ExplosionSpark)
                    {
                        halfWidth = particle.size * 0.34f;
                        halfHeight = particle.size * 1.15f;
                    }
                    else if (particle.visualType == ParticleVisualType::ExplosionSmoke)
                    {
                        halfWidth = particle.size * 0.72f;
                        halfHeight = particle.size * 0.62f;
                    }
                    else if (particle.visualType == ParticleVisualType::ExplosionDust)
                    {
                        halfWidth = particle.size * 0.86f;
                        halfHeight = particle.size * 0.42f;
                    }
                }
                else if (effectInstance.preset == ParticleEffectPreset::Fog)
                {
                    halfWidth = particle.size * 0.75f;
                    halfHeight = particle.size * 0.62f;
                }
                else if (effectInstance.preset == ParticleEffectPreset::Rain)
                {
                    rotatedRight = rainRight;
                    rotatedUp = rainUp;
                    halfWidth = particle.size * 0.50f;
                    halfHeight = particle.size * 0.50f;
                }
                else if (effectInstance.preset == ParticleEffectPreset::Damage)
                {
                    if (particle.visualType == ParticleVisualType::DamageSpark)
                    {
                        halfWidth = particle.size * 0.11f;
                        halfHeight = particle.size * 0.48f;
                    }
                    else if (particle.visualType == ParticleVisualType::DamageRing)
                    {
                        halfWidth = particle.size * 0.76f;
                        halfHeight = particle.size * 0.76f;
                    }
                    else if (particle.visualType == ParticleVisualType::DamageCore)
                    {
                        halfWidth = particle.size * 0.58f;
                        halfHeight = particle.size * 0.58f;
                    }
                    else if (particle.visualType == ParticleVisualType::DamageSpike)
                    {
                        halfWidth = particle.size * 0.15f;
                        halfHeight = particle.size * 1.12f;
                    }
                    else if (particle.visualType == ParticleVisualType::DamageOutline)
                    {
                        halfWidth = particle.size * 0.18f;
                        halfHeight = particle.size * 1.18f;
                    }
                }

                const D3DXVECTOR3 halfRight(rotatedRight.x * halfWidth,
                                            rotatedRight.y * halfWidth,
                                            rotatedRight.z * halfWidth);
                const D3DXVECTOR3 halfUp(rotatedUp.x * halfHeight,
                                         rotatedUp.y * halfHeight,
                                         rotatedUp.z * halfHeight);

                const D3DXVECTOR3 topLeft(center.x - halfRight.x + halfUp.x,
                                          center.y - halfRight.y + halfUp.y,
                                          center.z - halfRight.z + halfUp.z);
                const D3DXVECTOR3 topRight(center.x + halfRight.x + halfUp.x,
                                           center.y + halfRight.y + halfUp.y,
                                           center.z + halfRight.z + halfUp.z);
                const D3DXVECTOR3 bottomLeft(center.x - halfRight.x - halfUp.x,
                                             center.y - halfRight.y - halfUp.y,
                                             center.z - halfRight.z - halfUp.z);
                const D3DXVECTOR3 bottomRight(center.x + halfRight.x - halfUp.x,
                                              center.y + halfRight.y - halfUp.y,
                                              center.z + halfRight.z - halfUp.z);

                const int vertexIndex = activeCountNonDust * PARTICLE_VERTEX_COUNT;
                m_vertices[vertexIndex + 0] = { topLeft, particle.color, 0.0f, 0.0f };
                m_vertices[vertexIndex + 1] = { topRight, particle.color, 1.0f, 0.0f };
                m_vertices[vertexIndex + 2] = { bottomLeft, particle.color, 0.0f, 1.0f };
                m_vertices[vertexIndex + 3] = { bottomLeft, particle.color, 0.0f, 1.0f };
                m_vertices[vertexIndex + 4] = { topRight, particle.color, 1.0f, 0.0f };
                m_vertices[vertexIndex + 5] = { bottomRight, particle.color, 1.0f, 1.0f };
                ++activeCountNonDust;
            }

            if (activeCountNonDust <= 0)
            {
                return;
            }

            hResult = m_effect->SetTechnique(techniqueName);
            assert(SUCCEEDED(hResult));

            hResult = m_effect->SetTexture("g_texture0", batchTexture);
            assert(SUCCEEDED(hResult));

            UINT numPasses = 0;
            hResult = m_effect->Begin(&numPasses, 0);
            assert(SUCCEEDED(hResult));

            for (UINT passIndex = 0; passIndex < numPasses; ++passIndex)
            {
                hResult = m_effect->BeginPass(passIndex);
                assert(SUCCEEDED(hResult));

                hResult = m_effect->CommitChanges();
                assert(SUCCEEDED(hResult));

                hResult = Common::D3DDevice()->SetFVF(ParticleVertex::FVF);
                assert(SUCCEEDED(hResult));

                hResult = Common::D3DDevice()->DrawPrimitiveUP(D3DPT_TRIANGLELIST,
                                                               activeCountNonDust * 2,
                                                               m_vertices.data(),
                                                               sizeof(ParticleVertex));
                assert(SUCCEEDED(hResult));

                hResult = m_effect->EndPass();
                assert(SUCCEEDED(hResult));
            }

            hResult = m_effect->End();
            assert(SUCCEEDED(hResult));
            return;
        }

        hResult = m_effect->SetTechnique(techniqueName);
        assert(SUCCEEDED(hResult));

        hResult = m_effect->SetTexture("g_texture0", batchTexture);
        assert(SUCCEEDED(hResult));

        UINT numPasses = 0;
        hResult = m_effect->Begin(&numPasses, 0);
        assert(SUCCEEDED(hResult));

        for (UINT passIndex = 0; passIndex < numPasses; ++passIndex)
        {
            hResult = m_effect->BeginPass(passIndex);
            assert(SUCCEEDED(hResult));

            hResult = m_effect->CommitChanges();
            assert(SUCCEEDED(hResult));

            hResult = Common::D3DDevice()->SetFVF(ParticleVertex::FVF);
            assert(SUCCEEDED(hResult));

            hResult = Common::D3DDevice()->DrawPrimitiveUP(D3DPT_TRIANGLELIST,
                                                           activeCount * 2,
                                                           m_vertices.data(),
                                                           sizeof(ParticleVertex));
            assert(SUCCEEDED(hResult));

            hResult = m_effect->EndPass();
            assert(SUCCEEDED(hResult));
        }

        hResult = m_effect->End();
        assert(SUCCEEDED(hResult));
    };

    D3DXMATRIX world;
    D3DXMATRIX worldViewProj;
    D3DXMatrixIdentity(&world);
    worldViewProj = world * view * proj;

    HRESULT hResult = m_effect->SetMatrix("g_matWorldViewProj", &worldViewProj);
    assert(SUCCEEDED(hResult));

    if (effectInstance.preset == ParticleEffectPreset::Dust)
    {
        drawBatch(m_dustTexture, ParticleVisualType::Default, "ParticleAlphaTechnique");
        drawBatch(m_dustTexture2, ParticleVisualType::Default, "ParticleAlphaTechnique");
    }
    else if (effectInstance.preset == ParticleEffectPreset::Explosion)
    {
        drawBatch(m_fireTexture, ParticleVisualType::ExplosionFire, "ParticleAdditiveTechnique");
        drawBatch(m_fireTexture, ParticleVisualType::ExplosionSpark, "ParticleAdditiveTechnique");
        drawBatch(m_smokeTexture, ParticleVisualType::ExplosionSmoke, "ParticleAlphaTechnique");
        drawBatch(m_dustTexture2, ParticleVisualType::ExplosionDust, "ParticleAlphaTechnique");
    }
    else if (effectInstance.preset == ParticleEffectPreset::Damage)
    {
        drawBatch(m_damageSpikeTexture, ParticleVisualType::DamageOutline, "ParticleAlphaTechnique");
        drawBatch(m_damageRingTexture, ParticleVisualType::DamageRing, "ParticleAdditiveTechnique");
        drawBatch(m_damageSpikeTexture, ParticleVisualType::DamageSpike, "ParticleAdditiveTechnique");
        drawBatch(m_damageCoreTexture, ParticleVisualType::DamageCore, "ParticleAdditiveTechnique");
        drawBatch(m_damageSpikeTexture, ParticleVisualType::DamageSpark, "ParticleAdditiveTechnique");
    }
    else
    {
        if (additive)
        {
            drawBatch(texture, ParticleVisualType::Default, "ParticleAdditiveTechnique");
        }
        else
        {
            drawBatch(texture, ParticleVisualType::Default, "ParticleAlphaTechnique");
        }
    }

    hResult = Common::D3DDevice()->SetTexture(0, NULL);
    assert(SUCCEEDED(hResult));
    hResult = Common::D3DDevice()->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    assert(SUCCEEDED(hResult));
    hResult = Common::D3DDevice()->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    assert(SUCCEEDED(hResult));
    hResult = Common::D3DDevice()->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
    assert(SUCCEEDED(hResult));
    hResult = Common::D3DDevice()->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
    assert(SUCCEEDED(hResult));
    hResult = Common::D3DDevice()->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
    assert(SUCCEEDED(hResult));
    hResult = Common::D3DDevice()->SetRenderState(D3DRS_LIGHTING, TRUE);
    assert(SUCCEEDED(hResult));
}

std::wstring ParticleSystem::BuildAssetPath(const wchar_t* fileName) const
{
    const std::wstring exeDir = Util::GetExeDir();

    const std::wstring candidatePaths[] =
    {
        exeDir + fileName,
        exeDir + L"particle\\" + fileName,
        exeDir + L"assets\\particle\\" + fileName,
        exeDir + L"..\\Render\\assets\\particle\\" + fileName,
        exeDir + L"..\\..\\Render\\assets\\particle\\" + fileName,
        exeDir + L"..\\Sample\\res\\2D_image\\" + fileName,
        exeDir + L"..\\..\\Sample\\res\\2D_image\\" + fileName,
    };

    for (const auto& candidatePath : candidatePaths)
    {
        const DWORD attributes = GetFileAttributesW(candidatePath.c_str());
        if (attributes != INVALID_FILE_ATTRIBUTES &&
            (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
        {
            return candidatePath;
        }
    }

    return exeDir + fileName;
}

float ParticleSystem::RandomFloat(const float minValue, const float maxValue) const
{
    const float t = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
    return minValue + (maxValue - minValue) * t;
}

float ParticleSystem::RandomCenteredFloat(const float radius) const
{
    const float a = RandomFloat(-1.0f, 1.0f);
    const float b = RandomFloat(-1.0f, 1.0f);
    const float c = RandomFloat(-1.0f, 1.0f);
    return (a + b + c) * (radius / 3.0f);
}

float ParticleSystem::ClampFloat(const float value, const float minValue, const float maxValue)
{
    return (std::max)(minValue, (std::min)(value, maxValue));
}
}
