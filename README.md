﻿# RedfortressRender

自作のインディーゲームで使うことを前提にした、DirectX 9 ベースの 3D 描画ライブラリです。  
ゲーム本体で必要になった描画機能をまとめながら、サンプルプロジェクトで見た目と挙動を確認できる構成になっています。

## 概要

このリポジトリは、描画ライブラリ本体と確認用アプリをひとまとめにしたものです。

- `Render`
  描画ライブラリ本体です。メッシュ描画、アニメーション、ライティング、ポストエフェクト、2D 描画まわりを含みます。

- `Sample`
  ライブラリの動作確認用アプリです。各種モデルを配置し、設定ダイアログや `RenderSettings.csv` から描画パラメータを切り替えられます。

- `GrassLandSample`
  別シーン用のサンプルプロジェクトです。

- `UnitTest1`
  単体テスト用プロジェクトです。

## 実装してあるもの

- 通常メッシュ描画
- スムーズ法線メッシュ描画
- 法線マッピング
- 視差遮蔽マッピング
- サブサーフェイススキャッタリング
- ポイントライト対応メッシュ描画
- 環境マッピング対応メッシュ描画
- アニメーションメッシュ描画
- スキンメッシュ描画
- メッシュインスタンシング
- スプライト描画
- フォント描画
- GBuffer 生成
- 深度バッファシャドウ
- SSAO
- フォグ
- ガウシアンブラー
- ブルーム
- スターバースト
- 彩度フィルタ

## まだ

- [ ] SSGI
- [ ] GOD RAY
- [ ] Motion Blur(Object)
- [ ] Motion Blur(Camera)
- [ ] Tone Mapping
- [ ] Gaussian Filter (Crop)
- [ ] 草、波、水面
- [ ] パーティクル（煙）
- [ ] パーティクル（炎）
- [ ] パーティクル（霧）

## 仕組み

中心になるクラスは `Render` です。  
`Render` がカメラ、ライト、各種メッシュ、ポストエフェクト、2D 描画をまとめて持ち、毎フレームの描画順を制御します。

メッシュは用途ごとにクラスが分かれています。

- `MeshOld`
  基本的なメッシュ描画です。

- `MeshMix`
  法線マッピング、視差遮蔽マッピング、環境マッピング、ポイントライト、SSAO、深度シャドウなどをまとめて扱うメッシュです。

- `AnimMesh`
  アニメーション付きメッシュ描画です。

- `SkinAnimMesh`
  ボーンを使うスキンアニメーション描画です。

- `MeshInstancing`
  同一メッシュの大量配置用です。

ポストエフェクトは個別クラスとして分かれており、必要なものを `Render` から有効化します。

- `PostEffectZShadow`
- `PostEffectSSAO`
- `PostEffectFog`
- `PostEffectGauss`
- `PostEffectBloom`
- `PostEffectStarBurst`
- `PostEffectSaturate`
- `PostEffectEnd`

## 描画の流れ

描画はおおむね次の順で進みます。

1. 3D メッシュを描画する
2. GBuffer を生成する
3. 深度シャドウや SSAO など、GBuffer を使う処理を行う
4. ブラー、ブルーム、スターバースト、フォグなどのポストエフェクトを重ねる
5. スプライトやフォントを描画する

複数のメッシュ表現を個別クラスに分けつつ、最終的なフレーム生成は `Render` に集約する構成です。

## 開発環境

- Windows
- DirectX 9
- Visual Studio 2026

ソリューションファイルは [Render.sln](/C:/Users/bibindon/source/repos/bibindon/RedfortressRender/Render.sln) です。


---

# 自分用詳細説明書

この章は、他人向けの紹介ではなく、作者本人があとから見返して「どのクラスを使えばよいか」「どのパラメーターが何を意味するか」「サンプル画面で触っている値が Render 側の何に対応するか」を確認するためのメモです。

## 全体方針

`RedfortressRender` は、ゲーム本体から `NSRender::Render` をひとつ持ち、そこにモデル、カメラ、ライト、ポストエフェクト、2D 表示を登録していく構成です。ゲーム側から見ると、毎フレーム行うべきことは大きく分けて次の通りです。

1. 起動時に `Render::Initialize()` を呼ぶ。
2. 必要なモデルを `AddMesh...()` 系の関数で登録する。
3. 毎フレーム、カメラやモデル位置、エフェクト値を必要に応じて更新する。
4. 毎フレーム `Render::Draw()` を呼ぶ。
5. 終了時に `Render::Finalize()` を呼ぶ。

サンプルでは `InitializeSampleScene()` 内で `LoadSampleSettingsFromCsv(L"RenderSettings.csv")` と `g_Render.Initialize(hWnd, L"RenderSettings.csv")` を実行し、その後、初期カメラ、フォント、初期メッシュ、ポストエフェクト設定を適用しています。

## 最小構成の使い方

```cpp
NSRender::Render render;

render.Initialize(hWnd, L"RenderSettings.csv");
render.SetCamera(D3DXVECTOR3(0.0f, 1.7f, -2.0f),
                 D3DXVECTOR3(0.0f, 1.5f, 0.0f));

const int meshId = render.AddMeshMix(L"model.x",
                                     D3DXVECTOR3(0.0f, 0.0f, 0.0f),
                                     D3DXVECTOR3(0.0f, 0.0f, 0.0f),
                                     1.0f,
                                     100.0f,
                                     false,
                                     false);

while (running)
{
    render.Draw();
}

render.Finalize();
```

`RenderSettings.csv` を使わない場合は、`Initialize(hWnd)` だけでもよいです。ただし、その場合はコードから各種エフェクトの有効/無効や強さを設定することになります。

## Render クラスの公開 API

### 初期化・終了・描画

