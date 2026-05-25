// ひとまずComPtrは無しでやってみる。
// 必要になったら考える

// 解像度は1600x900を基本と考える。
// 例えば1920x1080は、1.2倍の解像度、と考える。
// この考えに従ってUIの大きさやフォントのサイズを調節できる

#include <d3d9.h>
#include <d3dx9.h>
#include <string>
#include <tchar.h>
#include <cassert>
#include <crtdbg.h>
#include <vector>
#include <deque>
#include <chrono>
#include <unordered_map>

#include "Font.h"
#include "FontEx.h"
#include "Sprite.h"
#include "ParticleSystem.h"

#include "MeshOld.h"
#include "MeshSmooth.h"
#include "MeshSSSLike.h"
#include "MeshPointLight.h"
#include "MeshNormalMapping.h"
#include "MeshMix.h"
#include "MeshMixManager.h"
#include "MeshPBRManager.h"
#include "MeshMixSkinAnim.h"
#include "MeshSSS.h"
#include "MeshPOM.h"

#include "AnimMesh.h"
#include "SkinAnimMesh.h"

#include "MeshInstancing.h"

#include "PostEffectGauss.h"
#include "PostEffectMaskedGauss.h"
#include "PostEffectSaturate.h"
#include "PostEffectBloom.h"
#include "PostEffectAA.h"
#include "PostEffectHalo.h"
#include "PostEffectStarBurst.h"
#include "PostEffectEnd.h"
#include "PostEffectSSAO.h"
#include "PostEffectSSGI.h"
#include "PostEffectDepthOfField.h"
#include "PostEffectHeightFog.h"
#include "PostEffectFXAA.h"
#include "PostEffectTAA.h"

#include "WindowManager.h"
#include "Light.h"
#include "PostEffectZShadow.h"
#include "GBuffer.h"
#include "PostEffectFog.h"
#include "PostEffectGodRay.h"
#include "PostEffectMotionBlurCamera.h"

namespace NSRender
{

enum class DebugGBufferView
{
    None = 0,
    WorldPos = 1,
    Normal = 2,
    Depth = 3,
    Thickness = 4,
};

struct RenderingQualitySettings
{
    std::wstring quality = L"LOW";
    bool gBufferEnabled = false;
    bool saturateEnabled = false;
    bool gaussianEnabled = false;
    bool maskedGaussianEnabled = false;
    bool postEffectAAEnabled = false;
    bool fxaaEnabled = false;
    bool taaEnabled = false;
    bool motionBlurCameraEnabled = false;
    bool depthBufferShadowEnabled = false;
    bool ssaoEnabled = false;
    bool ssgiEnabled = false;
    bool fogEnabled = false;
    bool heightFogEnabled = false;
    bool bloomEnabled = false;
    bool haloEnabled = false;
    DepthOfFieldMode depthOfFieldMode = DepthOfFieldMode::Disabled;
    bool starBurstEnabled = false;
    bool godRayEnabled = false;
};

class Render : public IDeviceResettable
{

public:

    // 初期化後に CSV 設定だけを読み直したい場合のための関数。
    // Sample の設定ダイアログから後読込するときにも使う。
    void ReloadSettingsCsv(const std::wstring& settingsCsvPath);

    void Initialize(HWND hWnd, const std::wstring& settingsCsvPath = L"");
    void Finalize();
    void Draw();

    void ChangeResolution(const int W, const int H);

    void ChangeWindowMode(const eWindowMode eWindowMode_);

    int AddMesh(const std::wstring& filePath,
                const D3DXVECTOR3& pos,
                const D3DXVECTOR3& rot,
                const float scale,
                const float radius = -1.f,
                const float uvTile = 1.0f);
    bool RemoveMesh(int id);

    int AddMeshNoLighting(const std::wstring& filePath,
                          const D3DXVECTOR3& pos,
                          const D3DXVECTOR3& rot,
                          const float scale,
                          const float radius = -1.f,
                          const float uvTile = 1.0f);

