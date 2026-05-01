# RenderSettings.csv について

描画ライブラリの初期化時に読み込まれる設定ファイルです。
ポストエフェクトなどの描画設定をコードを変更せずに調整できます。

---

## フォーマット

各行に `キー,値` の形式で記述します。`#` 以降はコメントとして無視されます。

```
SSAOEnable,1        # SSAOを有効にする
FogIntensity,2.0    # 霧の強さ
```

---

## パラメータ一覧

### ポストエフェクト ON/OFF

値は `1` / `0`、`true` / `false`、`on` / `off`、`yes` / `no` で指定します。

| キー | 説明 | デフォルト |
|---|---|---|
| `DepthBufferShadowEnable` | 深度バッファシャドウを有効にするか | `1` |
| `SSAOEnable` | SSAO（環境遮蔽）を有効にするか | `1` |
| `FogEnable` | 霧エフェクトを有効にするか | `1` |
| `SaturateEnable` | 彩度フィルターを有効にするか | `0` |
| `GaussianEnable` | ガウスフィルター（ぼかし）を有効にするか | `0` |
| `BloomEnable` | ブルームを有効にするか | `0` |
| `StarBurstEnable` | スターバーストを有効にするか | `0` |
| `DepthOfFieldEnable` | 被写界深度を有効にするか | `0` |
| `litByPointLight` | ポイントライトによる照明を有効にするか | `0` |
| `collision` | 衝突判定を有効にするか | `0` |

### 数値パラメータ

| キー | 説明 | デフォルト |
|---|---|---|
| `GaussianSampleSize` | ガウスフィルターのサンプル数（奇数、1〜101） | `101` |
| `FogIntensity` | 霧の強さ | `2.0` |
| `ShadowIntensity` | 影の濃さ | `0.5` |
| `SSAOBrightness` | SSAOの明るさ | `1.0` |
| `ShadowSaturationBoost` | 影部分の彩度ブースト量 | `0.35` |
| `SSAOSaturationBoost` | SSAO部分の彩度ブースト量 | `0.30` |
| `BloomThreshold` | ブルームが発生する輝度の閾値 | `2.5` |
| `StarBurstThreshold` | スターバーストが発生する輝度の閾値 | `2.8` |
| `DepthOfFieldFocalDistance` | 被写界深度のピントが合う距離 | `8.0` |

---

## 記述例

```
DepthBufferShadowEnable,1
SSAOEnable,1
FogEnable,1
SaturateEnable,0
GaussianEnable,0
GaussianSampleSize,101
FogIntensity,2.0
ShadowIntensity,0.25
SSAOBrightness,1.0
BloomThreshold,2.5
StarBurstThreshold,2.8
BloomEnable,0
StarBurstEnable,0
litByPointLight,y
collision,y