| 関数 | 用途 |
|---|---|
| `Initialize(HWND hWnd, const std::wstring& settingsCsvPath = L"")` | DirectX9 デバイス、描画リソース、ポストエフェクトを初期化する。CSV パスを渡すと初期値も読む。 |
| `Finalize()` | DirectX デバイスを解放する。 |
| `Draw()` | 1フレーム分の描画を行う。3D、GBuffer、ポストエフェクト、2D の順にまとめて実行する。 |
| `ChangeResolution(int W, int H)` | 解像度を変更する。 |
| `ChangeWindowMode(eWindowMode eWindowMode_)` | ウィンドウ、ボーダーレス、フルスクリーンなどの表示モードを変更する。 |
| `GetResolutionList()` | 使用可能な解像度一覧を取得する。 |
| `SetShowFPS(bool arg)` | FPS 表示の有効/無効を切り替える。 |

`Draw()` の中では、おおむね次の順で処理されます。

1. FPS 表示用の文字を登録する。
2. `DrawPass1()` で通常の 3D 描画をレンダーターゲットに出す。
3. `GBuffer::Draw()` で深度、ワールド座標、法線テクスチャを作る。
4. 深度バッファシャドウを適用する。
5. SSAO を適用する。
6. フォグを適用する。
7. 彩度フィルターを適用する。
8. 被写界深度を適用する。
9. ブルームを適用する。
10. スターバーストを適用する。
11. ガウシアンフィルターを適用する。
12. `PostEffectEnd` で最終テクスチャを画面に転送する。
13. 文字と画像を描画する。

重要なのは、文字やスプライトはポストエフェクトの後に描かれる点です。つまり、UI 文字や画像はブルーム、彩度、フォグなどの影響を受けにくい構造です。

## メッシュ追加 API

### 共通パラメーター

多くのメッシュ追加関数は、次のような引数を持ちます。

| 引数 | 意味 |
|---|---|
| `filePath` | `.x` ファイルなど、読み込むモデルファイルのパス。相対パスは実行時カレントディレクトリの影響を受けるので注意。 |
| `pos` | 配置位置。`D3DXVECTOR3(x, y, z)`。 |
| `rot` | 回転。基本的には X/Y/Z 軸回転を入れる想定。単位は実装側でラジアンとして扱う前提で考える。 |
| `scale` | 読み込み時の拡大率。サンプル側では 0.1 から 10.0 の範囲で調整する。 |
| `radius` | 描画・当たり判定・影響範囲などに使う想定の半径。`-1.0f` の場合は自動または未指定扱い。 |
| `uvTile` | UV の繰り返し倍率。通常メッシュ系で使う。標準は `1.0f`。 |

### AddMesh

```cpp
int AddMesh(const std::wstring& filePath,
            const D3DXVECTOR3& pos,
            const D3DXVECTOR3& rot,
            const float scale,
            const float radius = -1.f,
            const float uvTile = 1.0f);
```

基本的なライトありメッシュを追加します。戻り値は ID です。削除するときは `RemoveMesh(id)` を呼びます。

### AddMeshNoLighting

```cpp
int AddMeshNoLighting(const std::wstring& filePath,
                      const D3DXVECTOR3& pos,
                      const D3DXVECTOR3& rot,
                      const float scale,
                      const float radius = -1.f,
                      const float uvTile = 1.0f);
```

ライティングを受けない、またはライト計算を簡略化したいメッシュ用です。空、背景、発光板、デバッグ用モデルなどに向いています。

### AddMeshSmooth

```cpp
void AddMeshSmooth(const std::wstring& filePath,
                   const D3DXVECTOR3& pos,
                   const D3DXVECTOR3& rot,
                   const float scale,
                   const float radius = -1.f);
```

スムーズ法線メッシュを追加します。法線を滑らかにして、角張りを減らしたいモデルに使います。戻り値 ID がないので、現在の API では個別削除対象としては弱いです。

### AddMeshSSSLike / AddMeshSSS

```cpp
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
```

サブサーフェイススキャッタリング風、または SSS 用メッシュを追加します。肌、ロウ、半透明感のある素材を試すための系統です。`AddMeshSSS()` は ID を返すので `RemoveMeshSSS(id)` が使えます。

### AddMeshPointLight

```cpp
int AddMeshPointLight(const std::wstring& filePath,
                      const D3DXVECTOR3& pos,
                      const D3DXVECTOR3& rot,
                      const float scale,
                      const float radius = -1.f);
```

ポイントライト対応メッシュを追加します。点光源の影響を明示的に受けるモデル用です。点光源自体は `AddPointLight()` で追加します。

### AddMeshNormalMapping

```cpp
int AddMeshNormalMapping(const std::wstring& filePath,
                         const std::wstring& normalMap,
                         const D3DXVECTOR3& pos,
                         const D3DXVECTOR3& rot,
                         const float scale,
                         const float radius = -1.f);
```

法線マップを指定してメッシュを追加します。`normalMap` に法線マップ画像のパスを渡します。細かい凹凸をジオメトリを増やさずに出したい場合に使います。

### AddMeshPOM

```cpp
int AddMeshPOM(const std::wstring& filePath,
               const D3DXVECTOR3& pos,
               const D3DXVECTOR3& rot,
               const float scale,
               const float radius);
```

Parallax Occlusion Mapping 用メッシュです。法線マッピングよりも奥行き感を強く出すためのものです。床、壁、石畳など、近くで見る平面素材に使うと効果が分かりやすいです。

### AddMeshMix

```cpp
int AddMeshMix(const std::wstring& filePath,
               const D3DXVECTOR3& pos,
               const D3DXVECTOR3& rot,
               const float scale,
               const float radius = -1.f,
               const bool useParallaxOcclusionMapping = false,
               const bool useNormalMapping = false);
```

現在の主力メッシュ追加 API と考えてよいです。内部では `GetMeshParamPreset(eMeshParamPreset::GRASS)` をベースにした `MeshMixManager` を作成し、次のパラメーターを反映してから初期化します。

- `smooth = false`
- `parallaxOcclusionMapping = useParallaxOcclusionMapping`
- `normalMapping = useNormalMapping`
- `saturateShadow = m_meshMixSaturateShadowEnabled`
- `saturateShadowIntensity = m_meshMixSaturateShadowIntensity`
- `shadowDarkness = m_meshMixShadowDarkness`
- `specularIntensity = m_meshMixSpecularIntensity`
- `specularEdge = m_meshMixSpecularEdge`

