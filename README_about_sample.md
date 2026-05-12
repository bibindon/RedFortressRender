
# サンプルプログラムの使い方と仕組み

この文書は `Sample` プロジェクトの説明書である。
`Render` ライブラリ本体の API 説明ではなく、「確認用アプリとして何ができるか」「内部でどう状態を持ち、どう描画へ反映しているか」を詳しくまとめる。

`Sample` は単なる最小表示サンプルではない。
モデル読み込み、設定ダイアログ、CSV による初期設定、ポストエフェクト切り替え、ライト追加、2D 表示確認までまとめて行える、ライブラリ検証用の統合フロントエンドである。

## 目的

`Sample` には大きく 4 つの役割がある。

- `Render` ライブラリの初期化手順を実際に示す
- 各種 3D 描画機能とポストエフェクトをその場で試す
- `RenderSettings.csv` や `.x` 隣接 `.csv` の効き方を確認する
- モデル追加、ライト追加、ウィンドウモード変更など実運用に近い操作をまとめて検証する

そのため、サンプルコードは「最短の書き方」よりも「触りながら確認しやすい構成」を優先している。

## サンプルの起動

基本的な起動手順は次の通りである。

1. `Render.sln` を開く
2. `Sample` をスタートアッププロジェクトにする
3. `Debug|x64` か `Release|x64` でビルドする
4. 実行する

起動すると、メインウィンドウと設定ダイアログが開く。
初期状態では `RenderSettings.csv` が読み込まれ、いくつかの既定モデルも自動で配置される。

## 起動時に何が行われるか

初期化の中心は `Sample\main.cpp` の `InitializeSampleScene()` である。
おおむね次の順序で処理される。

1. Raw Input を登録する
2. リモートデスクトップ向け既定値を初期化する
3. `LoadSampleSettingsFromCsv(L"RenderSettings.csv")` でサンプル側の状態を復元する
4. `g_Render.Initialize(hWnd, L"RenderSettings.csv")` でライブラリ本体を初期化する
5. カメラ位置と注視点を設定する
6. フォントを作成する
7. 既定モデルを追加する
8. `ApplyAllSampleSettings()` でサンプル側の保持値を `Render` へ反映する

ここでいう「リモートデスクトップ向け既定値」は常時 ON ではない。
`InitializeRemoteDesktopDefault()` は現在時刻を見て、平日の 9:00 以上 18:00 未満の時間帯だけ `g_bRemoteDesktop` を自動で ON にする。
つまり既定挙動は「平日昼間だけ自動 ON、それ以外は自動 OFF」である。

ここで重要なのは、`RenderSettings.csv` を Sample 側と Render 側の両方が読むことである。

- Sample 側
  UI の状態、トグル、スライダー相当値、ライト色、エフェクト有効状態などを復元する
- Render 側
  描画ライブラリ内部の near/far、ポストエフェクト、各種初期パラメーターを復元する

この二重読み込みにより、サンプル画面に出ている UI 状態と、実際の描画設定がずれにくい構造になっている。

## 初期表示される内容

起動直後には次の 2 つが読み込まれる。

- `Sample\cubeNormalInverse.x`
- `Sample\plateField.x`

これらはライブラリ全体の見た目確認に使う土台である。
床や基準形状が最初から存在するため、Fog、Shadow、SSAO、Bloom、GodRay などをすぐ試せる。

## 画面構成

サンプル画面は大きく 3 つの要素でできている。

### 1. 3D 表示ウィンドウ

`Render::Draw()` の結果が表示される本体ウィンドウである。
カメラ移動、モデル追加、ライト追加、ポストエフェクト確認の中心になる。

### 2. 設定ダイアログ

`F1` で表示切り替えできる補助 UI である。
各種チェックボックス、スライダー、数値欄、コンボボックス、モデル一覧、ポイントライト一覧を持つ。

### 3. オーバーレイ表示

画面左上に操作説明やカメラ位置などを描く。
`F2` で表示を切り替えられる。

## 基本操作

主なキー操作は次の通りである。

- `W` `A` `S` `D`
  カメラ前後左右移動
- `Q` `E`
  カメラ上下移動
- `↑` `↓` `←` `→`
  カメラ回転
- `Esc`
  マウスルック ON/OFF
- `F1`
  設定ダイアログ表示切り替え
- `F2`
  オーバーレイ表示切り替え
- `F3`
  GBuffer の WorldPos デバッグ表示切り替え
- `F4`
  GBuffer の Normal デバッグ表示切り替え
- `F5`
  GBuffer の Depth デバッグ表示切り替え
