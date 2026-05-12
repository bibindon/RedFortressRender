# ライブラリの仕組み

この文書は `RedFortressRender` の内部構造を説明するものである。
「ゲーム側からどう呼ぶか」ではなく、「`Render::Draw()` の中で何が起きるか」「クラスがどう役割分担しているか」に重点を置く。

## 全体像

このライブラリの中心は `NSRender::Render` である。
`Render` は次の責務をまとめて持つ。

- Direct3D デバイスの利用窓口
- ウィンドウモードや解像度変更の制御
- シーン内オブジェクトの保持
- カメラとライトの利用
- GBuffer 生成
- ポストエフェクト適用
- 2D 文字・画像描画
- デバイスロスト / リセット対応

設計としては「多くの描画クラスを `Render` が束ねるハブ型」である。
各メッシュクラスやポストエフェクトクラスはそれぞれ独立しているが、毎フレームの描画順、レンダーターゲットの受け渡し、画面への最終出力は `Render` が統括する。

## 大きな構成要素

大別すると次の 6 層でできている。

1. 基盤層
   `Common`, `WindowManager`, `Camera`, `Light`
2. シーンオブジェクト層
   `MeshOld`, `MeshMixManager`, `AnimMesh`, `SkinAnimMesh` など
3. GBuffer 層
   `GBuffer`
4. ポストエフェクト層
   `PostEffect...` 群
5. 2D 層
   `Font`, `Sprite`
6. 統合層
   `Render`

以下、それぞれの役割を詳しく見る。

## 基盤層

### `Common`

`Common` は共有状態を持つユーティリティ層である。

主な役割:

- `LPDIRECT3DDEVICE9` の保持
- 画面サイズの保持
- `IDeviceResettable` リソースの一覧管理
- デバイスロスト時の一括通知
- 基準解像度 `1600x900` に対するスケーリング補助

特に重要なのは `m_resourceList` である。
デバイスリセットが必要なクラスは `Common::AddDeviceLostResource(this)` で登録し、解像度変更やウィンドウモード変更のときに `OnDeviceLost()` / `OnDeviceReset()` を一括で受ける。

### `WindowManager`

`WindowManager` は Direct3D デバイス生成と Reset を担う。
初期化時には `Direct3DCreate9()` と `CreateDevice()` を行い、生成したデバイスを `Common::SetD3DDevice()` に渡す。

役割は次の通りである。

- Direct3D9 オブジェクトの保持
- デバイス生成
- 解像度変更
- `WINDOW`, `BORDERLESS`, `FULLSCREEN` の切り替え
- フルスクリーン候補解像度列挙

`ChangeWindowMode()` は即時変更ではなく、`Render::Draw()` の最後から呼ばれて反映される。
この構造により、描画中ではなくフレーム終端で安全に Reset しやすくなっている。

### `Camera`

`Camera` は static クラスであり、グローバルなカメラ状態を持つ。

保持するもの:

- eye 位置
- lookAt 位置
- near / far
- view angle
- 一部カメラモード

`Render` はカメラ本体をメンバーとして持たず、`Camera::SetEyePos()` や `Camera::GetViewMatrix()` を通して利用する。
そのため、設計としては「`Render` がカメラを所有する」というより、「`Render` が共有カメラ状態を利用する」に近い。

### `Light`

`Light` も static クラスである。
平行光源、環境光、ポイントライトリストを保持する。

保持内容:

- 平行光源方向
- 平行光源色
- 平行光源強度
- 環境光色
- 環境光強度
- `PointLightInfo` の deque

各メッシュやシェーダは、描画時に `Light` から現在の状態を取得する。

## シーンオブジェクト層

### `Render` が保持するコンテナ

`Render` は内部でオブジェクトを次のように持つ。

- `std::deque<MeshOld> m_meshList`
- `std::deque<MeshSmooth> m_meshSmoothList`
- `std::deque<MeshSSSLike> m_meshSSSLikeList`
- `std::deque<MeshSSS> m_meshSSSList`
- `std::deque<MeshPointLight> m_meshPointLightList`
- `std::deque<MeshNormalMapping> m_meshNormalMapList`
- `std::deque<MeshPOM> m_meshPOMList`
- `std::deque<MeshMixManager> m_meshMixList`
- `std::vector<AnimMesh*> m_animMeshList`
- `std::vector<SkinAnimMesh*> m_skinAnimMeshList`
- `std::vector<MeshMixSkinAnim*> m_meshMixSkinAnimList`
- `std::unordered_map<std::wstring, MeshInstancing*> m_meshInstancingMap`

ここから分かる通り、所有方式は完全には統一されていない。

- 値保持の `deque`
- 生ポインタ保持の `vector`
- キー付きポインタ保持の `unordered_map`