    void AddMeshSmooth(const std::wstring& filePath,
                       const D3DXVECTOR3& pos,
                       const D3DXVECTOR3& rot,
                       const float scale,
                       const float radius = -1.f);

    void AddMeshSSSLike(const std::wstring& filePath,
                        const D3DXVECTOR3& pos,
                        const D3DXVECTOR3& rot,
                        const float scale,
                        const float radius = -1.f);

    int AddMeshSSS(const std::wstring& filePath,
                   const D3DXVECTOR3& pos,
                   const D3DXVECTOR3& rot,
                   const float scale,
                   const float radius = -1.f);
    bool RemoveMeshSSS(int id);

    int AddMeshPointLight(const std::wstring& filePath,
                          const D3DXVECTOR3& pos,
                          const D3DXVECTOR3& rot,
                          const float scale,
                          const float radius = -1.f);
    bool RemoveMeshPointLight(int id);

    int AddMeshNormalMapping(const std::wstring& filePath,
                             const std::wstring& normalMap,
                             const D3DXVECTOR3& pos,
                             const D3DXVECTOR3& rot,
                             const float scale,
                             const float radius = -1.f);
    bool RemoveMeshNormalMapping(int id);

    int AddMeshPOM(const std::wstring& filePath,
                   const D3DXVECTOR3& pos,
                   const D3DXVECTOR3& rot,
                   const float scale,
                   const float radius);
    bool RemoveMeshPOM(int id);

    int AddAnimMesh(const std::wstring& filePath,
                    const D3DXVECTOR3& pos,
                    const D3DXVECTOR3& rot,
                    const float scale,
                    const AnimSetMap& animSetMap);
    bool RemoveAnimMesh(int id);

    int AddSkinAnimMesh(const std::wstring& filePath,
                        const D3DXVECTOR3& pos,
                        const D3DXVECTOR3& rot,
                        const float scale,
                        const AnimSetMap& animSetMap);
    bool RemoveSkinAnimMesh(int id);

    // インスタンシング可能なメッシュ
    int AddMeshInstansing(const std::wstring& filePath,
                          const D3DXVECTOR3& pos,
                          const D3DXVECTOR3& rot,
                          const float scale);
    bool RemoveMeshInstancing(const std::wstring& filePath);
    void SetMeshInstancingHighQuality(bool enabled);

    int AddMeshMix(const std::wstring& filePath,
                   const D3DXVECTOR3& pos,
                   const D3DXVECTOR3& rot,
                   const float scale,
                   const float radius = -1.f,
                   const bool useParallaxOcclusionMapping = false,
                   const bool useNormalMapping = false,
                   const bool async = true);
    bool RemoveMeshMix(int id);
    int AddMeshPBR(const std::wstring& filePath,
                   const D3DXVECTOR3& pos,
                   const D3DXVECTOR3& rot,
                   const float scale,
                   const float radius = -1.f,
                   const std::wstring& envMapPath = L"",
                   const bool async = true);
    bool RemoveMeshPBR(int id);
    int AddMeshMixSkinAnim(const std::wstring& filePath,
                           const D3DXVECTOR3& pos,
                           const D3DXVECTOR3& rot,
                           const float scale,
                           const AnimSetMap& animSetMap,
                           const float radius = -1.f,
                           const bool useParallaxOcclusionMapping = false,
                           const bool useNormalMapping = false);
    int AddMeshMixSkinAnim(const std::wstring& meshFilePath,
                           const std::wstring& animationFilePath,
                           const D3DXVECTOR3& pos,
                           const D3DXVECTOR3& rot,
                           const float scale,
                           const AnimSetMap& animSetMap,
                           const float radius = -1.f,
                           const bool useParallaxOcclusionMapping = false,
                           const bool useNormalMapping = false);
    bool RemoveMeshMixSkinAnim(int id);
    const std::vector<MeshMixSkinAnim::AnimationInfo>* GetMeshMixSkinAnimAnimationInfoList(int id) const;
    bool PlayMeshMixSkinAnimAnimation(int id, const std::wstring& name);
    void SetMeshMixSkinAnimAlphaClip(const bool enabled);

