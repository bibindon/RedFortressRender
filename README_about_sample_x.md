# Sample フォルダーの `.x` / `.csv` 一覧

`Sample` フォルダーには、描画機能の確認用モデルと設定例が入っている。
ここでは「何の確認に使うか」をざっくり整理する。

## 起動時に読み込まれるもの

- `cubeNormalInverse.x`
  Sample 起動直後に配置されるキューブである
- `plateField.x`
  地面として使うプレートである

## 代表的な確認用モデル

| ファイル | 用途 |
|---|---|
| `cube.x` | 基本表示確認用 |
| `cubeNormal.x` + `cubeNormal.csv` | Normal Mapping の確認 |
| `cubePOM.blend.x` + `cubePOM.blend.csv` | POM の確認 |
| `cubeGlass.x` + `cubeGlass.csv` | ガラス表現の確認 |
| `cubeMixSun.blend.x` | 太陽光を受けるモデル確認 |
| `cubeNormalInverse.x` | 法線や陰影の見え方確認 |
| `enemyOrangeCube.x` | キャラクターっぽいメッシュの確認 |
| `monkey.blend.x` | 基本メッシュ確認 |
| `monkeySSS.x` + `monkeySSS.csv` | SSS の確認 |
| `monkeyGlass.x` + `monkeyGlass.csv` | ガラス表現の確認 |
| `sphereEnv.x` + `sphereEnv.csv` | 環境マッピング確認 |
| `sphereEnv1024.x` + `sphereEnv1024.csv` | 高解像度環境マップ確認 |
| `sphereGlass.x` + `sphereGlass.csv` | 球体ガラス表現確認 |
| `sun.x` + `sun.csv` | 光源マーカーや発光表現の確認 |
| `res\model\wolf.x` | 別系統モデル読み込み確認 |

## RenderSettings 系

| ファイル | 用途 |
|---|---|
| `RenderSettings.csv` | 標準設定 |
| `RenderSettings.light.csv` | 明るめ設定例 |
| `RenderSettings.night.csv` | 夜寄り設定例 |

## 使い方の目安

- モデルだけ読みたいなら `.x` だけでよい
- `MeshMix` 系の見た目も一緒に再現したいなら、同名の `.csv` も同じ場所に置く
- Sample の設定一式を切り替えたいなら `RenderSettings*.csv` を使う

## 補足

サンプル追加時は、最低でも次をそろえておくと管理しやすい。

- `.x`
- 必要なら同名 `.csv`
- この README への 1 行説明