という混成である。

### `MeshOld`

最も基本的な静的メッシュ描画である。
`AddMesh()` と `AddMeshNoLighting()` は最終的に `MeshOld` を使う。

### `MeshSmooth`

法線の滑らかさを重視する静的メッシュである。

### `MeshSSSLike` / `MeshSSS`

SSS 風、あるいは SSS 用の個別系統である。

### `MeshPointLight`

ポイントライト前提の静的メッシュである。

### `MeshNormalMapping`

法線マップ別指定型である。

### `MeshPOM`

視差遮蔽マッピング専用系である。

### `AnimMesh`

非スキン型のアニメーションメッシュである。
内部では `AnimController` と `AnimMeshAllocator` を使う。

### `SkinAnimMesh`

ボーンスキニング型アニメーションメッシュである。
`Render()` の引数として view/proj/light を直接受ける。

### `MeshMixSkinAnim`

`MeshMix` 的な見た目設定とスキンアニメーションを統合したクラスである。
GBuffer 側には `RenderToEffect()` を持ち、通常描画と GBuffer 描画の両方に対応する。

### `MeshInstancing`

同一メッシュの大量配置用である。
ただし現在の `Render::AddMeshInstansing()` 実装は、インスタンスごとの回転やスケールには対応していない。

## `MeshMix` 系が主力になっている理由

現行構成では `MeshMix` 系が最も重要である。
理由は、ポストエフェクト連携に必要な情報を最も多く持っているからである。

`stMeshParam` には次のような見た目情報が詰まっている。

- ambient
- specularIntensity / specularEdge
- smooth
- shadow / saturateShadow / shadowDarkness
- cubeMapping
- parallaxOcclusionMapping
- normalMapping
- glass / emit
- wave / sway
- pointLight
- ssao
- collision
- sss

つまり `MeshMix` は単なるメッシュクラスではなく、「材質と描画振る舞いをまとめた実行単位」である。

## `MeshMixManager` の役割

### なぜ `MeshMix` ではなく `MeshMixManager` が主力なのか

`Render` が持っているのは `MeshMix` ではなく `MeshMixManager` である。
これは、複数の `MeshMix` に共通なエフェクトやロード制御を共有したいためである。

`MeshMixManager.h` のコメントにもある通り、D3DXEFFECT をメッシュごとに作るのではなく、複数メッシュで共有するためのクラスとして設計されている。

### 共有エフェクト

`MeshMixManager.cpp` には次の共有状態がある。

- `GetSharedEffect()`
- `GetSharedEffectRefCount()`
- `GetSharedEffectLostState()`
- `GetSharedThicknessTexture()`

これにより、`MeshMix` 系メッシュごとにエフェクトを個別生成せずに済む。
参照カウントが 0 になると共有エフェクトを解放する。

### 非同期ロード

`Initialize(bool async)` は `async = true` のとき内部で `std::thread` を立て、`InitializeInternal()` を別スレッドで実行する。
これが `AddMeshMix()` の既定挙動である。

ロード中は `m_bLoaded == false` のままであり、描画時に `IsLoaded()` を見てスキップされる。

### `.x` 隣接 CSV 読み込み

`InitializeInternal()` の最初の方で `ReadCsvParam(tempPath)` を呼び、`.x` の横にある `.csv` を読む。
ここで次のような上書きが行われる。

- `meshtype=pom` なら POM + NormalMapping
- `meshtype=normalmapping`
- `meshtype=envmapping`
- `meshtype=glass`
- `meshtype=emit`
- `smooth`
- `sss`
- `sssintensity`
- `ssscolor`
- `sway`
- `wave`
- `litbypointlight`
- `shadow`
- `lambertshadow`
- `ssao`
- `collision`
- `cubemappingrate`
- `cubemappinggauss`

つまり `MeshMixManager` は「モデルファイルの読込」と「見た目設定の統合」を同時に担っている。

## GBuffer 層

### `GBuffer` が持つもの

`GBuffer` は次のレンダーターゲットを持つ。

- Z テクスチャ
- World Position テクスチャ
- Normal テクスチャ
- Thickness テクスチャ

フォーマットは用途に応じて分かれている。

- 深度: `D3DFMT_R32F`
- Position: `D3DFMT_A16B16G16R16F`
- Normal: `D3DFMT_A16B16G16R16F`
- Thickness: `D3DFMT_A16B16G16R16F`

### GBuffer の対象メッシュ

`GBuffer::Draw()` は全メッシュを描くわけではない。
対象は基本的に次の 2 系統である。

- `std::deque<MeshMixManager>`
- `std::vector<MeshMixSkinAnim*>`

しかも `MeshMixManager` 側では次の条件を満たしたものだけが描かれる。

