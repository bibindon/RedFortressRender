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
#include "Sprite.h"

#include "MeshOld.h"
#include "MeshSmooth.h"
#include "MeshSSSLike.h"
#include "MeshPointLight.h"
#include "MeshNormalMapping.h"
#include "MeshMix.h"
#include "MeshMixManager.h"
#include "MeshMixSkinAnim.h"
#include "MeshSSS.h"
#include "MeshPOM.h"

#include "AnimMesh.h"
#include "SkinAnimMesh.h"

#include "MeshInstancing.h"

#include "PostEffectGauss.h"
#include "PostEffectSaturate.h"
#include "PostEffectBloom.h"
#include "PostEffectStarBurst.h"
#include "PostEffectEnd.h"
#include "PostEffectSSAO.h"
#include "PostEffectDepthOfField.h"

#include "WindowManager.h"
#include "PostEffectZShadow.h"
#include "GBuffer.h"
#include "PostEffectFog.h"
#include "PostEffectGodRay.h"

namespace NSRender
{

class Render : public IDeviceResettable
{

public:

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

    int AddMeshMix(const std::wstring& filePath,
                   const D3DXVECTOR3& pos,
                   const D3DXVECTOR3& rot,
                   const float scale,
                   const float radius = -1.f,
                   const bool useParallaxOcclusionMapping = false,
                   const bool useNormalMapping = false,
                   const bool async = true);
    bool RemoveMeshMix(int id);
    int AddMeshMixSkinAnim(const std::wstring& filePath,
                           const D3DXVECTOR3& pos,
                           const D3DXVECTOR3& rot,
                           const float scale,
                           const AnimSetMap& animSetMap,
                           const float radius = -1.f,
                           const bool useParallaxOcclusionMapping = false,
                           const bool useNormalMapping = false);
    bool RemoveMeshMixSkinAnim(int id);

    void SetMeshMixPos(const int id, const D3DXVECTOR3& pos);
    void SetMeshMixSaturateShadow(const bool enabled);
    void SetMeshMixSaturateShadowIntensity(const float intensity);
    void SetMeshMixShadowDarkness(const float darkness);
    void SetMeshMixSpecularIntensity(const float intensity);
    void SetMeshMixSpecularEdge(const float edge);
    void SetMeshMixSpecularIntensityOverrideEnabled(const bool enabled);
    void SetMeshMixSpecularEdgeOverrideEnabled(const bool enabled);

    void SetCamera(const D3DXVECTOR3& pos, const D3DXVECTOR3& lookAt);
    void MoveCamera(const D3DXVECTOR3& pos);
    void RotateCamera(const D3DXVECTOR3& rot);

    D3DXVECTOR3 GetLookAtPos();
    D3DXVECTOR3 GetCameraPos();
    D3DXVECTOR3 GetCameraRotate();

    // フォント作成
    // IDが返ってくるので、そのIDを文字描画するときに指定する
    int SetUpFont(const std::wstring& fontName, const int fontSize, const UINT fontColor);

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

    // 彩度をどれくらい上げるか（下げるか）を設定
    void SetPostEffectSaturate(const float level);
    void SetPostEffectSaturateEnable(const bool arg);

    void SetPostEffectGaussianFilter(const bool arg);
    void SetPostEffectGaussianSampleSize(const int sampleSize);
    void SetPostEffectDepthBufferShadow(const bool arg);
    void SetPostEffectDepthBufferShadowIntensity(const float intensity);
    void SetPostEffectDepthBufferShadowSaturationBoost(const float saturationBoost);
    void SetPostEffectDepthBufferShadowCoverage(const float coverage);
    void SetPostEffectDepthBufferShadowPcfTapCount(const int tapCount);
    void SetPostEffectDepthBufferShadowCompositeTapCount(const int tapCount);
    void SetPostEffectSSAO(const bool arg);
    void SetPostEffectSSAOBrightness(const float brightness);
    void SetPostEffectSSAOSaturationBoost(const float saturationBoost);
    void SetPostEffectFog(const bool arg);
    void SetPostEffectFogIntensity(const float intensity);
    void SetPostEffectFogHeightEnable(const bool arg);
    void SetPostEffectFogHeightIntensity(const float intensity);
    void SetPostEffectFogHeightStart(const float start);

    void SetPostEffectBloom(const bool arg);
    void SetPostEffectBloomThreshold(const float threshold);
    void SetPostEffectDepthOfField(const bool arg);
    void SetPostEffectDepthOfFieldMode(const DepthOfFieldMode mode);
    void SetPostEffectDepthOfFieldFocalDistance(const float distance);
    void SetPostEffectDepthOfFieldMaxBlurDistance(const float distance);
    void SetPostEffectDepthOfFieldAutoActivationDistance(const float distance);

