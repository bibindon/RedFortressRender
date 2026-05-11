# ライブラリの使い方

この文書は `NSRender::Render` をゲームやツール側から利用するための説明書である。
サンプルアプリの実装と `Render.h` / `Render.cpp` を前提に、初期化、メッシュ登録、毎フレーム更新、終了処理、設定変更の流れを具体的にまとめる。

## 前提

このライブラリは次の前提で作られている。

- Windows
- DirectX 9
- `HWND` を持つ通常の Win32 アプリ
- 毎フレーム `Render::Draw()` を呼ぶゲームループ

`Render` は「描画機能の寄せ集め」ではなく、描画対象、カメラ、ライト、ポストエフェクト、2D 表示をまとめて管理する中心クラスである。
ゲーム側は基本的に `NSRender::Render` のインスタンスを 1 つ持って使う想定である。

## まず押さえるべき流れ

最小の流れは次の 5 段階である。

1. ウィンドウを作成する
2. `Render::Initialize()` を呼ぶ
3. カメラ、モデル、ライト、ポストエフェクトを設定する
4. 毎フレーム必要な更新を行った後に `Render::Draw()` を呼ぶ
5. 終了時に `Render::Finalize()` を呼ぶ

擬似コードにすると次のようになる。

```cpp
#include "Render.h"

NSRender::Render g_render;
int g_fontId = -1;

void InitializeRenderer(HWND hWnd)
{
    g_render.Initialize(hWnd, L"RenderSettings.csv");
    g_render.SetCamera(D3DXVECTOR3(0.0f, 1.7f, -2.0f),
                       D3DXVECTOR3(0.0f, 1.5f, 0.0f));

    g_fontId = g_render.SetUpFont(L"BIZ UDゴシック",
                                  20,
                                  D3DCOLOR_RGBA(255, 255, 255, 255));

    g_render.AddMeshMix(L"model.x",
                        D3DXVECTOR3(0.0f, 0.0f, 0.0f),
                        D3DXVECTOR3(0.0f, 0.0f, 0.0f),
                        1.0f);
}

void Tick()
{
    g_render.DrawText_(g_fontId, L"Hello", 20, 20);
    g_render.Draw();
}

void FinalizeRenderer()
{
    g_render.Finalize();
}
```

## 初期化

### `Initialize(HWND hWnd, const std::wstring& settingsCsvPath = L"")`

この関数は内部で次のものを初期化する。

- Direct3D デバイス生成
- ウィンドウ管理
- スプライト
- レンダーターゲットテクスチャ
- GBuffer
- 各種ポストエフェクト
- `RenderSettings.csv` の読み込みと適用

サンプルでは次のように呼んでいる。

```cpp
g_Render.Initialize(hWnd, L"RenderSettings.csv");
```

`settingsCsvPath` を空文字にすると、CSV を読まずに初期化する。
ただしこのライブラリはポストエフェクトやクリップ平面の既定値が多いため、外部から細かく調整したい場合は CSV を使う方が運用しやすい。

### 初期化直後に行うこと

`Initialize()` の直後には、少なくとも次を行うのがよい。

- カメラ位置と注視点の設定
- 必要なフォントの作成
- 最初に出したいモデルの登録
- 光源設定
- ポストエフェクト設定

例:

```cpp
g_render.SetCamera(D3DXVECTOR3(0.0f, 1.7f, -2.0f),
                   D3DXVECTOR3(0.0f, 1.5f, 0.0f));

g_render.SetLightDir(D3DXVECTOR3(0.2f, -1.0f, 0.1f));
g_render.SetLightColor(D3DXCOLOR(1.0f, 0.98f, 0.95f, 1.0f));
g_render.SetLightBrightness(1.0f);
g_render.SetAmbientLightColor(D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f));
g_render.SetAmbientLightBrightness(0.2f);
```

## モデルの追加

描画対象の追加 API は複数ある。
用途が近そうに見えても内部実装がかなり違うため、使い分けが重要である。

### 1. `AddMesh`

```cpp
int AddMesh(const std::wstring& filePath,
            const D3DXVECTOR3& pos,
            const D3DXVECTOR3& rot,
            const float scale,
            const float radius = -1.f,
            const float uvTile = 1.0f);
```