- `F6`
  GBuffer の Thickness デバッグ表示切り替え
- `8`
  通常ウィンドウ
- `9`
  ボーダーレス
- `0`
  フルスクリーン

これらのショートカットは `Sample\InputHandlers.cpp` にまとまっている。
メインウィンドウのメッセージ処理から呼ばれるが、実際の操作内容は InputHandlers 側へ集約されている。

## モデル追加と簡易操作

サンプルはキーボードだけでもかなりの確認ができる。

- `M`
  `MeshMix` を現在の注視点へ追加する
- `Shift + M`
  POM メッシュを追加する
- `Ctrl + M`
  SSS メッシュを追加する
- `Shift + Ctrl + M`
  単純メッシュを追加する
- `N`
  アニメーションメッシュを追加する
- `Shift + N`
  法線マップ付きメッシュを追加する
- `K`
  スキンアニメーションメッシュを追加する
- `I`
  インスタンシングメッシュを追加する
- `O`
  ポイントライト影響付きメッシュを追加する

追加位置はカメラ位置そのものではなく `GetLookAtPos()` を基準にしている。
そのため、「今見ている場所へ置く」感覚で使える。

## 2D 表示確認

3D だけでなく、2D 描画の確認もできる。

- `P`
  ランダム位置へ画像を追加する
- `Shift + P`
  画像を全消去する
- `Ctrl + P`
  ポイントライトを追加する
- `C`
  ランダム位置へテキストを追加する
- `Shift + C`
  テキストを全消去する

この 2D 要素は `g_imageInfoList` と `g_textInfoList` に保持され、毎フレームのオーバーレイ描画でまとめて表示される。

## エフェクト切り替えショートカット

頻繁に試すポストエフェクトはキーボードから直接切り替えられる。

- `G`
  Gaussian Filter ON/OFF
- `T`
  Saturate Filter ON/OFF
- `B`
  Bloom ON/OFF
- `Shift + B`
  StarBurst ON/OFF
- `U`
  Depth Of Field モード切り替え
- `H`
  Depth Buffer Shadow ON/OFF
- `J`
  SSAO ON/OFF
- `V`
  Fog ON/OFF
- `Shift + S`
  Saturation 値を上げる
- `Ctrl + S`
  Saturation 値を下げる

トグル後は `ApplyPostEffectToggleSettings()` や各 `Apply...()` 関数が呼ばれ、`Render` 側へ即時反映される。

## 設定ダイアログでできること

設定ダイアログはサンプルの中心機能である。
単なる補助 UI ではなく、Sample 側の状態編集と Render 側の実設定反映をつなぐ制御面になっている。

主なカテゴリは次の通りである。

- Saturation
- Gaussian / Masked Gaussian
- FXAA
- Camera Motion Blur
- Fog / Height Fog
- Sun Light / Ambient Light
- Depth Buffer Shadow
- SSAO
- Specular / SSS
- Bloom / Depth Of Field / StarBurst
- モデル読み込み
- Loaded Models 一覧
- Point Lights 一覧
- GodRay
- 解像度とウィンドウモード

### モデル読み込み

各 `Open...` ボタンから `.x` ファイルを選択し、その場でシーンへ追加できる。
内部では `SpawnMeshAtLookAt()`、`SpawnMeshMixAtLookAt()`、`SpawnAnimMeshAtLookAt()` などが呼ばれる。

### 読み込んだモデル一覧

Loaded Models 一覧には、サンプルが追加した描画オブジェクトの種別、パス、ID 対応情報が集約される。
この一覧は `g_loadedModelList` で管理している。

ここで持っている情報は次の通りである。

- どの API 系で追加したか
- どのファイルを読んだか
- どこへ置いたか
- どの Render ID に対応するか

削除時にはこの情報を見て `RemoveMesh()`、`RemoveMeshMix()`、`RemoveAnimMesh()` など適切な削除 API に振り分ける。

### ポイントライト管理

ポイントライトの色、明るさ、形状、線長、矩形サイズ、回転を UI 上で編集できる。
`Add` ボタンや `Ctrl + P` で追加するときは、いまダイアログで設定されている値がそのままテンプレートとして使われる。

### CSV 再読込

設定ダイアログには `Load CSV...` ボタンがある。
ここから任意の `RenderSettings.csv` 系ファイルを後読みできる。

この操作では次の順で再反映する。

1. `LoadSampleSettingsFromCsv()` で Sample 側状態を読み直す
2. `g_Render.ReloadSettingsCsv()` で Render 側設定を読み直す
3. `ApplyAllSampleSettings()` で Sample 側保持値を Render 側へ再適用する
4. `RefreshSettingsDialogState()` で UI 表示を更新する