- `IsEnabled() == true`
- `IsLoaded() == true`
- `IsSsaoEnabled() == true`

ここで `ssao` フラグが GBuffer 参加フラグとしても機能している点が重要である。
完全に「SSAO の有無」だけではなく、「スクリーンスペース系の対象になるか」の意味を兼ねている。

### Thickness パス

`GBuffer::Draw()` では前面深度だけでなく、別パスで Thickness も作る。
このとき `g_texFrontDepth` を使ってバックフェイス深度を扱う。
この厚みテクスチャは `MeshMixManager::SetSharedThicknessTexture()` 経由で共有され、SSS や SSAO などで利用される。

## `Render::Initialize()` の流れ

初期化順はおおむね次の通りである。

1. `LoadSettingsCsv()`
2. `WindowManager::Initialize()`
3. `Sprite::Initialize()`
4. レンダーターゲット生成
5. `GBuffer::Initialize()`
6. `PostEffectZShadow::Initialize()`
7. `PostEffectSSAO::Initialize()`
8. `PostEffectFog::Initialize()`
9. `PostEffectHeightFog::Initialize()`
10. `PostEffectSaturate::Initialize()`
12. `PostEffectGauss::Initialize()`
13. `PostEffectMaskedGauss::Initialize()`
14. `PostEffectFXAA::Initialize()`
15. `PostEffectMotionBlurCamera::Initialize()`
16. `ApplySettings()`
17. `PostEffectBloom::Initialize()`
18. `PostEffectDepthOfField::Initialize()`
19. `PostEffectStarBurst::Initialize()`
20. `PostEffectGodRay::Initialize()`
21. `PostEffectEnd::Initialize()`
22. `Common::AddDeviceLostResource(this)`

ここで注意点が 1 つある。
`ApplySettings()` は Bloom、DOF、StarBurst、GodRay の初期化前に呼ばれている。
現状は各クラス側の setter が耐えている前提で動いているが、将来初期化順を見直す余地はある。

## `Render::Draw()` の本当の流れ

毎フレームの描画順は非常に重要である。

### 1. FPS 用文字列の積み込み

`m_bShowFPS` が true の場合、`CalcFPS()` と `ShowFPS()` が呼ばれる。
この時点ではまだ画面には描かれず、フォント描画キューに積まれる。

### 2. GBuffer 生成

まず `m_GBuffer.Draw(...)` が走る。
ここで Z / Position / Normal / Thickness が作られる。

### 3. Thickness の共有

`MeshMixManager::SetSharedThicknessTexture(pTexTempThickness)` が呼ばれ、後続シェーダ群が厚み情報を参照できるようになる。

### 4. 本描画 `DrawPass1(true)`

ここで通常の 3D シーンを `m_pRenderTarget1` と `m_pRenderTarget2` へ描く。
`DrawPass1()` は内部で MRT をセットし、各メッシュコンテナを順番に描く。

描画順は次の通りである。

1. `MeshOld`
2. `MeshSmooth`
3. `MeshSSSLike`
4. `MeshSSS`
5. `MeshPointLight`
6. `MeshNormalMapping`
7. `MeshPOM`
8. `AnimMesh`
9. `SkinAnimMesh`
10. `MeshMixSkinAnim`
11. `MeshInstancing`
12. `MeshMixManager`

### 5. ポストエフェクト連結

その後 `pTempTexture` に現在の結果を持ち回しながら、順にポストエフェクトを適用していく。

順番は次の通りである。

1. Depth Buffer Shadow
2. SSAO
3. Fog
4. Height Fog
5. Saturate
6. Depth Of Field
7. Bloom
8. StarBurst
9. GodRay
10. Gaussian
11. Masked Gaussian
12. Motion Blur Camera
13. FXAA
14. PostEffectEnd

この順番は見た目に直接影響する。
たとえば、

- UI が後描きなのでブルームは乗らない
- DOF が Bloom より前なので、ぼけた結果に Bloom が乗る
- Motion Blur Camera は Gaussian より後ではなく前でもなく、その後段に FXAA がある

という画面設計になっている。

### 6. Debug GBuffer View

最終転送時には `m_debugGBufferView` を見て分岐する。
通常は `pTempTexture` を画面へ出すが、デバッグ中は GBuffer の WorldPos / Normal / Depth / Thickness を直接表示できる。

### 7. 2D 描画

最後に `Draw2D()` が呼ばれる。
ここで `Font::Draw()` と `Sprite::Draw()` が実行される。

### 8. Present とウィンドウモード反映

最後に `Present()` し、その直後に `m_windowManager.ChangeWindowMode()` が走る。

## `DrawPass1()` の意味

`DrawPass1()` は「シーン本体をレンダーターゲットへ描く工程」である。
ここではポストエフェクトはまだ行わない。