最も基本的なメッシュ追加 API である。
特殊な見た目が不要で、とにかく表示したいメッシュに向いている。

戻り値は ID であり、後で `RemoveMesh(id)` に渡せる。
ただし `RemoveMesh()` は実体削除ではなく「無効化」である。

### 2. `AddMeshNoLighting`

ライティングを受けないメッシュを追加する。
背景、UI 的な 3D オブジェクト、発光板、デバッグモデルなどに向く。

### 3. `AddMeshSmooth`

スムーズ法線を使うメッシュである。
角を立てたくないモデルに向いている。
戻り値はなく、個別 ID 管理もしない。

### 4. `AddMeshSSSLike` / `AddMeshSSS`

SSS 風表現を試すためのメッシュである。
`AddMeshSSS` の方は ID が返るので、`RemoveMeshSSS(id)` が使える。

### 5. `AddMeshPointLight`

ポイントライトの影響を受ける前提のメッシュである。
点光源演出を強く出したい物体向けである。

### 6. `AddMeshNormalMapping`

```cpp
int AddMeshNormalMapping(const std::wstring& filePath,
                         const std::wstring& normalMap,
                         const D3DXVECTOR3& pos,
                         const D3DXVECTOR3& rot,
                         const float scale,
                         const float radius = -1.f);
```

法線マップを明示的に指定するメッシュである。
別ファイルの法線マップを直接渡せる。

### 7. `AddMeshPOM`

視差遮蔽マッピング用のメッシュである。
平面寄りの素材に奥行き感を出したい場合に向く。

### 8. `AddMeshMix`

```cpp
int AddMeshMix(const std::wstring& filePath,
               const D3DXVECTOR3& pos,
               const D3DXVECTOR3& rot,
               const float scale,
               const float radius = -1.f,
               const bool useParallaxOcclusionMapping = false,
               const bool useNormalMapping = false,
               const bool async = true);
```

現状の主力 API はこれである。
`MeshMix` 系は次の機能を 1 つの系統にまとめて扱う。

- Normal Mapping
- POM
- SSAO 対応
- Depth Buffer Shadow 対応
- 環境マッピング
- SSS
- 波や揺れ
- ポイントライト反映
- `.x` の隣接 `.csv` による見た目上書き

特に理由がなければ、静的モデルはまず `AddMeshMix()` を使うのが無難である。

#### `async` 引数について

`async = true` の場合、`MeshMixManager` は内部で読み込みスレッドを立ててモデルを非同期ロードする。
大きいモデルを追加するときのフレーム停止を減らしたい場合に有効である。

一方で、追加直後の数フレームはまだロード未完了の可能性がある。
ロード完了前の `MeshMix` は描画されない。

#### `.x` 隣接 `.csv`

`MeshMix` 系では、`model.x` の横に `model.csv` を置くと追加設定を読む。
ここには `meshtype`, `smooth`, `sss`, `ssao`, `shadow`, `cubemappingrate` などを書ける。

詳しくは [README_about_x_csv.md](/C:/Users/bibindon/source/repos/bibindon/RedFortressRender/README_about_x_csv.md) を参照のこと。

### 9. `AddAnimMesh`

アニメーション付き `.x` 用である。
`AnimSetMap` を渡してアニメーション名と設定を登録する。

サンプルの最小例は次の通りである。

```cpp
NSRender::AnimSetMap animMap;
NSRender::AnimSetting idle;
idle.m_startPos = 0.f;
idle.m_duration = 1.f;
idle.m_loop = true;
idle.m_stopEnd = false;
animMap[L"0_Idle"] = idle;

g_render.AddAnimMesh(L"character.x",
                     D3DXVECTOR3(0.0f, 0.0f, 0.0f),
                     D3DXVECTOR3(0.0f, 0.0f, 0.0f),
                     1.0f,
                     animMap);
```

### 10. `AddSkinAnimMesh`

ボーンスキニングを含むアニメーションメッシュ用である。
内部では `SkinAnimMesh` が使われる。

### 11. `AddMeshMixSkinAnim`

`MeshMix` の見た目機能とスキンアニメーションを両立したい場合に使う。
アニメ付きキャラクターに SSAO、POM、スペキュラ調整などを乗せたいときの入口である。

### 12. `AddMeshInstansing`