つまり、`AddMeshMix()` を呼ぶ前に `SetMeshMixSaturateShadow()` や `SetMeshMixSpecularIntensity()` を呼んでおくと、その時点以降に追加される MeshMix に反映されます。すでに追加済みのメッシュに後から一括反映されるかどうかは、実装を確認してから使うべきです。

### AddMeshMixSkinAnim

```cpp
int AddMeshMixSkinAnim(const std::wstring& filePath,
                       const D3DXVECTOR3& pos,
                       const D3DXVECTOR3& rot,
                       const float scale,
                       const AnimSetMap& animSetMap,
                       const float radius = -1.f,
                       const bool useParallaxOcclusionMapping = false,
                       const bool useNormalMapping = false);
```

`MeshMix` 系の描画表現を持つスキンアニメーションメッシュを追加します。アニメ付きキャラクターにも POM / NormalMapping 系の表現を使いたい場合の入口です。

### AddAnimMesh / AddSkinAnimMesh

```cpp
int AddAnimMesh(const std::wstring& filePath,
                const D3DXVECTOR3& pos,
                const D3DXVECTOR3& rot,
                const float scale,
                const AnimSetMap& animSetMap);

int AddSkinAnimMesh(const std::wstring& filePath,
                    const D3DXVECTOR3& pos,
                    const D3DXVECTOR3& rot,
                    const float scale,
                    const AnimSetMap& animSetMap);
```

アニメーションメッシュ用です。`AnimSetMap` にアニメーション名とインデックスなどの対応を渡します。サンプル側には `CreateDefaultAnimSetMap()` があります。

### AddMeshInstansing

```cpp
int AddMeshInstansing(const std::wstring& filePath,
                      const D3DXVECTOR3& pos,
                      const D3DXVECTOR3& rot,
                      const float scale);
```

同じメッシュを大量に置くための API です。内部では `filePath` をキーに `MeshInstancing` を管理します。まだスペルが `Instansing` になっているので、将来 API を整理するなら `AddMeshInstancing` に直したいです。

現在の実装では、同じ `filePath` が未登録なら `MeshInstancing` を作成し、その後 `AddInstance(pos)` を呼ぶ形です。`rot` と `scale` を受け取っていますが、少なくとも見えている範囲ではインスタンス追加時に `pos` だけを渡しています。回転とスケールをインスタンスごとに効かせたい場合は実装修正が必要です。

## メッシュ削除 API

| 関数 | 挙動 |
|---|---|
| `RemoveMesh(int id)` | 通常メッシュを無効化または削除する。 |
| `RemoveMeshSSS(int id)` | SSS メッシュを無効化する。 |
| `RemoveMeshPointLight(int id)` | ポイントライト対応メッシュを無効化する。 |
| `RemoveMeshNormalMapping(int id)` | 法線マッピングメッシュを無効化する。 |
| `RemoveMeshPOM(int id)` | POM メッシュを無効化する。 |
| `RemoveAnimMesh(int id)` | `AnimMesh*` を `SAFE_DELETE` する。 |
| `RemoveSkinAnimMesh(int id)` | `SkinAnimMesh*` を `SAFE_DELETE` する。 |
| `RemoveMeshMix(int id)` | `MeshMixManager` を `SetEnabled(false)` にする。 |
| `RemoveMeshMixSkinAnim(int id)` | MeshMix + SkinAnim 系を削除または無効化する。 |
| `RemoveMeshInstancing(const std::wstring& filePath)` | `filePath` 単位でインスタンシングメッシュを削除する。 |

削除 API には、完全にメモリ解放するものと、表示無効化に留めるものが混在しています。ID の再利用、削除後の vector/deque の穴、長時間実行時のメモリ使用量には注意が必要です。

## カメラ API

```cpp
void SetCamera(const D3DXVECTOR3& pos, const D3DXVECTOR3& lookAt);
void MoveCamera(const D3DXVECTOR3& pos);
void RotateCamera(const D3DXVECTOR3& rot);
D3DXVECTOR3 GetLookAtPos();
D3DXVECTOR3 GetCameraPos();
D3DXVECTOR3 GetCameraRotate();
```

`SetCamera()` はカメラ位置と注視点をまとめて指定します。サンプルでは次の初期値です。

```cpp
g_Render.SetCamera(D3DXVECTOR3(0.0f, 1.7f, -2.0f),
                   D3DXVECTOR3(0.0f, 1.5f, 0.0f));
```

サンプルではマウス移動に応じて `RotateCamera()` を呼び、キーボード入力で移動します。

カメラ操作のサンプル定数は次の通りです。

| 定数 | 値 | 意味 |
|---|---:|---|
| `MOUSE_CAMERA_SENSITIVITY` | `0.005f` | マウス移動量を回転量に変換する係数。 |
| `MODEL_SPAWN_FORWARD_OFFSET` | `6.0f` | 注視方向の前方にモデルを出す距離。 |
| `MOUSE_WHEEL_CAMERA_DISTANCE` | `1.0f` | ホイール操作で注視点から離れる/近づく距離。 |

## ライト API

```cpp
void SetLightDir(const D3DXVECTOR3& dir);
void SetLightBrightness(const float brightness);
void AddPointLight(const D3DXVECTOR3& pos, const float brightness, const D3DXCOLOR color);
```

### 平行光源

`SetLightDir()` は平行光源の向きを設定します。`SetLightBrightness()` は太陽光の強さのように扱います。サンプル側の `Sun Light` スライダーは `SetLightBrightness()` に対応します。

サンプル UI 上の範囲は次の通りです。

| パラメーター | 最小 | 最大 | ステップ |
|---|---:|---:|---:|
| `Sun Light` | `0.0` | `5.0` | `0.1` |

### ポイントライト

`AddPointLight()` は、指定位置に点光源を追加します。

| 引数 | 意味 |
|---|---|
| `pos` | 点光源の位置。 |
| `brightness` | 明るさ。サンプル範囲は `0.0` から `5.0`。 |
| `color` | `D3DXCOLOR`。RGB はサンプル範囲 `0.0` から `1.0`。 |

サンプル UI 上の範囲は次の通りです。