処理の中では:

- RT0 を退避
- `m_pRenderTarget1` を RT0 に設定
- `m_pRenderTarget2` を RT1 に設定
- クリア
- `BeginScene()`
- 各メッシュ描画
- `EndScene()`
- RT を元に戻す

という流れである。

`m_pRenderTarget1` は HDR 的な `D3DFMT_A16B16G16R16F`、`m_pRenderTarget2` は `D3DFMT_A8R8G8B8` であり、描画パスや後処理に応じて使い分けている。

## 2D 層

### `Font`

`Render::SetUpFont()` で `Font*` を生成し、`m_fontList` に積む。
文字描画 API はすべて `Font` へ文字列を蓄積し、`Draw2D()` でまとめて描画する。

### `Sprite`

画像描画担当である。
`DrawImage()` は毎回 `LoadImage_()` と `PlaceImage()` を呼ぶため、頻繁な画像切り替え時のキャッシュ戦略は `Sprite` 側実装に依存する。

## デバイスロスト / リセット

### なぜ必要か

DirectX9 では解像度変更、ウィンドウモード変更、Alt+Tab などでデバイスロスト / Reset 対応が必要になる。

このライブラリでは `IDeviceResettable` を共通インターフェースとし、`Common` に登録したオブジェクトへ一括通知する形を採っている。

### 通知の流れ

`WindowManager::ResetDeviceForMode()` は次の順で動く。

1. `Common::OnDeviceLostAll()`
2. `IDirect3DDevice9::Reset()`
3. `Common::OnDeviceResetAll()`

### `Render` 自身の対応

`Render::OnDeviceLost()` では:

- `m_pRenderTarget1` 解放
- `m_pRenderTarget2` 解放
- `m_sprite.OnDeviceLost()`

`Render::OnDeviceReset()` では:

- `CreateTexture()`
- `m_sprite.OnDeviceReset()`

が走る。

各ポストエフェクトや `GBuffer`、`MeshMixManager` なども個別に `OnDeviceLost()` / `OnDeviceReset()` を持つ。

## `RenderSettings.csv` の内部処理

`Render::LoadSettingsCsv()` は単純な `key,value` パーサである。
前後空白を `Trim()` し、`#` 以降はコメントとして捨て、`m_settings` へ入れる。

`ApplySettings()` はそのマップを見て、各 setter を順番に呼んでいく。
ここで重要なのは、「CSV は直接クラス内部値を書き換えるのではなく、基本的に公開 setter 経由で反映する」点である。

このため、CSV と API の整合性は比較的保たれやすい。

## `MeshMix` パラメーターの伝播

`Render` は `m_meshMixSaturateShadowEnabled` や `m_meshMixSpecularIntensity` のような「現在の既定値」を持つ。

これらは 2 通りに使われる。

1. `AddMeshMix()` / `AddMeshMixSkinAnim()` 時に `stMeshParam` の初期値として注入される
2. `SetMeshMix...()` 系を呼んだとき、既存メッシュへ一括反映される

この構造により、ゲーム側は

- 追加前の既定値決め
- 追加後のリアルタイム一括調整

を同じ API 群で扱える。

## 設計上の特徴

### 1. static 状態が多い

`Camera` と `Light` が static であるため、厳密には `Render` が完全に自己完結しているわけではない。
複数レンダラーを同時に独立動作させる設計には向かない。

### 2. 所有権管理は古典的

ComPtr や `unique_ptr` ではなく、生ポインタと `SAFE_RELEASE` / `SAFE_DELETE` が中心である。
DirectX9 時代の実装としては自然だが、所有権の境界はコードをよく読まないと分かりづらい。

### 3. 描画と設定が密結合

`Render` は描画だけでなく、設定 CSV 解釈や UI 連携前提の既定値も持つ。
ライブラリとしては便利だが、純粋な描画バックエンドとしては責務が広めである。

### 4. `MeshMix` が事実上の標準パス

GBuffer、SSAO、Depth Shadow、厚み情報、隣接 CSV、共有エフェクトなど、進んだ機能は `MeshMix` 系に集中している。
このリポジトリを理解する上では、`MeshMixManager` を中心に追うのが最も効率的である。

## 読み進める順番のおすすめ

内部をさらに追うなら、次の順で読むと理解しやすい。

1. `Render.h`
2. `Render.cpp`
3. `GBuffer.h` / `GBuffer.cpp`
4. `MeshMix.h`
5. `MeshMixManager.h` / `MeshMixManager.cpp`
6. `PostEffect...` 各クラス
7. `WindowManager.cpp`
8. `Sample\main.cpp`

`Render::Draw()` の流れを先に掴んでから個別クラスへ降りると、各クラスが「どの段に参加しているか」が見えやすい。
