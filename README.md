# RedFortressRender

DirectX 9 ベースの 3D 描画ライブラリと、その確認用サンプルをまとめたリポジトリである。
ゲーム本体で必要になった描画機能を `Render` に集約し、`Sample` から見た目やパラメーターを確認できる構成になっている。

## 構成

- `Render`
  描画ライブラリ本体である。メッシュ描画、GBuffer、ポストエフェクト、2D 描画を持つ。
- `Sample`
  ライブラリ確認用のアプリである。設定ダイアログ、モデル読み込み、カメラ操作、ポイントライト追加ができる。
- `GrassLandSample`
  別サンプルの実験用プロジェクトである。
- `UnitTest1`
  テスト用プロジェクトである。
- `MikuDanceSample`
  現在はメモのみ置かれており、ソリューションには含まれていない。

ソリューションは `Render.sln` である。

## 現在入っている主な機能

- 通常メッシュ描画
- スムーズ法線メッシュ描画
- Normal Mapping
- Parallax Occlusion Mapping
- SSS 風表現
- 環境マッピング
- ポイントライト
- アニメーションメッシュ
- スキンアニメーションメッシュ
- メッシュインスタンシング
- GBuffer 生成
- Depth Buffer Shadow
- SSAO
- Fog と Height Fog
- Saturate
- Gaussian / Masked Gaussian
- Bloom
- Depth Of Field
- StarBurst
- God Ray
- FXAA
- Camera Motion Blur
- スプライト描画
- フォント描画

## ビルド環境

- Windows
- Visual Studio 2022 系
- DirectX 9
- NuGet package: `Microsoft.DXSDK.D3DX.9.29.952.8`

実運用では `x64` 構成を前提にしておくのが無難である。
`Sample` と `Render` の x64 構成では `v145`、Win32 構成では `v143` が使われている。

## ビルド方法

### Visual Studio

1. `Render.sln` を開く
2. `Sample` をスタートアッププロジェクトにする
3. 構成を `Debug|x64` か `Release|x64` にする
4. ビルドして実行する

## 実行時の流れ

`Sample\main.cpp` ではおおむね次の順で初期化している。

1. `LoadSampleSettingsFromCsv(L"RenderSettings.csv")`
2. `g_Render.Initialize(hWnd, L"RenderSettings.csv")`
3. カメラ設定
4. フォント作成
5. 既定モデル追加
6. 設定値を `Render` に反映
7. 設定ダイアログ表示

`RenderSettings.csv` は Sample 側と Render 側の両方で読まれる。
画面設定の復元と、描画ライブラリの初期値設定を同じファイルで兼ねている。

## サンプルの既定モデル

初期状態では次の 2 つが読み込まれる。

- `Sample\cubeNormalInverse.x`
- `Sample\plateField.x`

追加読み込み用のサンプルモデルと補助 CSV は `Sample` フォルダーにある。
`.x` の隣に同名の `.csv` を置くと、`MeshMix` 系の見た目設定を自動で読む。

詳しくは以下を参照のこと。

- `README_about_library_csv.md`
- `README_about_x_csv.md`
- `README_about_sample_x.md`

## ポストエフェクト適用順

現在の `Render::Draw()` の順序は次の通りである。

1. 3D 描画
2. GBuffer 生成
3. Depth Buffer Shadow
4. SSAO
5. Fog
6. Height Fog
7. Saturate
8. Depth Of Field
9. Bloom
10. StarBurst
11. God Ray
12. Gaussian / Masked Gaussian
13. Motion Blur Camera
14. FXAA
15. 最終転送
16. 2D 描画

UI や文字は最後に描かれるため、通常はポストエフェクトの影響を受けない。

## よく触る API

中心になるクラスは `NSRender::Render` である。

- `Initialize(HWND hWnd, const std::wstring& settingsCsvPath = L"")`
- `Finalize()`
- `Draw()`
- `SetCamera(...)`
- `AddMeshMix(...)`
- `AddMeshMixSkinAnim(...)`
- `AddAnimMesh(...)`
- `AddSkinAnimMesh(...)`
- `AddMeshInstansing(...)`
- `AddPointLight(...)`

`AddMeshInstansing` は現在の API 名がそのままスペルミス込みで残っている。
コード側でもこの名前で使われている。

## 既知の注意点

- リソースの相対パスは実行時カレントディレクトリの影響を受ける。
- `.x` ファイル読み込みは古い D3DX 依存のため、モデルによって相性がある。
- デバイスロストや解像度変更を考えると、x64 のサンプル構成で確認する方が安全である。
- Shader は `Render\shader` が正本である。`Sample` 側はビルド構成によってコピーして使う。

## ドキュメント更新の基準

次を変えたときは README も更新すること。

- `Render::ApplySettings()` が読むキー
- `MeshMixManager` が読む `.x` 隣接 CSV のキー
- `Sample` の初期ロードモデル
- `Render::Draw()` のポストエフェクト順
- ビルド手順や必要ツールチェーン