    void SetPostEffectStarBurst(const bool arg);
    void SetPostEffectStarBurstThreshold(const float threshold);

    void SetPostEffectGodRay(const bool arg);
    void SetPostEffectGodRayLightPos(const D3DXVECTOR3& pos);
    void SetPostEffectGodRayReverseSampling(const bool arg);
    void SetPostEffectGodRayRayLength(const float arg);
    void SetPostEffectGodRayIntensity(const float arg);
    void SetPostEffectGodRayVirtualProximityStrength(const float arg);
    void SetPostEffectGodRayOcclusionFalloff(const float arg);
    void SetPostEffectGodRayLightColor(const D3DXVECTOR3& color);

    void SetShowFPS(const bool arg);

    std::vector<std::pair<int, int>> GetResolutionList();

    // 平行光源がある方角
    void SetLightDir(const D3DXVECTOR3& dir);
    void SetLightBrightness(const float brightness);

    void AddPointLight(const D3DXVECTOR3& pos, const float brightness, const D3DXCOLOR color);

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

    std::unordered_map<std::wstring, MeshInstancing*> m_meshInstancingMap;

    // ポインターにしないとデバイスロストを扱う機能が機能しなくなる
    std::vector<Font*> m_fontList;
    Sprite m_sprite;

    //---------------------------------------------------------------
    // マルチパスレンダリング関連
    //---------------------------------------------------------------

    void DrawPass1(const bool renderToSceneRenderTargets = true);

    // 各ピクセルの深度とワールド座標を表した画像を生成
    GBuffer m_GBuffer;

    // 深度バッファシャドウ
    PostEffectZShadow m_postEffectZShadow;

    // SSAO用
    PostEffectSSAO m_postEffectSSAO;

    // 霧
    PostEffectFog m_postEffectFog;

    // 彩度フィルター
    PostEffectSaturate m_postEffectSaturate;

    // ブルームフィルター
    PostEffectBloom m_PostEffectBloom;

    // 被写界深度
    PostEffectDepthOfField m_postEffectDepthOfField;

    // ガウスフィルター
    PostEffectGauss m_postEffectGauss;

    // スターバースト
    PostEffectStarBurst m_postEffectStarBurst;

    // テクスチャ―の内容を画面に出力
    PostEffectEnd m_postEffectEnd;

    // ゴッドレイ
    PostEffectGodRay m_postEffectGodRay;

    void Draw2D();
    void LoadSettingsCsv(const std::wstring& settingsCsvPath);
    void ApplySettings();
    static std::wstring Trim(const std::wstring& text);
    static int NormalizeGaussianSampleSize(const int sampleSize);

    std::unordered_map<std::wstring, std::wstring> m_settings;
    int m_gaussianSampleSize = 101;
    bool m_postEffectSaturateEnabled = false;
    bool m_postEffectGaussEnabled = false;
    bool m_postEffectZShadowEnabled = true;
    bool m_postEffectSSAOEnabled = true;
    bool m_postEffectFogZEnabled = true;
    bool m_postEffectFogHeightEnabled = true;
    bool m_postEffectBloomEnabled = false;
    DepthOfFieldMode m_postEffectDepthOfFieldMode = DepthOfFieldMode::Disabled;
    bool m_postEffectStarBurstEnabled = false;
    bool m_postEffectGodRayEnabled = false;
    bool m_meshMixSaturateShadowEnabled = false;
    float m_meshMixSaturateShadowIntensity = 0.2f;
    float m_meshMixShadowDarkness = 1.0f;
    float m_meshMixSpecularIntensity = 0.0f;
    float m_meshMixSpecularEdge = 0.0f;
    bool m_meshMixSpecularIntensityOverrideEnabled = true;
    bool m_meshMixSpecularEdgeOverrideEnabled = true;

    LPDIRECT3DTEXTURE9 m_pRenderTarget1 = NULL;
    LPDIRECT3DTEXTURE9 m_pRenderTarget2 = NULL;

    //-----------------------------------------------------------------
    // FPS表示
    //-----------------------------------------------------------------

    bool m_bShowFPS = true;

    float CalcFPS();
    void ShowFPS(const float arg);

    int m_fontID = -1;
    std::vector<std::chrono::steady_clock::time_point> m_vecTime;

    //-----------------------------------------------------------------
    // デバイスロスト関連処理
    //-----------------------------------------------------------------

    void OnDeviceLost();

    void OnDeviceReset();

    void CreateTexture();

};
}