    void SetMeshMixPos(const int id, const D3DXVECTOR3& pos);
    void SetMeshMixSaturateShadow(const bool enabled);
    void SetMeshMixSaturateShadowIntensity(const float intensity);
    void SetMeshMixShadowDarkness(const float darkness);
    void SetMeshMixSpecularIntensity(const float intensity);
    void SetMeshMixSpecularEdge(const float edge);
    void SetMeshMixFresnelIntensity(const float intensity);
    void SetMeshMixEnvMapBlend(const float blend);
    void SetMeshPBRRoughness(const float roughness);
    void SetMeshPBRMetallic(const float metallic);
    void SetMeshPBREnvReflectionIntensity(const float intensity);
    void SetMeshPBREnvMaxMipLevel(const float mipLevel);
    void SetMeshPBREnvDiffuseIntensity(const float intensity);
    void SetMeshPBREnvDiffuseMipLevel(const float mipLevel);
    bool SetMeshPBREnvMapTexturePath(const std::wstring& envMapTexturePath);
    void SetMeshMixSpecularIntensityOverrideEnabled(const bool enabled);
    void SetMeshMixSpecularEdgeOverrideEnabled(const bool enabled);
    void SetPhongTreatTextureAsWhite(const bool enabled);

    void SetCamera(const D3DXVECTOR3& pos, const D3DXVECTOR3& lookAt);
    void MoveCamera(const D3DXVECTOR3& pos);
    void RotateCamera(const D3DXVECTOR3& rot);
    void SetCameraShakeDuration(const float durationSeconds);
    void SetCameraShakeIntensity(const float intensity);
    void TriggerCameraShake();
    void SetCameraClipPlanes(const float nearPlane, const float farPlane);
    void SetGBufferEnable(const bool enabled);
    void SetGBufferClipPlanes(const float nearPlane, const float farPlane);
    RenderingQualitySettings SetRenderQuality(const std::wstring& quality);

    D3DXVECTOR3 GetLookAtPos();
    D3DXVECTOR3 GetCameraPos();
    D3DXVECTOR3 GetCameraRotate();

    // フォント作成
    // IDが返ってくるので、そのIDを文字描画するときに指定する
    int SetUpFont(const std::wstring& fontName, const int fontSize, const UINT fontColor);
    int SetUpFontEx(const std::wstring& fontName, const int fontSize, const UINT fontColor);

    // フォント作成時に取得したIDを指定して文字を描画する
    // 文字が表示され続けるためにはこの関数を毎フレーム実行する必要がある。
    void DrawText_(const int fontId,
                   const std::wstring& text,
                   const int X,
                   const int Y);

    void DrawText_(const int fontId,
                   const std::wstring& text,
                   const int X,
                   const int Y,
                   const UINT color);

    void DrawTextEx(const int fontId,
                    const std::wstring& text,
                    const int X,
                    const int Y);

    void DrawTextEx(const int fontId,
                    const std::wstring& text,
                    const int X,
                    const int Y,
                    const UINT color);

    void DrawTextCenter(const int fontId,
                        const std::wstring& text,
                        const int X,
                        const int Y,
                        const int Width,
                        const int Height);

    void DrawTextCenter(const int fontId,
                        const std::wstring& text,
                        const int X,
                        const int Y,
                        const int Width,
                        const int Height,
                        const UINT color);

    void DrawImage(const std::wstring& text,
                   const int X,
                   const int Y,
                   const int transparency = 255);

    void PlaceParticleEffect(const ParticleEffectPreset preset, const D3DXVECTOR3& origin);
    void ClearParticleEffect();
    void SetDustFixedScreenSize(bool enabled);
    ParticleEffectPreset GetParticleEffectPreset() const;

    // 彩度をどれくらい上げるか（下げるか）を設定
    void SetPostEffectSaturate(const float level);
    void SetPostEffectSaturateEnable(const bool arg);

