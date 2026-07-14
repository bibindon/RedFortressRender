#pragma once

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
#include "FontExAnim.h"
#include "Sprite.h"
#include "ParticleSystem.h"
#include "LoadingScreen.h"

#include "MeshOld.h"
#include "MeshSmooth.h"
#include "MeshSSSLike.h"
#include "MeshPointLight.h"
#include "MeshNormalMapping.h"
#include "MeshMix.h"
#include "MeshMixManager.h"
#include "MeshPBRManager.h"
#include "MeshMixSkinAnim.h"
#include "MeshMixSkinAnim2.h"
#include "MeshMixAnimNoBone.h"
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
#include "RenderSettingsDialog.h"

namespace NSRender
{

struct RenderFrameProfile
{
    double sceneUpdateMilliseconds = 0.0;
    double gBufferMilliseconds = 0.0;
    double mirrorMilliseconds = 0.0;
    double mainPassMilliseconds = 0.0;
    double postEffectMilliseconds = 0.0;
    double draw2DMilliseconds = 0.0;
    double frameWaitMilliseconds = 0.0;
    double presentMilliseconds = 0.0;
    double totalMilliseconds = 0.0;
};

enum class DebugGBufferView
{
    None = 0,
    WorldPos = 1,
    Normal = 2,
    Depth = 3,
    Thickness = 4,
    BackDepth = 5,
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

enum class RenderLoadedModelType
{
    MeshMix,
    MeshPBR,
    MeshInstancing,
    MeshMixSkinAnim
};

struct RenderLoadedModelInfo
{
    RenderLoadedModelType type = RenderLoadedModelType::MeshMix;
    int renderId = -1;
    std::wstring filePath;
    float scale = 1.0f;
    D3DXVECTOR3 pos = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
};

struct RenderSettingsDialogTextInfo
{
    std::wstring text;
    float x = 0.0f;
    float y = 0.0f;
    bool decorated = false;
};

struct WorldTextInfo
{
    std::wstring text;
    D3DXVECTOR3 worldPos = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    int fontSize = 20;
    D3DXCOLOR color = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
    bool decorated = false;
};

enum class BlinkMode
{
    WhiteFlash,
    Visibility,
    StarFlash,
    PinkWhiteFlash,
    CyanWhiteFlash
};

struct BoneAttachment
{
    int childMeshId = -1;
    int parentSkinnedMeshId = -1;
    std::string boneName;
    D3DXVECTOR3 localRotate = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 localOffset = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
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
    HWND GetWindowHandle() const;
    void ShowSettingsDialog(bool activateDialog = true);
    void ToggleSettingsDialog();

    void ChangeResolution(const int W, const int H);

    void ChangeWindowMode(const eWindowMode eWindowMode_);
    eWindowMode GetWindowMode() const;

    int AddMesh_deprecated(const std::wstring& filePath,
                           const D3DXVECTOR3& pos,
                           const D3DXVECTOR3& rot,
                           const float scale,
                           const float radius = -1.f,
                           const float uvTile = 1.0f);
    bool RemoveMesh(int id);
    void SetMeshWorldMatrix(int id, const D3DXMATRIX& mat);
    void SetMeshEnabled(int id, bool enabled);

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
                          const float scale,
                          const std::wstring& csvPath = L"");
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
    bool LoadXFileListFromCsv(const std::wstring& csvPath,
                              const float scale = 1.0f,
                              int* loadedCount = nullptr,
                              int* skippedCount = nullptr);
    void ClearCsvLoadedMeshes();