そのため、起動時だけでなく実行中に `RenderSettings.light.csv` や `RenderSettings.night.csv` を読み込んで比較できる。

## `RenderSettings.csv` の役割

`Sample` フォルダーには複数の設定 CSV がある。

- `RenderSettings.csv`
- `RenderSettings.light.csv`
- `RenderSettings.night.csv`

これらはサンプルの描画条件を切り替えるためのプリセットである。
キーの一部は Render 本体に直接効き、一部は Sample 側の UI 状態やトグルに効く。

例えば次のような項目がある。

- Fog
- Height Fog
- SSAO
- Shadow
- Specular
- SSS
- Bloom
- Depth Of Field
- GodRay
- Motion Blur Camera
- Gaussian
- FXAA
- Saturate
- Camera / GBuffer の near/far
- モデル読込スケール

CSV の詳しいキー体系は `Render::ApplySettings()` と `LoadSampleSettingsFromCsv()` の両方を見る必要がある。
つまり、`RenderSettings.csv` は「Render 専用設定ファイル」ではなく、「Render と Sample をまたぐ統合プリセット」である。

## `.x` 隣接 `.csv` との関係

`RenderSettings.csv` とは別に、モデル個別の見た目上書き用 CSV も使える。

例:

- `cubePOM.blend.x` に対する `cubePOM.blend.csv`
- `monkeySSS.x` に対する `monkeySSS.csv`
- `sphereEnv.x` に対する `sphereEnv.csv`

こちらは `MeshMix` 系が読み込むもので、材質や見た目の性格を個別に上書きする。
全体設定を変える `RenderSettings.csv` と、モデル単位の見た目を変える隣接 CSV は役割が異なる。

詳しくは次を参照のこと。

- `README_about_x_csv.md`
- `README_about_sample_x.md`

## サンプルの内部構造

`Sample` は主に次のファイル群でできている。

### `main.cpp`

アプリ入口である。
ウィンドウ作成、Raw Input 登録、サンプルシーン初期化、メッセージループ、毎フレーム描画を担当する。

### `AppState.h`

サンプル全体で共有する状態の定義を持つ。

主な内容は次の通りである。

- グローバルな bool / float / color / vector
- 選択中ファイルパス
- 読み込み済みモデル一覧
- Apply 系関数宣言
- スライダー変換関数宣言
- モデル追加やダイアログ補助関数宣言

サンプルの設計は、状態を `AppState` に寄せ、それを入力処理、ダイアログ、描画補助から共有参照する形である。

### `AppState.cpp`

状態保持、CSV 読み込み、カメラ補助、モデル追加、オーバーレイ描画など土台寄りの処理を持つ。

特に重要なのは次の関数である。

- `LoadSampleSettingsFromCsv()`
- `ApplyAllSampleSettings()`
- `ReloadRenderSettingsFromCsv()`
- `RegisterLoadedModel()`
- `RemoveLoadedModel()`
- `SpawnMesh...AtLookAt()` 群
- `DrawSampleOverlay()`

### `AppStateRendering.cpp`

`AppState` にある値を `Render` 側へ反映する `Apply...()` 群を持つ。

役割は次の通りである。

- 値の clamp
- スライダー値と実値の変換
- `g_Render.Set...()` 呼び出し
- UI と描画設定の橋渡し

値をグローバル変数へ代入しただけでは描画は変わらず、必ず `Apply...()` を通す前提になっている。

### `InputHandlers.cpp`

キーボードショートカットと移動フラグ管理を担当する。
メインループのメッセージ処理から呼ばれ、操作体系を一か所へまとめている。

### `SettingsDialog.cpp`

設定ダイアログ本体である。

担当内容:

- ダイアログ生成
- `WM_COMMAND` / `WM_HSCROLL` / `WM_VSCROLL` 処理
- `Open...` ボタン処理
- 解像度コンボ、SSAO モードコンボ処理
- CSV 再読込ボタン処理

### `SettingsDialogRefresh.cpp`

ダイアログ上の各コントロール表示を現在状態へ同期する処理群である。
トグル状態、チェック状態、有効/無効、スライダー位置、文字表示などをまとめて更新する。

入力処理と描画反映を分けたうえで、UI 更新専用ファイルをさらに分離しているため、見通しを保ちやすい。

## 状態反映の考え方

`Sample` では状態変更を次の 3 段階に分けている。