| パラメーター | 最小 | 最大 | ステップ |
|---|---:|---:|---:|
| `PointLight R` | `0.0` | `1.0` | `0.05` |
| `PointLight G` | `0.0` | `1.0` | `0.05` |
| `PointLight B` | `0.0` | `1.0` | `0.05` |
| `PointLight Power` | `0.0` | `5.0` | `0.1` |

## 2D 表示 API

### フォント

```cpp
int SetUpFont(const std::wstring& fontName, const int fontSize, const UINT fontColor);
void DrawText_(const int fontId, const std::wstring& text, const int X, const int Y);
void DrawText_(const int fontId, const std::wstring& text, const int X, const int Y, const UINT color);
void DrawTextCenter(const int fontId, const std::wstring& text, const int X, const int Y, const int Width, const int Height);
void DrawTextCenter(const int fontId, const std::wstring& text, const int X, const int Y, const int Width, const int Height, const UINT color);
```

`SetUpFont()` はフォントを作成し、ID を返します。サンプルでは次のように使っています。

```cpp
g_fontId = g_Render.SetUpFont(L"BIZ UDゴシック", 20, D3DCOLOR_RGBA(255, 255, 255, 255));
```

`DrawText_()` と `DrawTextCenter()` は、文字を表示し続けたい場合、毎フレーム呼ぶ必要があります。内部ではそのフレームで描く文字を登録して、`Draw2D()` でまとめて描く設計だと考えられます。

### 画像

```cpp
void DrawImage(const std::wstring& text,
               const int X,
               const int Y,
               const int transparency = 255);
```

`text` には画像名または画像パスを渡します。`transparency` は透明度で、標準値は `255` です。毎フレーム描きたい画像は毎フレーム呼ぶ必要があります。

## ポストエフェクト API

### 彩度フィルター

```cpp
void SetPostEffectSaturate(const float level);
void SetPostEffectSaturateEnable(const bool arg);
```

画面全体の彩度を調整します。`level` は彩度の強さです。サンプル UI の範囲は次の通りです。

| パラメーター | 最小 | 最大 | ステップ |
|---|---:|---:|---:|
| `Saturation` | `0.0` | `4.0` | `0.1` |

`0.0` はグレースケール寄り、`1.0` は標準、`1.0` より大きいと彩度強調、と考えると調整しやすいです。

### ガウシアンフィルター

```cpp
void SetPostEffectGaussianFilter(const bool arg);
void SetPostEffectGaussianSampleSize(const int sampleSize);
```

画面全体にガウシアンブラーをかけます。`sampleSize` はサンプル数です。`NormalizeGaussianSampleSize()` により、次のように補正されます。

- 最小 `1`
- 最大 `101`
- 偶数を渡した場合は 1 減らして奇数にする

つまり、`2` を渡すと `1`、`10` を渡すと `9`、`102` を渡すと `101` になります。

サンプル UI の範囲は次の通りです。

| パラメーター | 最小 | 最大 |
|---|---:|---:|
| `Gaussian Size` | `1` | `101` |

ゲーム用途では、常時 101 のような大きい値を使うと非常に重くなりやすいです。演出として短時間使う、または距離や明るさに応じて限定的に使うのが安全です。

### 深度バッファシャドウ

```cpp
void SetPostEffectDepthBufferShadow(const bool arg);
void SetPostEffectDepthBufferShadowIntensity(const float intensity);
void SetPostEffectDepthBufferShadowSaturationBoost(const float saturationBoost);
```

GBuffer の深度・法線などを使って、スクリーンスペース的に影を重ねる処理です。

| パラメーター | 意味 | サンプル範囲 |
|---|---|---|
| `DepthBufferShadowEnable` | 有効/無効 | `true/false`, `on/off`, `1/0`, `yes/no` |
| `ShadowIntensity` | 影の強さ | `0.0` から `1.0`、ステップ `0.05` |
| `ShadowSaturationBoost` | 影部分の彩度ブースト | `0.0` から `1.0`、ステップ `0.05` |

`ShadowIntensity` は単純に暗くする量です。`ShadowSaturationBoost` は、黒に落とすだけでなく色味を強くする方向の調整です。彩度を上げる影表現を試すときに重要です。

### SSAO

```cpp
void SetPostEffectSSAO(const bool arg);
void SetPostEffectSSAOBrightness(const float brightness);
void SetPostEffectSSAOSaturationBoost(const float saturationBoost);
```

GBuffer の深度、ワールド座標、法線を使って SSAO をかけます。

| パラメーター | 意味 | サンプル範囲 |
|---|---|---|
| `SSAOEnable` | 有効/無効 | `true/false`, `on/off`, `1/0`, `yes/no` |
| `SSAOBrightness` | SSAO の明るさ補正 | `0.25` から `4.0`、ステップ `0.05` |
| `SSAOSaturationBoost` | SSAO 部分の彩度ブースト | `0.0` から `1.0`、ステップ `0.05` |

`SSAOBrightness` は、SSAO の黒さ・明るさの調整です。値を低くすると暗く締まりやすく、高くすると影が弱く見える可能性があります。`SSAOSaturationBoost` は深度シャドウと同じく、暗部の色味を強くするための値です。

### フォグ

```cpp
void SetPostEffectFog(const bool arg);
void SetPostEffectFogIntensity(const float intensity);
```

`SetPostEffectFog()` は内部で `SetEnableZ(arg)` と `SetEnableHeight(arg)` の両方を切り替えます。つまり、現状では Z 距離フォグと高さフォグをまとめて ON/OFF する設計です。

| パラメーター | 意味 | サンプル範囲 |
|---|---|---|
| `FogEnable` | 有効/無効 | `true/false`, `on/off`, `1/0`, `yes/no` |
| `FogIntensity` | フォグ密度 | `0.0` から `20.0`、ステップ `0.1` |

`FogIntensity` は大きすぎると画面がすぐ白っぽく、または霧色で埋まりやすいです。広い屋外では小さめから調整するのが無難です。

### ブルーム

```cpp
void SetPostEffectBloom(const bool arg);
void SetPostEffectBloomThreshold(const float threshold);
```

明るい部分を抽出してにじませる処理です。