    void SetPostEffectGaussianFilter(const bool arg);
    void SetPostEffectGaussianSampleSize(const int sampleSize);
    void SetPostEffectGaussianStrength(const float strength);
    void SetPostEffectFontSampleSize(const int sampleSize);
    void SetPostEffectMaskedGaussianFilter(const bool arg);
    void SetPostEffectMaskedGaussianSampleSize(const int sampleSize);
    void SetPostEffectMaskedGaussianMaskPath(const std::wstring& maskPath);
    void SetPostEffectAA(const bool arg);
    void SetPostEffectFXAA(const bool arg);
    void SetPostEffectFXAAQuality(const int quality);
    void SetPostEffectTAA(const bool arg);
    void SetPostEffectTAAHistoryWeight(const float historyWeight);
    void SetPostEffectMotionBlurCamera(const bool arg);
    void SetPostEffectMotionBlurCameraQuality(const int quality);
    void SetPostEffectMotionBlurCameraMaxBlurPixels(const float maxBlurPixels);
    void SetPostEffectMotionBlurCameraSampleCount(const int sampleCount);
    void SetPostEffectDepthBufferShadow(const bool arg);
    void SetPostEffectDepthBufferShadowIntensity(const float intensity);
    void SetPostEffectDepthBufferShadowSaturationBoost(const float saturationBoost);
    void SetPostEffectDepthBufferShadowCoverage(const float coverage);
    void SetPostEffectDepthBufferShadowPcfTapCount(const int tapCount);
    void SetPostEffectDepthBufferShadowCompositeTapCount(const int tapCount);
    void SetPostEffectDepthBufferShadowTexSizeDivisor(const int scaleDivisor);
    void SetPostEffectSSAO(const bool arg);
    void SetPostEffectSSAOBlur(const bool arg);
    void SetPostEffectSSAOSeparableBlur(const bool enabled);
    void SetPostEffectSSAOShadowStrength(const float shadowStrength);
    void SetPostEffectSSAOSaturationBoost(const float saturationBoost);
    void SetPostEffectSSAOSampleCount(const int sampleCount);
    void SetPostEffectSSAORandomSamplingDirection(const bool enabled);
    void SetPostEffectSSAODepthScaledSampleDistance(const bool enabled);
    void SetPostEffectSSAOSampleRadius(const float sampleRadius);
    void SetPostEffectSSAOBlurKernelSize(const int kernelSize);
    void SetPostEffectSSAOTexSizeDivisor(const int scaleDivisor);
    void SetPostEffectSSAOCompositeGaussian3x3(const bool enabled);
    void SetPostEffectSSAOMaxDarknessClamp(const bool enabled);
    void SetPostEffectSSGI(const bool arg);
    void SetPostEffectSSGIBlur(const bool arg);
    void SetPostEffectSSGISeparableBlur(const bool enabled);
    void SetPostEffectSSGISampleCount(const int sampleCount);
    void SetPostEffectSSGIDepthScaledSampleDistance(const bool enabled);
    void SetPostEffectSSGISampleRadius(const float sampleRadius);
    void SetPostEffectSSGIBlurKernelSize(const int kernelSize);
    void SetPostEffectSSGITexSizeDivisor(const int scaleDivisor);
    void SetPostEffectSSGIIndirectLightStrength(const float strength);
    void SetPostEffectSSGIIndirectLightMaxContribution(const float maxContribution);
    void SetPostEffectSSGIUseThickness(const bool enabled);
    void SetPostEffectFog(const bool arg);
    void SetPostEffectFogIntensity(const float intensity);
    void SetPostEffectFogColor(const D3DXCOLOR& color);
    void SetPostEffectHeightFog(const bool arg);
    void SetPostEffectHeightFogIntensity(const float intensity);
    void SetPostEffectHeightFogStart(const float start);
    void SetPostEffectHeightFogMax(const float maxHeight);
    void SetPostEffectHeightFogDistanceStart(const float distanceStart);
    void SetPostEffectHeightFogDistanceMax(const float distanceMax);
    void SetPostEffectFogHeightEnable(const bool arg);
    void SetPostEffectFogHeightIntensity(const float intensity);
    void SetPostEffectFogHeightStart(const float start);