    // すべての非同期メッシュ読み込みが完了したかどうかを確認する。
    bool IsAllMeshLoaded() const;

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
                           const bool useNormalMapping = false,
                           const MeshMixSkinAnimLoadMode loadMode = MeshMixSkinAnimLoadMode::DirectX);
    int AddMeshMixSkinAnim(const std::wstring& meshFilePath,
                           const std::wstring& animationFilePath,
                           const D3DXVECTOR3& pos,
                           const D3DXVECTOR3& rot,
                           const float scale,
                           const AnimSetMap& animSetMap,
                           const float radius = -1.f,
                           const bool useParallaxOcclusionMapping = false,
                           const bool useNormalMapping = false,
                           const MeshMixSkinAnimLoadMode loadMode = MeshMixSkinAnimLoadMode::DirectX);
    int AddMeshMixSkinAnim2(const std::wstring& filePath,
                            const D3DXVECTOR3& pos,
                            const D3DXVECTOR3& rot,
                            const float scale,
                            const AnimSetMap& animSetMap,
                            const float radius = -1.f,
                            const bool useParallaxOcclusionMapping = false,
                            const bool useNormalMapping = false);
    int AddMeshMixSkinAnim2(const std::wstring& meshFilePath,
                            const std::wstring& animationFilePath,
                            const D3DXVECTOR3& pos,
                            const D3DXVECTOR3& rot,
                            const float scale,
                            const AnimSetMap& animSetMap,
                            const float radius = -1.f,
                            const bool useParallaxOcclusionMapping = false,
                            const bool useNormalMapping = false);
    bool RemoveMeshMixSkinAnim(int id);
    const std::vector<MeshMixSkinAnimAnimationInfo>* GetMeshMixSkinAnimAnimationInfoList(int id) const;
    bool PlayMeshMixSkinAnimAnimation(int id, const std::wstring& name);
    void SetMeshMixSkinAnimSpeed(int id, float speed);
    void SetMeshMixSkinAnimAlphaClip(const bool enabled);
    void SetMeshMixSkinAnimIgnoreTransparentMaterial(const bool enabled);
    void SetMeshMixSkinAnimPos(const int id, const D3DXVECTOR3& pos);
    void SetMeshMixSkinAnimRotY(const int id, const float rotY);
    void SetMeshMixSkinAnimScale(const int id, const float scale);
    void StartMeshMixSkinAnimBlink(int id, int durationFrames, int intervalFrames = 4,
                                   BlinkMode mode = BlinkMode::WhiteFlash);
    void StopMeshMixSkinAnimBlink(int id);
    void SetMeshMixSkinAnimEnabled(int id, bool enabled);
    bool IsMeshMixSkinAnimEnabled(int id) const;
    std::vector<RenderLoadedModelInfo> GetLoadedModelInfoList();

    int AddMeshMixAnimNoBone(const std::wstring& filePath,
                             const D3DXVECTOR3& pos,
                             const D3DXVECTOR3& rot,
                             const float scale,
                             const AnimSetMap& animSetMap = AnimSetMap(),
                             const float radius = -1.f,
                             const MeshMixSkinAnimLoadMode loadMode = MeshMixSkinAnimLoadMode::DirectX);
    bool RemoveMeshMixAnimNoBone(int id);
    void SetMeshMixAnimNoBonePos(const int id, const D3DXVECTOR3& pos);
    void SetMeshMixAnimNoBoneRotY(const int id, const float rotY);
    void SetMeshMixAnimNoBoneScale(const int id, const float scale);
    void SetMeshMixAnimNoBoneEnabled(int id, bool enabled);

    bool LoadXFileListMoveFromCsv(const std::wstring& csvPath,
                                  int* loadedCount = nullptr,
                                  int* skippedCount = nullptr);
    void UpdateMovingPlatforms(float deltaSeconds);
    void ResetMovingPlatforms();
    void RegisterCsvIdMapping(int csvId, int renderId);

    struct MovingPlatform
    {
        int renderId = -1;
        int csvId = -1;
        D3DXVECTOR3 startPos;
        D3DXVECTOR3 endPos;
        float duration = 10.0f;
        float elapsed = 0.0f;
    };
    const std::vector<MovingPlatform>& GetMovingPlatforms() const;

