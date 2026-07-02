#pragma once

#include <d3d9.h>
#include <d3dx9.h>

#include <string>
#include <vector>

namespace NSRender
{
enum class ParticleEffectPreset
{
    None = 0,
    Smoke,
    Fire,
    Dust,
    Fog,
    Rain,
    Explosion,
    Damage,
};

class ParticleSystem
{
public:
    void Initialize();
    void Finalize();

    void OnDeviceLost();
    void OnDeviceReset();

    void PlaceEffect(ParticleEffectPreset preset, const D3DXVECTOR3& origin);
    void ClearEffect();
    void SetDustFixedScreenSize(bool enabled);
    void SetExplosionScale(float scale);
    float GetExplosionScale() const;

    void Update(float deltaTime);
    void Draw(const D3DXMATRIX& view, const D3DXMATRIX& proj);
    void RenderDustToGBufferEffect(LPD3DXEFFECT effect,
                                   const D3DXMATRIX& view,
                                   const D3DXMATRIX& proj,
                                   const char* techniqueName);

    ParticleEffectPreset GetPreset() const;

private:
    enum class ParticleVisualType
    {
        Default = 0,
        ExplosionFire,
        ExplosionSpark,
        ExplosionSmoke,
        ExplosionDust,
        DamageOutline,
        DamageCore,
        DamageRing,
        DamageSpike,
        DamageSpark,
    };

    struct Particle
    {
        D3DXVECTOR3 pos = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
        D3DXVECTOR3 velocity = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
        float life = 0.0f;
        float maxLife = 1.0f;
        float size = 1.0f;
        float startSize = 1.0f;
        float endSize = 1.0f;
        float rotation = 0.0f;
        float rotationSpeed = 0.0f;
        D3DXVECTOR3 basePos = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
        float swayPhase = 0.0f;
        float randomScale = 1.0f;
        float alphaBias = 1.0f;
        bool useAltTexture = false;
        ParticleVisualType visualType = ParticleVisualType::Default;
        D3DCOLOR color = 0xffffffff;
        bool active = false;
    };

    struct ParticleVertex
    {
        D3DXVECTOR3 pos;
        D3DCOLOR color;
        float u;
        float v;

        enum
        {
            FVF = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1
        };
    };

    struct EffectInstance
    {
        ParticleEffectPreset preset = ParticleEffectPreset::None;
        D3DXVECTOR3 origin = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
        std::vector<Particle> particles;
        float smokeEmitAccumulator = 0.0f;
        float fireEmitAccumulator = 0.0f;
        float fireSparkAccumulator = 0.0f;
        float fogEmitAccumulator = 0.0f;
        float dustEmitAccumulator = 0.0f;
        float rainEmitAccumulator = 0.0f;
        unsigned long long generation = 0;
    };

    static constexpr int MAX_EFFECT_INSTANCES = 8;
    static constexpr int MAX_PARTICLES = 768;
    static constexpr int PARTICLE_VERTEX_COUNT = 6;

    bool TryInitializeResources();
    void ClearParticles(EffectInstance& effect);
    void SpawnParticle(EffectInstance& effect,
                       const D3DXVECTOR3& pos,
                       const D3DXVECTOR3& velocity,
                       float maxLife,
                       float startSize,
                       float endSize,
                       float rotation,
                       float rotationSpeed,
                       D3DCOLOR color,
                       ParticleVisualType visualType = ParticleVisualType::Default);

    void EmitSmoke(EffectInstance& effect, float deltaTime);
    void EmitFire(EffectInstance& effect, float deltaTime);
    void EmitDust(EffectInstance& effect, float deltaTime);
    void EmitFog(EffectInstance& effect, float deltaTime);
    void EmitRain(EffectInstance& effect, float deltaTime);
    void EmitExplosion(EffectInstance& effect);
    void EmitDamage(EffectInstance& effect);
    void UpdateEffect(EffectInstance& effect, float deltaTime);
    void DrawEffect(const EffectInstance& effect, const D3DXMATRIX& view, const D3DXMATRIX& proj);
    int FillDustVertices(const EffectInstance& effectInstance,
                         LPDIRECT3DTEXTURE9 batchTexture,
                         const D3DXMATRIX& view);

    std::wstring BuildAssetPath(const wchar_t* fileName) const;
    float RandomFloat(float minValue, float maxValue) const;
    float RandomCenteredFloat(float radius) const;
    static float ClampFloat(float value, float minValue, float maxValue);

    bool m_initialized = false;
    ParticleEffectPreset m_lastPlacedPreset = ParticleEffectPreset::None;
    unsigned long long m_nextGeneration = 1;
    std::vector<EffectInstance> m_effects;
    std::vector<ParticleVertex> m_vertices;

    LPDIRECT3DTEXTURE9 m_smokeTexture = NULL;
    LPDIRECT3DTEXTURE9 m_fireTexture = NULL;
    LPDIRECT3DTEXTURE9 m_dustTexture = NULL;
    LPDIRECT3DTEXTURE9 m_dustTexture2 = NULL;
    LPDIRECT3DTEXTURE9 m_fogTexture = NULL;
    LPDIRECT3DTEXTURE9 m_rainTexture = NULL;
    LPDIRECT3DTEXTURE9 m_damageCoreTexture = NULL;
    LPDIRECT3DTEXTURE9 m_damageRingTexture = NULL;
    LPDIRECT3DTEXTURE9 m_damageSpikeTexture = NULL;
    LPD3DXEFFECT m_effect = NULL;
    bool m_dustFixedScreenSizeEnabled = true;
    float m_explosionScale = 1.0f;
};
}