```cpp
int AddMeshInstansing(const std::wstring& filePath,
                      const D3DXVECTOR3& pos,
                      const D3DXVECTOR3& rot,
                      const float scale);
```

同一メッシュを大量に置きたい場合の API である。
内部では `filePath` ごとに `MeshInstancing` を 1 つ持ち、`AddInstance(pos)` していく。

注意点:

- API 名は `Instansing` というスペルで固定されている
- 現状コードでは、インスタンス追加時に使われるのは `pos` だけである
- `rot` と `scale` は受け取るが、インスタンス単位では反映されない

## モデルの削除

削除 API は一見統一されているが、中身は完全一致ではない。

- `RemoveMesh`, `RemoveMeshSSS`, `RemoveMeshPointLight`, `RemoveMeshNormalMapping`, `RemoveMeshPOM`, `RemoveMeshMix`
  多くは「無効化」である
- `RemoveAnimMesh`, `RemoveSkinAnimMesh`
  実体を `SAFE_DELETE` する
- `RemoveMeshInstancing`
  `filePath` 単位で `MeshInstancing` を削除する

長時間動かしながら大量の生成・削除を繰り返す用途では、この違いを前提に設計した方がよい。

## カメラ制御

### 基本 API

```cpp
void SetCamera(const D3DXVECTOR3& pos, const D3DXVECTOR3& lookAt);
void MoveCamera(const D3DXVECTOR3& pos);
void RotateCamera(const D3DXVECTOR3& rot);
void SetCameraClipPlanes(float nearPlane, float farPlane);
void SetGBufferClipPlanes(float nearPlane, float farPlane);
```

### `SetCamera`

カメラ位置と注視点をまとめて設定する。

```cpp
g_render.SetCamera(D3DXVECTOR3(0.0f, 1.7f, -2.0f),
                   D3DXVECTOR3(0.0f, 1.5f, 0.0f));
```

### `MoveCamera`

現在の eye と lookAt の両方に同じ移動量を加える。
カメラ姿勢を保ったまま平行移動したい場合に向く。

### `RotateCamera`

注視点を中心に eye を球面回転させる。
内部では pitch を `±89°` 付近に制限しているため、真上・真下へはひっくり返らない。

### Near / Far

このライブラリではカメラのクリップ平面と GBuffer の深度レンジを別々に持てる。

- `SetCameraClipPlanes`
  通常描画カメラと Fog 系の距離解釈に影響する
- `SetGBufferClipPlanes`
  GBuffer、SSAO、GodRay、DOF などのスクリーンスペース系に影響する

両者を極端にずらすとポストエフェクト解釈が見た目とずれるため、まずは同じ値から始めるのが安全である。

## ライト制御

### 平行光源

```cpp
g_render.SetLightDir(D3DXVECTOR3(0.2f, -1.0f, 0.1f));
g_render.SetLightColor(D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f));
g_render.SetLightBrightness(1.0f);
```

`SetLightDir()` は内部で正規化される。
コメントにもある通り、「光が来る方向」と「法線方向」を逆に解釈しないよう注意が必要である。

### 環境光

```cpp
g_render.SetAmbientLightColor(D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f));
g_render.SetAmbientLightBrightness(0.2f);
```

暗部を持ち上げたいときに使う。

### ポイントライト

```cpp
g_render.AddPointLight(D3DXVECTOR3(0.0f, 2.0f, 0.0f),
                       2.0f,
                       D3DXCOLOR(1.0f, 0.7f, 0.4f, 1.0f));
```

さらに形状も指定できる。

```cpp
g_render.AddPointLight(pos,
                       1.5f,
                       color,
                       NSRender::PointLightShape::Line,
                       8.0f);
```

`PointLightShape` には次がある。

- `Point`
- `Line`
- `Square`
- `Cube`
- `Sphere`

## `MeshMix` 系の見た目調整

`MeshMix` や `MeshMixSkinAnim` は、追加前の既定値と追加後の一括上書きの両方がある。

### 主な API

```cpp
SetMeshMixSaturateShadow(bool enabled);
SetMeshMixSaturateShadowIntensity(float intensity);
SetMeshMixShadowDarkness(float darkness);
SetMeshMixSpecularIntensity(float intensity);
SetMeshMixSpecularEdge(float edge);
SetMeshMixSpecularIntensityOverrideEnabled(bool enabled);
SetMeshMixSpecularEdgeOverrideEnabled(bool enabled);
SetMeshMixSSS(bool enabled);
SetMeshMixSSSIntensity(float intensity);
SetMeshMixSSSColor(DWORD color);
```