    void SetMeshMixPos(const int id, const D3DXVECTOR3& pos);
    D3DXVECTOR3 GetMeshMixPos(const int id) const;
    void SetMeshMixRotY(const int id, const float rotY);
    void SetMeshMixWorldMatrix(const int id, const D3DXMATRIX& mat);
    void SetMeshMixEnabled(const int id, const bool enabled);
    void SetMeshMixDamageFlash(const int id, const bool enabled);
    void AttachMeshToBone(int childMeshId, int parentSkinnedMeshId, const char* boneName,
                          const D3DXVECTOR3& localRotate = D3DXVECTOR3(0.0f, 0.0f, 0.0f),
                          const D3DXVECTOR3& localOffset = D3DXVECTOR3(0.0f, 0.0f, 0.0f));
    void DetachMeshFromBone(int childMeshId);
    D3DXVECTOR3 GetMeshMixRot(const int id) const;
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
    void SetCameraHorizontalFovDegrees(const float horizontalFovDegrees);
    float GetCameraNearPlane() const;
    float GetCameraFarPlane() const;
    float GetCameraHorizontalFovDegrees() const;
    float GetCameraShakeDuration() const;
    float GetCameraShakeIntensity() const;
    void SetGBufferEnable(const bool enabled);
    void SetGBufferClipPlanes(const float nearPlane, const float farPlane);
    bool IsGBufferEnabled() const;
    float GetGBufferNearPlane() const;
    float GetGBufferFarPlane() const;
    RenderingQualitySettings SetRenderQuality(const std::wstring& quality);
    std::wstring GetRenderQuality() const;

    D3DXVECTOR3 GetLookAtPos();
    D3DXVECTOR3 GetCameraPos();
    D3DXVECTOR3 GetCameraRotate();

    // フォント作成
    // IDが返ってくるので、そのIDを文字描画するときに指定する
    int SetUpFont(const std::wstring& fontName, const int fontSize, const UINT fontColor);
    int SetUpFontEx(const std::wstring& fontName, const int fontSize, const UINT fontColor);
    int SetUpFontExAnim(const std::wstring& fontName, const int fontSize, const UINT fontColor);

    void StartTextExAnim(const int fontId,
                         const std::wstring& text,
                         const int X,
                         const int Y,
                         const int framesPerCharacter = 3);
    void StartTextExAnim(const int fontId,
                         const std::wstring& text,
                         const int X,
                         const int Y,
                         const int framesPerCharacter,
                         const UINT color);
    void FinishTextExAnim(const int fontId);
    bool IsTextExAnimFinished(const int fontId) const;

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

    void DrawTextNormalized(const int fontId,
                            const std::wstring& text,
                            const float X,
                            const float Y);