    void SetPostEffectBloom(const bool arg);
    void SetPostEffectBloomThreshold(const float threshold);
    void SetPostEffectBloomWeightSum(const float weightSum);
    void SetPostEffectHalo(const bool arg);
    void SetPostEffectHaloThreshold(const float threshold);
    void SetPostEffectDepthOfField(const bool arg);
    void SetPostEffectDepthOfFieldMode(const DepthOfFieldMode mode);
    void SetPostEffectDepthOfFieldFocalDistance(const float distance);
    void SetPostEffectDepthOfFieldStartNear(const float distance);
    void SetPostEffectDepthOfFieldMaxBlurDistance(const float distance);
    void SetPostEffectDepthOfFieldAutoActivationDistance(const float distance);

    void SetPostEffectStarBurst(const bool arg);
    void SetPostEffectStarBurstThreshold(const float threshold);
    void SetPostEffectStarBurstDistanceFade(const float fade);

    void SetPostEffectGodRay(const bool arg);
    void SetPostEffectGodRayLightPos(const D3DXVECTOR3& pos);
    void SetPostEffectGodRayReverseSampling(const bool arg);
    void SetPostEffectGodRayRayLength(const float arg);
    void SetPostEffectGodRayIntensity(const float arg);
    void SetPostEffectGodRayVirtualProximityStrength(const float arg);
    void SetPostEffectGodRayOcclusionFalloff(const float arg);
    void SetPostEffectGodRayLightColor(const D3DXVECTOR3& color);
    void SetDebugGBufferView(const DebugGBufferView view);

    void SetShowFPS(const bool arg);
    void SetShowCameraPosition(const bool arg);

    std::vector<std::pair<int, int>> GetResolutionList();

    // 平行光源がある方角
    void SetLightDir(const D3DXVECTOR3& dir);
    void SetLightColor(const D3DXCOLOR& color);
    void SetLightBrightness(const float brightness);
    void SetAmbientLightColor(const D3DXCOLOR& color);
    void SetAmbientLightBrightness(const float brightness);
    void SetMeshMixSSS(const bool enabled);
    void SetMeshMixSSSIntensity(const float intensity);
    void SetMeshMixSSSColor(const DWORD color);

    void AddPointLight(const D3DXVECTOR3& pos,
                       const float brightness,
                       const D3DXCOLOR color,
                       const PointLightShape shape = PointLightShape::Point,
                       const float lineLength = 12.0f,
                       const float squareWidth = 10.0f,
                       const float squareHeight = 10.0f,
                       const D3DXVECTOR3& rotation = D3DXVECTOR3(0.f, 0.f, 0.f));

private:

    HWND m_hWnd = NULL;

    // ウィンドウ管理
    WindowManager m_windowManager;

    void ChangeWindowMode();

    std::deque<MeshOld> m_meshList;
    std::vector<bool> m_meshEnabledList;
    std::vector<AnimMesh*> m_animMeshList;
    std::vector<SkinAnimMesh*> m_skinAnimMeshList;
    std::vector<MeshMixSkinAnim*> m_meshMixSkinAnimList;
    std::deque<MeshSmooth> m_meshSmoothList;
    std::deque<MeshSSSLike> m_meshSSSLikeList;
    std::deque<MeshSSS> m_meshSSSList;
    std::vector<bool> m_meshSSSEnabledList;
    std::deque<MeshPointLight> m_meshPointLightList;
    std::vector<bool> m_meshPointLightEnabledList;
    std::deque<MeshNormalMapping> m_meshNormalMapList;
    std::vector<bool> m_meshNormalMapEnabledList;
    std::deque<MeshPOM> m_meshPOMList;
    std::vector<bool> m_meshPOMEnabledList;

    std::deque<MeshMixManager> m_meshMixList;
    std::deque<MeshPBRManager> m_meshPBRList;

    std::unordered_map<std::wstring, MeshInstancing*> m_meshInstancingMap;
    bool m_meshInstancingHighQualityEnabled = true;