これらは内部で次の 2 つに効く。

- すでに追加済みの `MeshMix` / `MeshMixSkinAnim`
- これ以降に追加される `MeshMix` / `MeshMixSkinAnim` の既定値

つまり、「新規追加前に既定値を決める用途」と「追加済みの全体調整」の両方に使える。

例:

```cpp
g_render.SetMeshMixSpecularIntensity(0.2f);
g_render.SetMeshMixSpecularEdge(0.6f);
g_render.SetMeshMixSSS(true);
g_render.SetMeshMixSSSIntensity(4.0f);
g_render.SetMeshMixSSSColor(0xFFFFC080);
```

## 2D 表示

### フォント

```cpp
int fontId = g_render.SetUpFont(L"BIZ UDゴシック",
                                20,
                                D3DCOLOR_RGBA(255, 255, 255, 255));
```

`SetUpFont()` はフォント ID を返す。
文字表示は毎フレーム積み上げる方式なので、表示したい文字は毎フレーム `DrawText_()` する必要がある。

```cpp
g_render.DrawText_(fontId, L"HP 100", 20, 20);
g_render.DrawText_(fontId, L"Warning", 20, 50, D3DCOLOR_RGBA(255, 80, 80, 255));
g_render.DrawTextCenter(fontId, L"PAUSE", 0, 0, 1600, 900);
```

### 画像

```cpp
g_render.DrawImage(L"ui\\icon.png", 100, 100, 255);
```

`transparency` はアルファ値として使う。

### 2D 描画タイミング

2D は `Render::Draw()` の最後に描かれる。
そのため通常はブルーム、ガウス、彩度、DOF などの影響を受けない。
HUD や文字をくっきり表示したい場合に都合がよい。

## ポストエフェクト

### 基本方針

ポストエフェクトは、ほぼすべて `SetPostEffect...()` 系で ON/OFF とパラメーター変更ができる。
主なものは次の通りである。

- Saturate
- Gaussian
- Masked Gaussian
- FXAA
- Motion Blur Camera
- Depth Buffer Shadow
- SSAO / SSAO2
- Fog / Height Fog
- Bloom
- Depth Of Field
- StarBurst
- GodRay

### 代表例

```cpp
g_render.SetPostEffectBloom(true);
g_render.SetPostEffectBloomThreshold(2.5f);

g_render.SetPostEffectSSAO(true);
g_render.SetPostEffectSSAOMode(NSRender::SSAOMode::SSAO2);
g_render.SetPostEffectSSAO2SampleCount(16);

g_render.SetPostEffectDepthOfFieldMode(NSRender::DepthOfFieldMode::AutoNear);
g_render.SetPostEffectDepthOfFieldFocalDistance(8.0f);
g_render.SetPostEffectDepthOfFieldAutoActivationDistance(10.0f);
```

### Fog の注意点

`SetPostEffectFog(bool)` は距離 Fog と Height Fog の両方を同時に切り替える。
Height Fog だけ別に触りたい場合は `SetPostEffectHeightFog()` や `SetPostEffectFogHeightEnable()` を使う。

### SSAO モード

`SetPostEffectSSAOMode()` で `Legacy` と `SSAO2` を切り替えられる。
現在の実装では、`Draw()` 中でモードに応じて `PostEffectSSAO` か `PostEffectSSAO2` を分岐して使う。

### DOF モード

`DepthOfFieldMode` は次の 3 つである。

- `Disabled`
- `Enabled`
- `AutoNear`

`AutoNear` は画面中央付近の位置テクスチャを見て自動でブレンド量を変える。

## デバッグ表示

GBuffer の中身を直接見たい場合は `SetDebugGBufferView()` を使う。

```cpp
g_render.SetDebugGBufferView(NSRender::DebugGBufferView::WorldPos);
```

選べるのは次である。

- `None`
- `WorldPos`
- `Normal`
- `Depth`
- `Thickness`

## 解像度とウィンドウモード

### API