| パラメーター | 意味 | サンプル範囲 |
|---|---|---|
| `BloomEnable` | 有効/無効 | `true/false`, `on/off`, `1/0`, `yes/no` |
| `BloomThreshold` | 抽出しきい値 | `0.0` から `5.0`、ステップ `0.1` |

`BloomThreshold` が低いほど、暗めの部分までブルーム対象になり、画面全体がぼやけたり白飛びしやすくなります。高いほど、本当に明るい部分だけが光ります。

### 被写界深度

```cpp
void SetPostEffectDepthOfField(const bool arg);
void SetPostEffectDepthOfFieldFocalDistance(const float distance);
```

焦点距離から外れた部分をぼかす処理です。

| パラメーター | 意味 | サンプル範囲 |
|---|---|---|
| `DepthOfFieldEnable` | 有効/無効 | `true/false`, `on/off`, `1/0`, `yes/no` |
| `DepthOfFieldFocalDistance` | 焦点距離 | `0.5` から `50.0`、ステップ `0.1` |

`DepthOfFieldFocalDistance` はワールド空間距離に近い感覚で調整します。プレイヤー視点のゲームでは、常時強くかけると見づらくなりやすいので、イベントシーンやスクリーンショット用に向いています。

### スターバースト

```cpp
void SetPostEffectStarBurst(const bool arg);
void SetPostEffectStarBurstThreshold(const float threshold);
```

明るい部分から放射状または星状の光を出す処理です。

| パラメーター | 意味 | サンプル範囲 |
|---|---|---|
| `StarBurstEnable` | 有効/無効 | `true/false`, `on/off`, `1/0`, `yes/no` |
| `StarBurstThreshold` | 抽出しきい値 | `0.0` から `5.0`、ステップ `0.1` |

ブルームと似て、しきい値が低いほど多くの部分が対象になります。ブルームと同時に使うと派手になりますが、白飛びや画面の眠さに注意します。

## MeshMix 用の追加調整

```cpp
void SetMeshMixPos(const int id, const D3DXVECTOR3& pos);
void SetMeshMixSaturateShadow(const bool enabled);
void SetMeshMixSaturateShadowIntensity(const float intensity);
void SetMeshMixShadowDarkness(const float darkness);
void SetMeshMixSpecularIntensity(const float intensity);
void SetMeshMixSpecularEdge(const float edge);
```

### SetMeshMixPos

指定 ID の MeshMix の位置を変更します。動く床、動くオブジェクト、簡易的な配置調整に使えます。

### Half Lambert / MeshMix 側の影彩度

サンプルでは `Half Lambert Shadow Saturation` という名前で、`SetMeshMixSaturateShadow()` と `SetMeshMixSaturateShadowIntensity()` に対応しています。

| パラメーター | 最小 | 最大 | ステップ |
|---|---:|---:|---:|
| `Lambert Sat` | `0.0` | `2.0` | `0.05` |

値が `0.0` より大きい場合、サンプル側では `SetMeshMixSaturateShadow(true)` を呼び、強さを `SetMeshMixSaturateShadowIntensity()` に渡します。

### MeshMix Shadow Darkness

| パラメーター | 最小 | 最大 | ステップ |
|---|---:|---:|---:|
| `Lambert Darkness` | `0.0` | `1.0` | `0.05` |

MeshMix の陰影の暗さを調整します。`0.0` に近いほど影が弱く、`1.0` に近いほど暗く締まる、と考えて調整します。

### MeshMix Specular

| パラメーター | 最小 | 最大 | ステップ |
|---|---:|---:|---:|
| `Specular Intensity` | `0.0` | `2.0` | `0.05` |
| `Specular Edge` | `0.0` | `1.0` | `0.05` |

`Specular Intensity` はスペキュラの強さです。`Specular Edge` はスペキュラの出方、縁、鋭さ、境界に関係する値として扱います。金属や濡れた地面のように強い反射を出したい場合は `Intensity` を上げ、ギラつきすぎる場合は下げます。

## RenderSettings.csv

`Render::LoadSettingsCsv()` は、CSV を次のルールで読みます。

- 1行ごとに読む。
- `#` 以降はコメントとして無視する。
- 最初の `,` の左側をキー、右側を値として読む。
- 前後の空白は `Trim()` で削除する。
- キーが空でなければ `m_settings[key] = value` に保存する。

つまり、次のような書き方ができます。

```csv
# ポストエフェクト
DepthBufferShadowEnable,true
SSAOEnable,true
FogEnable,false
SaturateEnable,true
GaussianEnable,false
BloomEnable,true
DepthOfFieldEnable,false
StarBurstEnable,true

# 数値パラメーター
GaussianSampleSize,11
FogIntensity,2.0
ShadowIntensity,0.5
ShadowSaturationBoost,0.35
SSAOBrightness,1.0
SSAOSaturationBoost,0.30
BloomThreshold,2.5
DepthOfFieldFocalDistance,8.0
StarBurstThreshold,2.8
```

### bool 値として認識される文字列

`TryParseBoolSetting()` は、次の文字列を bool として認識します。大文字小文字は小文字化して比較しています。

| true 扱い | false 扱い |
|---|---|
| `1` | `0` |
| `true` | `false` |
| `on` | `off` |
| `yes` | `no` |

それ以外の文字列は無視されます。

### RenderSettings.csv のキー一覧