    void DrawTextNormalized(const int fontId,
                            const std::wstring& text,
                            const float X,
                            const float Y,
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

    void DrawTextExNormalized(const int fontId,
                              const std::wstring& text,
                              const float X,
                              const float Y);

    void DrawTextExNormalized(const int fontId,
                              const std::wstring& text,
                              const float X,
                              const float Y,
                              const UINT color);

    void DrawTextExCenter(const int fontId,
                          const std::wstring& text,
                          const int X,
                          const int Y,
                          const int Width,
                          const int Height);

    void DrawTextExCenter(const int fontId,
                          const std::wstring& text,
                          const int X,
                          const int Y,
                          const int Width,
                          const int Height,
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

    void AddSettingsDialogText(const std::wstring& text,
                               const float X,
                               const float Y,
                               const bool decorated);
    std::vector<RenderSettingsDialogTextInfo> GetSettingsDialogTextList() const;
    bool SetSettingsDialogTextPosition(const size_t index,
                                       const float X,
                                       const float Y);

    int AddWorldText(const std::wstring& text,
                     const D3DXVECTOR3& worldPos,
                     const int fontSize,
                     const D3DXCOLOR& color,
                     const bool decorated = false);
    bool RemoveWorldText(int index);
    void ClearWorldTexts();
    const std::vector<WorldTextInfo>& GetWorldTextList() const;

    // 1 フレームだけ表示されるワールドテキスト。
    // 呼び出しフレームの Draw() 内で一度だけ描画され、次のフレームには残らない。
    void DrawWorldText(const std::wstring& text,
                       const D3DXVECTOR3& worldPos,
                       const int fontSize,
                       const D3DXCOLOR& color,
                       const bool decorated = false);

    void DrawImage(const std::wstring& text,
                   const int X,
                   const int Y,
                   const int transparency = 255);

    void DrawImageSized(const std::wstring& filename,
                        const int X,
                        const int Y,
                        const int width,
                        const int height,
                        const int transparency = 255);

    void DrawImageSizedRect(const std::wstring& filename,
                            const int X,
                            const int Y,
                            const int width,
                            const int height,
                            const int sourceX,
                            const int sourceY,
                            const int sourceWidth,
                            const int sourceHeight,
                            const int transparency = 255);

    void DrawImageStretched(const std::wstring& filename,
                            const int transparency = 255);

    void DrawImageStretchedScaled(const std::wstring& filename,
                                  const float scale,
                                  const int transparency = 255);

    void DrawImageEx(const std::wstring& filename,
                     const int centerX,
                     const int centerY,
                     const int transparency,
                     const bool flipX,
                     const float scale);

    void DrawImageAutoResize(const std::wstring& text,
                             const float X,
                             const float Y,
                             const int transparency = 255);

    void DrawImageAutoResizeEx(const std::wstring& text,
                               const float X,
                               const float Y,
                               const float scale,
                               const bool flipX,
                               const int transparency = 255);

    void DrawImageAutoResizeSizedRect(const std::wstring& filename,
                                      const float X,
                                      const float Y,
                                      const int sourceX,
                                      const int sourceY,
                                      const int sourceWidth,
                                      const int sourceHeight,
                                      const float scale = 1.0f,
                                      const int transparency = 255);

    void DrawWorldImage(const std::wstring& filename,
                        const D3DXVECTOR3& worldPos,
                        const int transparency = 255);

    void LoadImage(const std::wstring& filename);
    SIZE GetImageSize(const std::wstring& filename);

    void StartFadeIn(const float durationSeconds);
    void StartFadeOut(const float durationSeconds);
    void SetFadeAlpha(const float alpha);
    float GetFadeAlpha() const;
    void StartLoadingScreen();
    void EndLoadingScreen();
    void SetLoadingScreenTitle(const std::wstring& title);
    void SetLoadingScreenTitleFontPath(const std::wstring& fontPath);
    void SetLoadingScreenProgress(int progress);
    void SetLoadingScreenShowTitle(bool show);

    void PlaceParticleEffect(const ParticleEffectPreset preset, const D3DXVECTOR3& origin);
    void PlaceDashParticleEffect(const D3DXVECTOR3& origin, const D3DXVECTOR3& direction);
    void ClearParticleEffect();
    void SetDustFixedScreenSize(bool enabled);
    ParticleEffectPreset GetParticleEffectPreset() const;
    void SetExplosionScale(float scale);
    float GetExplosionScale() const;
    void SetDamageScale(float scale);
    float GetDamageScale() const;

    // 彩度をどれくらい上げるか（下げるか）を設定
    void SetPostEffectSaturate(const float level);
    void SetPostEffectSaturateEnable(const bool arg);
    float GetPostEffectSaturate() const;
    bool IsPostEffectSaturateEnabled() const;

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
    bool IsPostEffectGaussianFilterEnabled() const;
    int GetPostEffectGaussianSampleSize() const;
    float GetPostEffectGaussianStrength() const;
    int GetPostEffectFontSampleSize() const;
    bool IsPostEffectMaskedGaussianFilterEnabled() const;
    std::wstring GetPostEffectMaskedGaussianMaskPath() const;
    bool IsPostEffectAAEnabled() const;
    bool IsPostEffectFXAAEnabled() const;
    int GetPostEffectFXAAQuality() const;
    bool IsPostEffectTAAEnabled() const;
    float GetPostEffectTAAHistoryWeight() const;
    bool IsPostEffectMotionBlurCameraEnabled() const;
    float GetPostEffectMotionBlurCameraMaxBlurPixels() const;
    int GetPostEffectMotionBlurCameraSampleCount() const;
    void SetPostEffectDepthBufferShadow(const bool arg);
    void SetPostEffectDepthBufferShadowIntensity(const float intensity);
    void SetPostEffectDepthBufferShadowSaturationBoost(const float saturationBoost);
    void SetPostEffectDepthBufferShadowCoverage(const float coverage);
    void SetPostEffectDepthBufferShadowCoverageFar(const float coverage);
    void SetPostEffectDepthBufferShadowBias(const float shadowBias);
    void SetPostEffectDepthBufferShadowPcfTapCount(const int tapCount);
    void SetPostEffectDepthBufferShadowCompositeTapCount(const int tapCount);
    void SetPostEffectDepthBufferShadowTexSizeDivisor(const int scaleDivisor);
    void SetPostEffectDepthBufferShadowDebugLightDepth(const bool enabled);
    bool IsPostEffectDepthBufferShadowEnabled() const;
    bool IsPostEffectDepthBufferShadowDebugLightDepthEnabled() const;
    float GetPostEffectDepthBufferShadowIntensity() const;
    float GetPostEffectDepthBufferShadowSaturationBoost() const;
    float GetPostEffectDepthBufferShadowCoverage() const;
    float GetPostEffectDepthBufferShadowCoverageFar() const;
    float GetPostEffectDepthBufferShadowBias() const;
    int GetPostEffectDepthBufferShadowPcfTapCount() const;
    int GetPostEffectDepthBufferShadowCompositeTapCount() const;
    int GetPostEffectDepthBufferShadowTexSizeDivisor() const;
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
    bool IsPostEffectSSAOEnabled() const;
    bool IsPostEffectSSAOBlurEnabled() const;
    bool IsPostEffectSSAOSeparableBlurEnabled() const;
    float GetPostEffectSSAOShadowStrength() const;
    float GetPostEffectSSAOSaturationBoost() const;
    int GetPostEffectSSAOSampleCount() const;
    bool IsPostEffectSSAORandomSamplingDirectionEnabled() const;
    bool IsPostEffectSSAODepthScaledSampleDistanceEnabled() const;
    float GetPostEffectSSAOSampleRadius() const;
    int GetPostEffectSSAOBlurKernelSize() const;
    int GetPostEffectSSAOTexSizeDivisor() const;
    bool IsPostEffectSSAOCompositeGaussian3x3Enabled() const;
    bool IsPostEffectSSAOMaxDarknessClampEnabled() const;
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
    bool IsPostEffectSSGIEnabled() const;
    bool IsPostEffectSSGIBlurEnabled() const;
    bool IsPostEffectSSGISeparableBlurEnabled() const;
    int GetPostEffectSSGISampleCount() const;
    bool IsPostEffectSSGIDepthScaledSampleDistanceEnabled() const;
    float GetPostEffectSSGISampleRadius() const;
    int GetPostEffectSSGIBlurKernelSize() const;
    int GetPostEffectSSGITexSizeDivisor() const;
    float GetPostEffectSSGIIndirectLightStrength() const;
    float GetPostEffectSSGIIndirectLightMaxContribution() const;
    bool IsPostEffectSSGIUseThicknessEnabled() const;
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
    bool IsPostEffectFogEnabled() const;
    float GetPostEffectFogIntensity() const;
    D3DXCOLOR GetPostEffectFogColor() const;
    bool IsPostEffectHeightFogEnabled() const;
    float GetPostEffectHeightFogIntensity() const;
    float GetPostEffectHeightFogStart() const;
    float GetPostEffectHeightFogMax() const;
    float GetPostEffectHeightFogDistanceStart() const;
    float GetPostEffectHeightFogDistanceMax() const;

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
    bool IsPostEffectBloomEnabled() const;
    float GetPostEffectBloomThreshold() const;
    float GetPostEffectBloomWeightSum() const;
    bool IsPostEffectHaloEnabled() const;
    float GetPostEffectHaloThreshold() const;
    DepthOfFieldMode GetPostEffectDepthOfFieldMode() const;
    float GetPostEffectDepthOfFieldFocalDistance() const;
    float GetPostEffectDepthOfFieldStartNear() const;
    float GetPostEffectDepthOfFieldMaxBlurDistance() const;
    float GetPostEffectDepthOfFieldAutoActivationDistance() const;

    void SetPostEffectStarBurst(const bool arg);
    void SetPostEffectStarBurstThreshold(const float threshold);
    void SetPostEffectStarBurstDistanceFade(const float fade);
    bool IsPostEffectStarBurstEnabled() const;
    float GetPostEffectStarBurstThreshold() const;
    float GetPostEffectStarBurstDistanceFade() const;

    void SetPostEffectGodRay(const bool arg);
    void SetPostEffectGodRayLightPos(const D3DXVECTOR3& pos);
    void SetPostEffectGodRayReverseSampling(const bool arg);
    void SetPostEffectGodRayRayLength(const float arg);
    void SetPostEffectGodRayIntensity(const float arg);
    void SetPostEffectGodRayVirtualProximityStrength(const float arg);
    void SetPostEffectGodRayOcclusionFalloff(const float arg);
    void SetPostEffectGodRayLightColor(const D3DXVECTOR3& color);
    void SetDebugGBufferView(const DebugGBufferView view);
    bool IsPostEffectGodRayEnabled() const;
    D3DXVECTOR3 GetPostEffectGodRayLightPos() const;
    float GetPostEffectGodRayIntensity() const;
    float GetPostEffectGodRayVirtualProximityStrength() const;
    D3DXVECTOR3 GetPostEffectGodRayLightColor() const;
    DebugGBufferView GetDebugGBufferView() const;

    void SetShowFPS(const bool arg);
    bool IsShowFPS() const;
    void SetSkinAnimationUpdateEnabled(bool enabled);
    const RenderFrameProfile& GetLastFrameProfile() const;
    void SetShowCameraPosition(const bool arg);
    void SetSceneUpdatePaused(const bool paused);
    bool IsSceneUpdatePaused() const;

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
    D3DXCOLOR GetLightColor() const;
    float GetLightBrightness() const;
    D3DXCOLOR GetAmbientLightColor() const;
    float GetAmbientLightBrightness() const;
    bool IsMeshMixSSSEnabled() const;
    float GetMeshMixSSSIntensity() const;
    float GetMeshMixSaturateShadowIntensity() const;
    float GetMeshMixShadowDarkness() const;
    float GetMeshMixSpecularIntensity() const;
    float GetMeshMixSpecularEdge() const;
    float GetMeshMixFresnelIntensity() const;
    float GetMeshMixEnvMapBlend() const;
    bool IsMeshMixSpecularIntensityOverrideEnabled() const;
    bool IsMeshMixSpecularEdgeOverrideEnabled() const;
    bool IsPhongTreatTextureAsWhiteEnabled() const;
    bool IsMeshMixSkinAnimAlphaClipEnabled() const;
    bool IsMeshMixSkinAnimIgnoreTransparentMaterialEnabled() const;
    float GetMeshPBRRoughness() const;
    float GetMeshPBRMetallic() const;
    float GetMeshPBREnvReflectionIntensity() const;
    float GetMeshPBREnvMaxMipLevel() const;
    float GetMeshPBREnvDiffuseIntensity() const;
    float GetMeshPBREnvDiffuseMipLevel() const;
    std::wstring GetMeshPBREnvMapTexturePath() const;

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
    RenderSettingsDialog m_settingsDialog;

    // ウィンドウ管理
    WindowManager m_windowManager;

    void ChangeWindowMode();
    bool IsMeshMixSlotUsed(int id) const;

    std::deque<MeshOld> m_meshList;
    std::vector<bool> m_meshEnabledList;
    std::vector<BoneAttachment> m_boneAttachments;
    std::vector<AnimMesh*> m_animMeshList;
    std::vector<SkinAnimMesh*> m_skinAnimMeshList;
    std::vector<IMeshMixSkinAnim*> m_meshMixSkinAnimList;
    std::vector<MeshMixAnimNoBone*> m_meshMixAnimNoBoneList;
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
    std::vector<bool> m_meshMixSlotUsedList;
    std::deque<MeshPBRManager> m_meshPBRList;

    std::unordered_map<std::wstring, MeshInstancing*> m_meshInstancingMap;
    bool m_meshInstancingHighQualityEnabled = false;

    std::unordered_map<int, int> m_csvIdToRenderId;
    std::vector<std::wstring> m_csvInstancingFilePaths;
    std::vector<int> m_csvSkinAnimRenderIds;
    std::vector<MovingPlatform> m_movingPlatforms;
    bool m_sceneUpdatePaused = false;

    // ポインターにしないとデバイスロストを扱う機能が機能しなくなる
    std::vector<Font*> m_fontList;
    std::vector<FontEx*> m_fontExList;
    std::vector<FontExAnim*> m_fontExAnimList;
    std::vector<RenderSettingsDialogTextInfo> m_settingsDialogTextList;
    std::vector<WorldTextInfo> m_worldTextList;
    std::vector<WorldTextInfo> m_pendingWorldTexts;

    struct MeshMixSkinAnimBlinkInfo
    {
        int meshId = -1;
        int remainingFrames = 0;
        int intervalFrames = 4;
        BlinkMode mode = BlinkMode::WhiteFlash;
    };
    std::vector<MeshMixSkinAnimBlinkInfo> m_meshMixSkinAnimBlinkList;

    int m_worldTextFontId = -1;
    std::unordered_map<int, int> m_worldTextFontIdBySize;
    std::unordered_map<int, int> m_worldTextFontExIdBySize;
    int m_settingsDialogTextFontId = -1;
    int m_settingsDialogTextFontExId = -1;
    int m_loadingScreenFontId = -1;
    int m_loadingScreenTitleFontId = -1;
    std::wstring m_loadingScreenTitleFontName = L"BIZ UDゴシック";
    std::wstring m_loadingScreenTitleFontPath;
    bool m_loadingScreenTitleFontRegistered = false;
    Sprite m_sprite;
    LoadingScreen m_loadingScreen;
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
    void EnsureSettingsDialogTextFonts();
    void DrawSettingsDialogText();
    void DrawWorldTexts();
    void DrawWorldTextImpl(const WorldTextInfo& worldText);
    void UpdateMeshMixSkinAnimBlink();
    void EnsureLoadingScreenFont();
    void DrawLoadingScreen();
    void DrawFadeOverlay();
    void UpdateFade(float deltaSeconds);
    void EnsureFadeTexture();
    void UpdateSkinAnimationState();
    void UpdateBoneAttachments();
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
    float m_cameraShakeDurationSeconds = 1.0f;
    float m_cameraShakeIntensity = 0.12f;
    float m_gBufferNearPlane = 0.1f;
    float m_gBufferFarPlane = 30'000.0f;
    RenderingQualitySettings m_renderingQualitySettings;
    float m_postEffectSaturateLevel = 1.0f;
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
    bool m_postEffectZShadowDebugLightDepthEnabled = false;
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
    float m_postEffectDepthBufferShadowIntensity = 0.5f;
    float m_postEffectDepthBufferShadowSaturationBoost = 0.35f;
    float m_postEffectDepthBufferShadowCoverage = 0.05f;
    float m_postEffectDepthBufferShadowCoverageFar = 0.8f;
    float m_postEffectDepthBufferShadowBias = 0.00139f;
    int m_postEffectDepthBufferShadowPcfTapCount = 1;
    int m_postEffectDepthBufferShadowCompositeTapCount = 1;
    int m_postEffectDepthBufferShadowTexSizeDivisor = 1;
    bool m_postEffectSSAOBlurEnabled = true;
    bool m_postEffectSSAOSeparableBlurEnabled = true;
    float m_postEffectSSAOShadowStrength = 1.0f;
    float m_postEffectSSAOSaturationBoost = 0.30f;
    int m_postEffectSSAOSampleCount = 16;
    bool m_postEffectSSAORandomSamplingDirectionEnabled = true;
    bool m_postEffectSSAODepthScaledSampleDistanceEnabled = false;
    float m_postEffectSSAOSampleRadius = 4.0f;
    int m_postEffectSSAOBlurKernelSize = 21;
    int m_postEffectSSAOTexSizeDivisor = 1;
    bool m_postEffectSSAOCompositeGaussian3x3Enabled = false;
    bool m_postEffectSSAOMaxDarknessClampEnabled = true;
    bool m_postEffectSSGIBlurEnabled = true;
    bool m_postEffectSSGISeparableBlurEnabled = true;
    int m_postEffectSSGISampleCount = 16;
    bool m_postEffectSSGIDepthScaledSampleDistanceEnabled = false;
    float m_postEffectSSGISampleRadius = 1.0f;
    int m_postEffectSSGIBlurKernelSize = 21;
    int m_postEffectSSGITexSizeDivisor = 1;
    float m_postEffectSSGIIndirectLightStrength = 1.0f;
    float m_postEffectSSGIIndirectLightMaxContribution = 1.0f;
    bool m_postEffectSSGIUseThicknessEnabled = true;
    float m_postEffectFogIntensity = 2.0f;
    D3DXCOLOR m_postEffectFogColor = D3DXCOLOR(0.72f, 0.78f, 0.86f, 1.0f);
    float m_postEffectHeightFogIntensity = 0.3f;
    float m_postEffectHeightFogStart = 0.0f;
    float m_postEffectHeightFogMax = -5.0f;
    float m_postEffectHeightFogDistanceStart = 0.0f;
    float m_postEffectHeightFogDistanceMax = 20.0f;
    float m_postEffectBloomThreshold = 2.5f;
    float m_postEffectBloomWeightSum = 1.0f;
    float m_postEffectHaloThreshold = 2.5f;
    float m_postEffectDepthOfFieldFocalDistance = 8.0f;
    float m_postEffectDepthOfFieldStartNear = 0.0f;
    float m_postEffectDepthOfFieldMaxBlurDistance = 16.0f;
    float m_postEffectDepthOfFieldAutoActivationDistance = 10.0f;
    float m_postEffectStarBurstThreshold = 2.8f;
    float m_postEffectStarBurstDistanceFade = 0.0f;
    D3DXVECTOR3 m_postEffectGodRayLightPos = D3DXVECTOR3(1000.0f, 100.0f, 1000.0f);
    float m_postEffectGodRayIntensity = 0.1f;
    float m_postEffectGodRayVirtualProximityStrength = 1.5f;
    D3DXVECTOR3 m_postEffectGodRayLightColor = D3DXVECTOR3(1.0f, 0.9f, 0.8f);
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
    std::wstring m_pbrEnvMapPath;
    bool m_meshMixSpecularIntensityOverrideEnabled = false;
    bool m_meshMixSpecularEdgeOverrideEnabled = false;
    bool m_phongTreatTextureAsWhiteEnabled = false;
    bool m_meshMixSkinAnimAlphaClipEnabled = true;
    bool m_meshMixSkinAnimIgnoreTransparentMaterialEnabled = false;
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
    bool m_skinAnimationUpdateEnabled = true;
    RenderFrameProfile m_lastFrameProfile;
    bool m_bShowCameraPosition = false;

    float CalcFPS();
    void ShowFPS(const float arg);
    void ShowCameraPosition();
    float CalcFrameDeltaSeconds();
    void WaitForTargetFrameRate();

    int m_fontID = -1;
    int m_cameraPositionFontId = -1;
    std::vector<std::chrono::steady_clock::time_point> m_vecTime;
    std::chrono::steady_clock::time_point m_lastFrameTime {};
    bool m_hasLastFrameTime = false;
    std::chrono::steady_clock::time_point m_lastFramePacingTime {};
    bool m_hasLastFramePacingTime = false;
    DWORD m_lastSleepMs = 0;
    bool m_hasRequestedTimerResolution = false;

    D3DXVECTOR3 m_fadeColor = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    float m_fadeAlpha = 0.0f;
    float m_fadeStartAlpha = 0.0f;
    float m_fadeTargetAlpha = 0.0f;
    float m_fadeDuration = 0.0f;
    float m_fadeElapsed = 0.0f;
    bool m_fadeActive = false;
    LPDIRECT3DTEXTURE9 m_fadeTexture = NULL;

    //-----------------------------------------------------------------
    // デバイスロスト関連処理
    //-----------------------------------------------------------------

    void OnDeviceLost();

    void OnDeviceReset();

    void CreateTexture();

};
}