    // ポインターにしないとデバイスロストを扱う機能が機能しなくなる
    std::vector<Font*> m_fontList;
    std::vector<FontEx*> m_fontExList;
    Sprite m_sprite;
    ParticleSystem m_particleSystem;

    //---------------------------------------------------------------
    // マルチパスレンダリング関連
    //---------------------------------------------------------------

    void DrawPass1(const bool renderToSceneRenderTargets = true,
                   int activeMirrorMeshIndex = -1);
    void DrawSceneGeometry(int activeMirrorMeshIndex,
                           bool renderActiveMirrorAsMirror,
                           int skippedMeshMixIndex = -1);
    int FindActiveMirrorMeshIndex() const;
    bool RenderMirrorTexture(int activeMirrorMeshIndex);
    void ApplyTAAProjectionJitter();
    void ClearTAAProjectionJitter();

    // 各ピクセルの深度とワールド座標を表した画像を生成
    GBuffer m_GBuffer;

    // 深度バッファシャドウ
    PostEffectZShadow m_postEffectZShadow;

    // SSAO
    PostEffectSSAO m_postEffectSSAO;

    // SSGI
    PostEffectSSGI m_postEffectSSGI;

    // 霧
    PostEffectFog m_postEffectFog;
    PostEffectHeightFog m_postEffectHeightFog;

    // 彩度フィルター
    PostEffectSaturate m_postEffectSaturate;

    // ブルームフィルター
    PostEffectBloom m_PostEffectBloom;

    // ハロー
    PostEffectHalo m_postEffectHalo;

    // 被写界深度
    PostEffectDepthOfField m_postEffectDepthOfField;

    // ガウスフィルター
    PostEffectGauss m_postEffectGauss;
    PostEffectMaskedGauss m_postEffectMaskedGauss;
    PostEffectAA m_postEffectAA;

    // FXAA
    PostEffectFXAA m_postEffectFXAA;

    // TAA
    PostEffectTAA m_postEffectTAA;

    // カメラモーションブラー
    PostEffectMotionBlurCamera m_postEffectMotionBlurCamera;

    // スターバースト
    PostEffectStarBurst m_postEffectStarBurst;

    // テクスチャ―の内容を画面に出力
    PostEffectEnd m_postEffectEnd;

    // ゴッドレイ
    PostEffectGodRay m_postEffectGodRay;

    void Draw2D();
    void UpdateSkinAnimationState();
    void LoadSettingsCsv(const std::wstring& settingsCsvPath);
    void ApplySettings();
    void EnsureGBufferInitialized();
    void EnsurePostEffectSaturateInitialized();
    void EnsurePostEffectGaussInitialized();
    void EnsurePostEffectMaskedGaussInitialized();
    void EnsurePostEffectAAInitialized();
    void EnsurePostEffectFXAAInitialized();
    void EnsurePostEffectTAAInitialized();
    void EnsurePostEffectMotionBlurCameraInitialized();
    void EnsurePostEffectZShadowInitialized();
    void EnsurePostEffectSSAOInitialized();
    void EnsurePostEffectSSGIInitialized();
    void EnsurePostEffectFogInitialized();
    void EnsurePostEffectHeightFogInitialized();
    void EnsurePostEffectBloomInitialized();
    void EnsurePostEffectHaloInitialized();
    void EnsurePostEffectDepthOfFieldInitialized();
    void EnsurePostEffectStarBurstInitialized();
    void EnsurePostEffectGodRayInitialized();
    static void SwapPostEffectBuffers(LPDIRECT3DTEXTURE9& texSource,
                                      LPDIRECT3DTEXTURE9& texTarget);
    static void CopyTexture(LPDIRECT3DTEXTURE9 texSource,
                            LPDIRECT3DTEXTURE9 texTarget);
    static std::wstring Trim(const std::wstring& text);
    static int NormalizeGaussianSampleSize(const int sampleSize);
    static int NormalizeFontGaussianSampleSize(const int sampleSize);