| キー | 型 | 対応 API | 既定値または失敗時の値 | 説明 |
|---|---|---|---:|---|
| `DepthBufferShadowEnable` | bool | `SetPostEffectDepthBufferShadow()` | 指定なしなら変更なし | 深度バッファシャドウの ON/OFF。 |
| `SSAOEnable` | bool | `SetPostEffectSSAO()` | 指定なしなら変更なし | SSAO の ON/OFF。 |
| `FogEnable` | bool | `SetPostEffectFog()` | 指定なしなら変更なし | フォグの ON/OFF。 |
| `SaturateEnable` | bool | `SetPostEffectSaturateEnable()` | 指定なしなら変更なし | 彩度フィルターの ON/OFF。 |
| `GaussianEnable` | bool | `SetPostEffectGaussianFilter()` | 指定なしなら変更なし | ガウシアンフィルターの ON/OFF。 |
| `GaussianSampleSize` | int | `SetPostEffectGaussianSampleSize()` | `m_gaussianSampleSize`、初期値 `101` | ガウシアンのサンプルサイズ。1〜101 の奇数へ補正される。 |
| `FogIntensity` | float | `SetPostEffectFogIntensity()` | `2.0f` | フォグ密度。 |
| `ShadowIntensity` | float | `SetPostEffectDepthBufferShadowIntensity()` | `0.5f` | 深度バッファシャドウの強さ。 |
| `SSAOBrightness` | float | `SetPostEffectSSAOBrightness()` | `1.0f` | SSAO の明るさ補正。 |
| `ShadowSaturationBoost` | float | `SetPostEffectDepthBufferShadowSaturationBoost()` | `0.35f` | 深度バッファシャドウ部分の彩度ブースト。 |
| `SSAOSaturationBoost` | float | `SetPostEffectSSAOSaturationBoost()` | `0.30f` | SSAO 部分の彩度ブースト。 |
| `BloomEnable` | bool | `SetPostEffectBloom()` | 指定なしなら変更なし | ブルームの ON/OFF。 |
| `BloomThreshold` | float | `SetPostEffectBloomThreshold()` | `2.5f` | ブルーム抽出しきい値。 |
| `DepthOfFieldEnable` | bool | `SetPostEffectDepthOfField()` | 指定なしなら変更なし | 被写界深度の ON/OFF。 |
| `DepthOfFieldFocalDistance` | float | `SetPostEffectDepthOfFieldFocalDistance()` | `8.0f` | 被写界深度の焦点距離。 |
| `StarBurstEnable` | bool | `SetPostEffectStarBurst()` | 指定なしなら変更なし | スターバーストの ON/OFF。 |
| `StarBurstThreshold` | float | `SetPostEffectStarBurstThreshold()` | `2.8f` | スターバースト抽出しきい値。 |

注意点として、`ApplySettings()` は `m_postEffectGauss.Initialize()` の直後、ブルーム、被写界深度、スターバーストの Initialize より前に呼ばれています。そのため、ブルームやスターバーストなど、Initialize 前に値を設定して問題ないかは各クラスの実装に依存します。もし CSV の値が効いていないエフェクトがあれば、`ApplySettings()` の呼び出し位置を全ポストエフェクト Initialize 後へ移動することを検討します。

## サンプル設定ダイアログの項目一覧

`Sample.rc` の `IDD_SETTINGS_DIALOG` には、各種描画パラメーターを変更する UI が定義されています。主な項目は次の通りです。

| UI 表示 | 対応する値・API |
|---|---|
| `Saturation` | `g_saturateLevel` → `SetPostEffectSaturate()` |
| `Animate Light` | `g_bAnimateLight`。平行光源の向きを自動更新する用途。 |
| `Remote Desktop` | `g_bRemoteDesktop`。リモートデスクトップ時のマウス処理切り替え。 |
| `Gaussian Filter` | `g_bGaussianFilter` → `SetPostEffectGaussianFilter()` |
| `Depth Buffer Shadow` | `g_bDepthBufferShadow` → `SetPostEffectDepthBufferShadow()` |
| `SSAO` | `g_bSSAO` → `SetPostEffectSSAO()` |
| `Bloom` | `g_bBloom` → `SetPostEffectBloom()` |
| `Depth Of Field` | `g_bDepthOfField` → `SetPostEffectDepthOfField()` |
| `StarBurst` | `g_bStarBurst` → `SetPostEffectStarBurst()` |
| `Window Mode` | `ChangeWindowMode()` |
| `Resolution` | `ChangeResolution()` |
| `DOF Focal Dist` | `SetPostEffectDepthOfFieldFocalDistance()` |
| `Gaussian Size` | `SetPostEffectGaussianSampleSize()` |
| `Fog Density` | `SetPostEffectFogIntensity()` |
| `Sun Light` | `SetLightBrightness()` |
| `Shadow Intensity` | `SetPostEffectDepthBufferShadowIntensity()` |
| `Shadow Saturation` | `SetPostEffectDepthBufferShadowSaturationBoost()` |
| `Lambert Sat` | `SetMeshMixSaturateShadow()` / `SetMeshMixSaturateShadowIntensity()` |
| `Lambert Darkness` | `SetMeshMixShadowDarkness()` |
| `Specular Intensity` | `SetMeshMixSpecularIntensity()` |
| `Specular Edge` | `SetMeshMixSpecularEdge()` |
| `SSAO Brightness` | `SetPostEffectSSAOBrightness()` |
| `SSAO Saturation` | `SetPostEffectSSAOSaturationBoost()` |
| `Bloom Threshold` | `SetPostEffectBloomThreshold()` |
| `StarBurst Threshold` | `SetPostEffectStarBurstThreshold()` |
| `Model Load Scale` | 新規モデル読み込み時の `scale`。 |
| `MeshMixManager (.x)` | MeshMix 系モデルの読み込み。POM / NormalMapping 選択あり。 |
| `Mesh (.x)` | 通常メッシュ読み込み。 |
| `Anim Mesh (.x)` | アニメーションメッシュ読み込み。 |
| `Skin Anim Mesh (.x)` | スキンアニメーションメッシュ読み込み。 |
| `MeshMix Skin Anim (.x)` | MeshMix 表現を使うスキンアニメーションメッシュ読み込み。 |
| `Loaded Models` | 読み込み済みモデル一覧。 |
| `Point Lights` | 追加済みポイントライト一覧。 |
| `PointLight R/G/B` | 点光源色。 |
| `PointLight Power` | 点光源の明るさ。 |
| `Add` | 現在の注視点などにポイントライトを追加する。 |

## サンプル UI のパラメーター範囲まとめ