1. 状態値を書き換える
2. `Apply...()` で `Render` へ反映する
3. `RefreshSettingsDialogState()` で UI 表示を同期する

例えば Bloom を切り替えるときは、概念的には次の流れになる。

1. `g_bBloom` を変更する
2. `ApplyPostEffectToggleSettings()` を呼ぶ
3. `RefreshSettingsDialogState()` を呼ぶ

この構造により、キーボード操作とダイアログ操作が別経路でも、最終的には同じ描画状態へ収束する。

## 毎フレームの流れ

メインループでは、メッセージ処理の合間に `TickAndRenderFrame()` が呼ばれる。
ここでは主に次を行う。

1. `DrawSampleOverlay()`
2. `UpdateDirectionalLight()`
3. `UpdateCameraMoveByKeyboard()`
4. 必要に応じて `ApplyGodRayLightPos()`
5. `g_Render.Draw()`

### `DrawSampleOverlay()`

操作説明、カメラ位置、GodRay ライト位置などを文字で描く。
さらに追加済み画像や追加済みテキストもここで 2D 描画する。

### `UpdateDirectionalLight()`

サンプル独自の方向光更新処理である。
時間経過やモードに応じてライト方向を動かす確認用途に使う。

### `UpdateCameraMoveByKeyboard()`

`WASD` などで立てた移動フラグを元に、毎フレームカメラを移動する。
Shift 押下で移動速度を上げる処理もここにある。

## マウスルックの扱い

`Esc` でマウスルックを切り替える。

- ON
  カーソルを隠す
  必要に応じて `ClipCursor()` でウィンドウへ閉じ込める
  Raw Input を使って相対移動で視点を回す
- OFF
  カーソルを戻す
  クリップを解除する

ただしリモートデスクトップ環境では、カーソル閉じ込めが操作性を悪くすることがある。
そのため `g_bRemoteDesktop` を見て挙動を少し変えている。

加えて、このフラグは起動時に自動判定される。
現在の実装では、平日の 9:00 以上 18:00 未満であればリモートデスクトップ向け設定が既定で ON になり、それ以外の時間帯では OFF になる。
必要なら設定ダイアログの `Remote Desktop` チェックボックスで後から手動変更できる。

## リソースパスの考え方

サンプルで使っているモデルや画像は、実行時カレントディレクトリ前提の相対パスが多い。

例:

- `L"RenderSettings.csv"`
- `L"..\\..\\Sample\\cubeNormalInverse.x"`
- `L"..\\..\\Sample\\res\\model\\wolf.x"`

このため、Visual Studio からの実行と、別の場所からの直接起動ではパス解決結果が変わる可能性がある。
サンプルを拡張するときは、まず実行時作業ディレクトリを意識する必要がある。

## サンプルを見るときの読み順

全体を追うなら、次の順で読むと理解しやすい。

1. `Sample\main.cpp`
2. `Sample\AppState.h`
3. `Sample\AppState.cpp`
4. `Sample\AppStateRendering.cpp`
5. `Sample\InputHandlers.cpp`
6. `Sample\SettingsDialog.cpp`
7. `Sample\SettingsDialogRefresh.cpp`

この順なら、「どこで状態を持ち、どこで入力を受け、どこで描画へ反映し、どこで UI 表示を更新するか」がつながって見える。

## 実運用上の注意

- `Sample` は確認用アプリであり、設計は厳密な MVC ではない
- グローバル状態が多いため、値変更時は `Apply...()` 呼び出し忘れに注意が必要である
- `RenderSettings.csv` は Render と Sample の双方に効くため、片方だけ見て編集すると意図を誤りやすい
- `MeshMix` 系モデルは隣接 `.csv` でも見た目が変わるため、モデル差分確認時は `.x` だけでなく `.csv` も一緒に見るべきである
- 設定ダイアログの数値とショートカット操作は相互に影響するため、実装変更時は UI 同期を確認した方がよい

## どんなときにこのサンプルを使うべきか

次のような用途では、まず `Sample` で確認するのが有効である。

- 新しいポストエフェクトを追加したとき
- `RenderSettings.csv` のキーを増やしたとき
- `MeshMix` 系の材質解釈を変えたとき
- カメラ、Fog、Shadow、SSAO の見た目を調整したいとき
- 解像度変更やウィンドウモード変更の挙動を見たいとき
- 2D 表示と 3D 表示の重なり順を確認したいとき

`Sample` はライブラリの「動く仕様書」である。
新機能追加時や既存機能変更時は、このサンプルで再現手順を作れる状態を保つのが望ましい。