    std::unordered_map<std::wstring, std::wstring> m_settings;
    int m_gaussianSampleSize = 101;
    float m_gaussianStrength = 1.0f;
    int m_fontExGaussianSampleSize = 21;
    std::wstring m_maskedGaussianMaskPath;
    int m_fxaaQuality = 4;
    float m_taaHistoryWeight = 0.85f;
    int m_motionBlurCameraQuality = 4;
    float m_motionBlurCameraMaxBlurPixels = 24.0f;
    int m_motionBlurCameraSampleCount = 13;
    RenderingQualitySettings m_renderingQualitySettings;
    bool m_postEffectSaturateEnabled = false;
    bool m_postEffectGaussEnabled = false;
    bool m_postEffectMaskedGaussEnabled = false;
    bool m_postEffectAAEnabled = false;
    bool m_postEffectFXAAEnabled = false;
    bool m_postEffectTAAEnabled = false;
    bool m_postEffectMotionBlurCameraEnabled = false;
    unsigned int m_taaFrameIndex = 0;
    bool m_gBufferEnabled = true;
    bool m_postEffectZShadowEnabled = true;
    bool m_postEffectSSAOEnabled = true;
    bool m_postEffectSSGIEnabled = false;
    bool m_postEffectFogZEnabled = true;
    bool m_postEffectFogHeightEnabled = false;
    bool m_postEffectBloomEnabled = false;
    bool m_postEffectHaloEnabled = false;
    DepthOfFieldMode m_postEffectDepthOfFieldMode = DepthOfFieldMode::Disabled;
    bool m_postEffectStarBurstEnabled = false;
    bool m_postEffectGodRayEnabled = false;
    DebugGBufferView m_debugGBufferView = DebugGBufferView::None;
    bool m_meshMixSaturateShadowEnabled = false;
    float m_meshMixSaturateShadowIntensity = 0.2f;
    float m_meshMixShadowDarkness = 1.0f;
    float m_meshMixSpecularIntensity = 0.0f;
    float m_meshMixSpecularEdge = 0.0f;
    float m_meshMixFresnelIntensity = 0.08f;
    float m_meshMixCubeMappingRate = 1.0f;
    float m_meshPBRRoughness = 0.85f;
    float m_meshPBRMetallic = 0.0f;
    float m_meshPBREnvReflectionIntensity = 0.05f;
    float m_meshPBREnvMaxMipLevel = 5.0f;
    float m_meshPBREnvDiffuseIntensity = 0.8f;
    float m_meshPBREnvDiffuseMipLevel = 3.0f;
    bool m_meshMixSpecularIntensityOverrideEnabled = true;
    bool m_meshMixSpecularEdgeOverrideEnabled = true;
    bool m_phongTreatTextureAsWhiteEnabled = false;
    bool m_meshMixSkinAnimAlphaClipEnabled = true;
    bool m_meshMixSSSEnabled = false;
    float m_meshMixSSSIntensity = 1.0f;
    DWORD m_meshMixSSSColor = 0xffff80;

    LPDIRECT3DTEXTURE9 m_pRenderTarget1 = NULL;
    LPDIRECT3DTEXTURE9 m_pRenderTarget2 = NULL;
    LPDIRECT3DTEXTURE9 m_pLightEffectSourceTexture = NULL;
    LPDIRECT3DTEXTURE9 m_pMirrorRenderTarget = NULL;

    //-----------------------------------------------------------------
    // FPS表示
    //-----------------------------------------------------------------

    bool m_bShowFPS = true;
    bool m_bShowCameraPosition = false;

    float CalcFPS();
    void ShowFPS(const float arg);
    void ShowCameraPosition();
    float CalcFrameDeltaSeconds();

    int m_fontID = -1;
    int m_cameraPositionFontId = -1;
    std::vector<std::chrono::steady_clock::time_point> m_vecTime;
    std::chrono::steady_clock::time_point m_lastFrameTime {};
    bool m_hasLastFrameTime = false;

    //-----------------------------------------------------------------
    // デバイスロスト関連処理
    //-----------------------------------------------------------------

    void OnDeviceLost();

    void OnDeviceReset();

    void CreateTexture();

};
}