| パラメーター | 最小 | 最大 | ステップ |
|---|---:|---:|---:|
| `Saturation` | `0.0` | `4.0` | `0.1` |
| `Fog Density` | `0.0` | `20.0` | `0.1` |
| `Sun Light` | `0.0` | `5.0` | `0.1` |
| `Shadow Intensity` | `0.0` | `1.0` | `0.05` |
| `Shadow Saturation` | `0.0` | `1.0` | `0.05` |
| `SSAO Brightness` | `0.25` | `4.0` | `0.05` |
| `SSAO Saturation` | `0.0` | `1.0` | `0.05` |
| `Lambert Sat` | `0.0` | `2.0` | `0.05` |
| `Lambert Darkness` | `0.0` | `1.0` | `0.05` |
| `Specular Intensity` | `0.0` | `2.0` | `0.05` |
| `Specular Edge` | `0.0` | `1.0` | `0.05` |
| `Bloom Threshold` | `0.0` | `5.0` | `0.1` |
| `DOF Focal Dist` | `0.5` | `50.0` | `0.1` |
| `StarBurst Threshold` | `0.0` | `5.0` | `0.1` |
| `Model Load Scale` | `0.1` | `10.0` | `0.1` |
| `PointLight R/G/B` | `0.0` | `1.0` | `0.05` |
| `PointLight Power` | `0.0` | `5.0` | `0.1` |
| `Gaussian Size` | `1` | `101` | 実装上は奇数へ補正 |

## 描画順の詳細メモ

現在の `Render::Draw()` の描画順は、見た目の調整で非常に重要です。

```text
DrawPass1
  ↓
GBuffer 作成
  ↓
Depth Buffer Shadow
  ↓
SSAO
  ↓
Fog
  ↓
Saturate
  ↓
Depth Of Field
  ↓
Bloom
  ↓
StarBurst
  ↓
Gaussian
  ↓
PostEffectEnd
  ↓
Draw2D
```

この順番だと、フォグや SSAO の結果に対して彩度フィルターがかかります。その後に被写界深度、ブルーム、スターバースト、ガウシアンがかかります。

検討ポイントは次の通りです。

- 彩度フィルターを SSAO / Shadow の後に置いているため、暗部の色味調整が画面全体に影響しやすい。
- Bloom と StarBurst は Gaussian より前なので、最後の Gaussian が光表現もまとめてぼかす。
- UI は最後に描かれるため、UI に Bloom や Gaussian はかからない。
- Depth Of Field が Bloom より前なので、ぼけた結果に Bloom が乗る。
- もし「光ってからボケる」見た目にしたいなら Bloom と DOF の順番を入れ替える実験価値がある。

## GBuffer の役割

`GBuffer` は、ポストエフェクト用に深度、ワールド座標、法線を作るためのクラスです。`Render::Draw()` では次のようなテクスチャを受け取っています。

```cpp
LPDIRECT3DTEXTURE9 pTexTempZ = NULL;
LPDIRECT3DTEXTURE9 pTexTempPos = NULL;
LPDIRECT3DTEXTURE9 pTexTempNoral = NULL;

m_GBuffer.Draw(m_meshMixList,
               m_meshMixSkinAnimList,
               &pTexTempZ,
               &pTexTempPos,
               &pTexTempNoral);
```

用途は次の通りです。

| テクスチャ | 主な用途 |
|---|---|
| `pTexTempZ` | 深度バッファシャドウ、SSAO、フォグ。 |
| `pTexTempPos` | SSAO、フォグ、被写界深度。 |
| `pTexTempNoral` | 深度バッファシャドウ、SSAO。名前は `Noral` になっているが、おそらく `Normal` の typo。 |

スクリーンスペース系の表現は、この GBuffer の品質に強く依存します。深度が荒い、法線が不正、ワールド座標がズレる、半ピクセル補正が合っていない、などがあると SSAO や Shadow の境界が破綻します。

## ウィンドウと解像度

`Render.h` のコメントでは、解像度は `1600x900` を基本として考えています。たとえば `1920x1080` は 1.2 倍の解像度として扱い、この倍率に従って UI サイズやフォントサイズを調整できる、という考え方です。

サンプルの初期ウィンドウサイズも次の通りです。

```cpp
constexpr int WINDOW_SIZE_W = 1600;
constexpr int WINDOW_SIZE_H = 900;
```

このため、UI レイアウトやスプライト座標の基準は 1600x900 と考えるとよいです。将来的に 1280x720、1920x1080、2560x1440 へ対応する場合は、基準解像度からのスケール係数を明確にした方がよいです。

## サンプルアプリの操作メモ

サンプルアプリは、`Sample` プロジェクト側でカメラ操作、設定ダイアログ、モデル追加、ポイントライト追加を行います。

### 初期化時の流れ

1. Common Controls を初期化する。
2. `CreateSampleWindow()` でウィンドウを作る。
3. `InitializeRemoteDesktopDefault()` を呼ぶ。
4. `LoadSampleSettingsFromCsv(L"RenderSettings.csv")` を呼ぶ。
5. `g_Render.Initialize(hWnd, L"RenderSettings.csv")` を呼ぶ。
6. カメラを設定する。
7. フォントを作る。
8. `cubeNormalInverse.x` と `plateField.x` を `AddMeshMix()` で追加する。
9. 各種設定値を `Apply...()` 系で Render に反映する。
10. 設定ダイアログを表示する。

### リモートデスクトップ設定

`Remote Desktop` チェックは、マウスルック時のカーソル処理に関係します。

通常時はクライアント領域中央へカーソルを戻しながら相対移動量を取ります。リモートデスクトップ時は、カーソルの強制センタリングがうまく動かないことがあるため、前回マウス位置との差分から回転量を計算する設計です。

## 開発時の注意点

### 相対パス問題

DirectX の `D3DXCreateEffectFromFile()` や `D3DXLoadMeshHierarchyFromX()` などに相対パスを渡す場合、基準は「exe の場所」ではなく「現在のカレントディレクトリ」になることがあります。ファイルオープンダイアログを使うとカレントディレクトリが変わることがあるため、相対パスが突然壊れる可能性があります。

対策は次のどれかです。

- 起動時に exe のあるディレクトリへ `SetCurrentDirectory()` する。
- すべてのリソースパスを exe 基準の絶対パスへ変換してから渡す。
- `Common` などに `GetExeDirectory()` と `MakeResourcePath()` を用意する。
- ファイルオープンダイアログを使う場合は、ダイアログ後にカレントディレクトリを戻す。