```cpp
g_render.ChangeResolution(1920, 1080);
g_render.ChangeWindowMode(NSRender::eWindowMode::BORDERLESS);
```

`ChangeResolution()` は即座にデバイスリセットへ進む。
`ChangeWindowMode()` は `WindowManager` にリクエストを出し、実際の切り替えは `Draw()` の最後で処理される。

ウィンドウモードは次の 3 種類である。

- `WINDOW`
- `BORDERLESS`
- `FULLSCREEN`

## RenderSettings.csv の使いどころ

`Render::Initialize()` に CSV パスを渡すと、`Render::LoadSettingsCsv()` と `ApplySettings()` が動く。
ここで設定できる内容はかなり多い。

- クリップ平面
- SSAO
- Fog
- Gaussian
- FXAA
- Motion Blur Camera
- SSS
- Bloom
- DOF
- StarBurst
- GodRay

詳しいキー一覧は [README_about_library_csv.md](/C:/Users/bibindon/source/repos/bibindon/RedFortressRender/README_about_library_csv.md) を参照のこと。

ゲーム本体側で「起動時プリセット」を持ちたい場合、この CSV を使うとコード側の初期化がかなり減る。

## 毎フレーム更新でやること

毎フレームのゲームループでは、概ね次を行う。

1. 入力に応じてカメラやライトを更新する
2. 表示したい文字や画像を登録する
3. 必要ならポストエフェクト値を更新する
4. `Render::Draw()` を呼ぶ

サンプルの実装に近い形では次のようになる。

```cpp
void Tick()
{
    UpdateCameraByInput();
    UpdateDirectionalLight();

    g_render.DrawText_(fontId, L"Debug", 20, 20);
    g_render.Draw();
}
```

### 毎フレーム再登録が必要なもの

- `DrawText_()`
- `DrawTextCenter()`
- `DrawImage()`

これらは「今フレーム描くものを積む」使い方である。
一度呼べば残り続ける API ではない点に注意すること。

## 終了処理

終了時は `Finalize()` を呼ぶ。

```cpp
g_render.Finalize();
```

現状の `Finalize()` は `Common::D3DDevice()->Release()` と `Common::SetD3DDevice(NULL)` が中心であり、すべての所有リソースを丁寧に掃除する作りではない。
通常のプロセス終了時には問題になりにくいが、ライブラリの再初期化を繰り返す用途では注意した方がよい。

## パスに関する注意

このライブラリでは、相対パスが「exe の場所」ではなく「実行時カレントディレクトリ」依存になる箇所がある。
`MeshMixManager` は内部で相対パスを exe ディレクトリ基準に補っているが、すべての API が同じ流儀ではない。

安全策は次のいずれかである。

- 起動時にカレントディレクトリを exe ディレクトリへ合わせる
- 重要なモデルやテクスチャは絶対パスで渡す
- すべてのリソースパスを自前で正規化してから渡す

## 実運用上の注意点

### 1. `MeshMix` は非同期ロードが既定

追加直後に必ず見えるとは限らない。
ロード完了前は `IsLoaded()` が false で描画されない。

### 2. 削除 API は完全統一ではない

無効化と delete が混在している。
長時間運用ではメモリ増加に注意すること。

### 3. `AddMeshInstansing` は未完成寄り

個別回転・個別スケールを持たせたい用途には、そのままでは足りない。

### 4. 2D はポストエフェクト後

UI にブルームを乗せたい、UI に DOF をかけたい、という設計には向かない。

### 5. 64bit アニメーションの注意

`AnimController.h` にある通り、DirectX9 のアニメーションは 64bit ビルド時に速度が大きくずれる問題を前提に扱っている。
アニメーション付きメッシュを導入するときは、見た目の速度を必ず実機確認した方がよい。

## 実践向けのおすすめ開始パターン

最初の導入では次の順を勧める。

1. `Initialize()` と `SetCamera()` だけを入れる
2. `AddMeshMix()` で 1 モデル表示する
3. `SetLightDir()` / `SetLightBrightness()` を足す
4. `SetUpFont()` と `DrawText_()` を足す
5. `RenderSettings.csv` にポストエフェクト設定を逃がす

最初からすべての API を触るより、`MeshMix + Camera + Light + Draw` の一本線をまず通す方がデバッグしやすい。