### デバイスロスト

`Render.h` では、`Font*` を `std::vector<Font*>` で持っており、コメントに「ポインターにしないとデバイスロストを扱う機能が機能しなくなる」とあります。DirectX9 では、フルスクリーン切り替え、Alt+Tab、解像度変更などでデバイスロスト対応が必要になります。

注意点は次の通りです。

- `D3DPOOL_DEFAULT` のリソースはロスト時に解放し、リセット後に作り直す。
- `ID3DXFont` や `ID3DXSprite` は `OnLostDevice()` / `OnResetDevice()` が必要な場合がある。
- RenderTarget テクスチャは解像度変更時に作り直す。
- `Common::AddDeviceLostResource(this)` に登録するクラスは、`OnDeviceLost()` と `OnDeviceReset()` の責務を明確にする。

### メモリ管理

現状は ComPtr を使わず、生ポインタと `SAFE_DELETE` / `Release()` を使っています。作者メモとしては、次の点に注意します。

- `LPDIRECT3DTEXTURE9` は `Release()` 漏れに注意。
- `NEW` した `AnimMesh` / `SkinAnimMesh` / `MeshInstancing` は確実に `SAFE_DELETE` する。
- `Remove...()` が「無効化」なのか「delete」なのかを API ごとに確認する。
- 長時間プレイ中にモデルを何度も追加/削除する場合、無効化だけではメモリが増え続ける可能性がある。

### `.x` ファイル読み込み

`.x` ファイルは、モデルによって読み込めるものと読み込めないものがあります。DirectX9 / D3DX の X ファイル読み込みは古く、Blender や MMD 系から出したファイルでは次の問題が起きやすいです。

- テキスト形式とバイナリ形式の違い。
- テンプレートやチャンクの互換性。
- スキンメッシュ、ボーン、アニメーションの形式差。
- テクスチャパスが絶対パスまたは不正な相対パスになっている。
- 法線、接線、頂点宣言が想定と違う。
- モデルサイズや座標系が極端に違う。
- マテリアル数やテクスチャ数が多すぎる。

問題が出たら、まずは単純な X ファイル、次にテクスチャ付き、次にボーン付き、という順で切り分けるとよいです。

## 今後追加したい説明・改善メモ

### API 名の整理

- `AddMeshInstansing` は `AddMeshInstancing` に直したい。
- `DrawText_` の末尾アンダースコアは理由がなければ整理したい。
- `pTexTempNoral` は `pTexTempNormal` に直したい。

### 設定値の一元化

現在は、サンプル UI の定数、`RenderSettings.csv` のキー、Render 側のデフォルト値が分散しています。将来的には次のようにしたいです。

- `RenderSettingsDefinition.h` のようなファイルにキー名、最小値、最大値、デフォルト値をまとめる。
- サンプル UI も CSV 読み込みも同じ定義を参照する。
- README の表も、その定義から生成できるようにする。

### プリセット化

見た目調整用に、次のようなプリセットを持つと便利です。

- `Natural`：自然で軽い設定。
- `Anime`：彩度高め、影は柔らかめ。
- `Realistic`：SSAO、Shadow、Bloom を控えめに使う。
- `Screenshot`：重くてもよいので Bloom、DOF、StarBurst を強める。
- `Debug`：ポストエフェクトを切って素のモデルを確認する。

### ポストエフェクト順序の実験

現在の順序が悪いわけではありませんが、表現によっては次の実験価値があります。

- `Saturate` を Shadow / SSAO の前に置く。
- `Bloom` を `DepthOfField` の前後で比較する。
- `Gaussian` を最後ではなく Bloom 内部だけに限定する。
- UI にもポストエフェクトをかけるモードを用意する。
- Debug 表示として GBuffer の Z / Position / Normal を画面に出す。

## よく使う調整方針

### 画面が暗すぎる場合

1. `Sun Light` を上げる。
2. `Shadow Intensity` を下げる。
3. `SSAO Brightness` を上げる。
4. `Lambert Darkness` を下げる。
5. `Bloom Threshold` を下げすぎていないか確認する。

### 画面が白っぽい場合

1. `Fog Density` を下げる。
2. `Bloom Threshold` を上げる。
3. `StarBurst Threshold` を上げる。
4. `Gaussian Filter` を切る。
5. `Saturation` を 1.0 付近へ戻す。

### 影が汚い場合

1. GBuffer の深度と法線を確認する。
2. 半ピクセルずれが起きていないか確認する。
3. `Shadow Intensity` を下げる。
4. `Shadow Saturation` を下げる。
5. SSAO と Depth Buffer Shadow を片方ずつ ON/OFF して原因を分ける。

### モデルが重い場合

1. テクスチャの重複ロードを確認する。
2. `.x` ファイルのメッシュ数、マテリアル数、テクスチャ数を確認する。
3. 法線再計算や接線生成を毎回やっていないか確認する。
4. 同じモデルが大量にあるなら `MeshInstancing` を使う。
5. まずポストエフェクトを全部切って、モデル単体の負荷を確認する。

### ポストエフェクトが重い場合

1. `Gaussian Size` を下げる。
2. Bloom / StarBurst / DOF / SSAO を個別に切って負荷を見る。
3. GBuffer 生成の対象メッシュを減らせないか確認する。
4. 解像度を下げて負荷変化を見る。
5. フル解像度ではなく半解像度のポストエフェクト化を検討する。

## README 更新時のメモ

この README の上部は他人向け紹介文として残し、この「自分用詳細説明書」は実装が増えるたびに更新します。特に次を変更したときは README も直すべきです。

- `Render.h` の公開 API を追加・削除したとき。
- `RenderSettings.csv` のキーを増やしたとき。
- サンプル設定ダイアログのスライダー範囲を変えたとき。
- ポストエフェクトの描画順を変えたとき。
- `MeshMix` のパラメーターやプリセットを変えたとき。
- DirectX9 のデバイスロスト処理を整理したとき。
- `.x` ファイル読み込みの仕様や制限を見つけたとき。
