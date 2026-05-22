# RenderSettings.csv について

`Sample\RenderSettings.csv` は、起動時に次の 2 か所から読まれる。

- `Sample::LoadSampleSettingsFromCsv()`
  サンプルアプリの UI 状態や初期値を復元する。
- `NSRender::Render::LoadSettingsCsv()` / `ApplySettings()`
  描画ライブラリ本体の初期設定として使う。

つまりこのファイルは、「サンプルの見た目復元」と「Render の初期化設定」の兼用である。

## 書式

1 行につき `キー,値` である。

```csv
SSAOEnable,1
FogIntensity,1.5
BloomThreshold,2.8
```

ルール:

- 最初の `,` より左がキー、右が値である
- `#` 以降はコメントとして無視される
- 同じキーが複数回出た場合は、後に書かれた値が優先される
- 空キー、空値は無視される

## bool の書き方

Render 側では次の値を bool として解釈する。

- `true` 扱い: `1`, `true`, `on`, `yes`
- `false` 扱い: `0`, `false`, `off`, `no`

Sample 側の一部キーは `std::stoi(value) != 0` で判定しているため、互換性を優先するなら `0` / `1` を使うのが安全である。

## 主なキー

### カメラと GBuffer

| キー | 内容 |
|---|---|
| `CameraNear` | カメラ Near |
| `CameraFar` | カメラ Far |
| `GBufferNear` | GBuffer Near |
| `GBufferFar` | GBuffer Far |

### Saturate / Gaussian / FXAA / Motion Blur

| キー | 内容 |
|---|---|
| `SaturateEnable` | 彩度フィルター ON/OFF |
| `SaturateLevel` | 彩度係数 |
| `GaussianEnable` | Gaussian ON/OFF |
| `GaussianSampleSize` | Gaussian サンプル数。1 から 101 の奇数に補正される |
| `MaskedGaussianEnable` | Masked Gaussian ON/OFF |
| `MaskedGaussianMaskPath` | マスク画像パス |
| `FXAAEnable` | FXAA ON/OFF |
| `FXAAQuality` | FXAA 品質。1 から 8 に補正される |
| `MotionBlurCameraEnable` | カメラモーションブラー ON/OFF |
| `MotionBlurCameraQuality` | 品質。1 から 8 に補正される |
| `MotionBlurCameraMaxBlurPixels` | 最大ブラー量。1 から 64 に補正される |
| `MotionBlurCameraSampleCount` | サンプル数。2 から 21 に補正される |
| `RenderQuality` | `LOW` / `MIDDLE` / `HIGH`。それ以外は `LOW` |

### SSS

| キー | 内容 |
|---|---|
| `SSSEnable` | MeshMix 系 SSS ON/OFF |
| `SSSIntensity` | SSS の強さ |
| `SSSColorR` | SSS 色 R |
| `SSSColorG` | SSS 色 G |
| `SSSColorB` | SSS 色 B |

### Fog / Height Fog

| キー | 内容 |
|---|---|
| `FogEnable` | Fog ON/OFF |
| `FogIntensity` | 距離 Fog の強さ |
| `FogHeightEnable` | Height Fog ON/OFF |
| `FogHeightIntensity` | Height Fog の強さ |
| `FogHeightStart` | Fog 開始高さ |
| `FogHeightMax` | Fog の上限高さ |
| `FogHeightDistanceStart` | 距離ベースの開始位置 |
| `FogHeightDistanceMax` | 距離ベースの最大範囲 |

### Shadow / SSAO

| キー | 内容 |
|---|---|
| `DepthBufferShadowEnable` | Depth Buffer Shadow ON/OFF |
| `ShadowIntensity` | Shadow の強さ |
| `ShadowCoverage` | Shadow の被覆率 |
| `ShadowSaturationBoost` | Shadow 部分彩度補正 |
| `ShadowPcfTapCount` | PCF サンプル数 |
| `ShadowCompositeTapCount` | 合成サンプル数 |
| `ShadowBlurTapCount` | Sample 側では PCF / Composite の両方に同値を入れる簡略キー |
| `SSAOEnable` | SSAO ON/OFF |
| `SSAOBlurEnable` | SSAO のブラー ON/OFF |
| `SSAOSeparableBlurEnable` | SSAO のブラーを横/縦の 2 パスへ分離 |
| `SSAODepthScaledSampleDistanceEnable` | SSAO の深度連動距離 ON/OFF |
| `SSAOSampleRadius` | SSAO のサンプル半径 |
| `SSAOShadowStrength` | SSAO の影の強さ |
| `SSAOShadowSaturationBoost` | SSAO の彩度補正 |
| `SSAOSampleCount` | SSAO サンプル数 |
| `SSGIEnable` | SSGI ON/OFF |
| `SSGIBlurEnable` | SSGI のブラー ON/OFF |
| `SSGISeparableBlurEnable` | SSGI のブラーを横/縦の 2 パスへ分離 |
| `SSGIDepthScaledSampleDistanceEnable` | SSGI の深度連動距離 ON/OFF |
| `SSGIUseThickness` | SSGI で厚みテクスチャを使うか |
| `SSGITexSize` | SSGI バッファ解像度。`1/1`, `1/2`, `1/4` |
| `SSGISampleCount` | SSGI サンプル数 |
| `SSGISampleRadius` | SSGI のサンプル半径 |
| `SSGIBlurKernelSize` | SSGI ブラーカーネルサイズ |
| `SSGIIndirectLightStrength` | SSGI の間接光強度 |
| `SSGIIndirectLightMaxContribution` | SSGI の最大寄与量 |

### MeshMix の陰影

| キー | 内容 |
|---|---|
| `HalfLambertShadowSaturation` | 半ランバート影の彩度係数 |
| `ShadowDarkness` | 影の暗さ |
| `SpecularIntensity` | スペキュラ強度 |
| `SpecularIntensityOverride` | スペキュラ強度上書き ON/OFF |
| `SpecularEdge` | スペキュラの鋭さ |
| `SpecularEdgeOverride` | スペキュラエッジ上書き ON/OFF |

### Bloom / DOF / StarBurst / GodRay

| キー | 内容 |
|---|---|
| `BloomEnable` | Bloom ON/OFF |
| `BloomThreshold` | Bloom 閾値 |
| `HaloEnable` | Halo ON/OFF |
| `DepthOfFieldEnable` | DOF の旧形式 ON/OFF |
| `DepthOfFieldMode` | `0=OFF`, `1=ON`, `2 以上=AutoNear` |
| `DepthOfFieldFocalDistance` | 焦点距離 |
| `DepthOfFieldMaxBlurDistance` | 最大ぼかし距離 |
| `DepthOfFieldAutoActivationDistance` | AutoNear 切替距離 |
| `StarBurstEnable` | StarBurst ON/OFF |
| `StarBurstThreshold` | StarBurst 閾値 |
| `GodRayEnable` | GodRay ON/OFF |
| `GodRayLightColorR` | 光色 R |
| `GodRayLightColorG` | 光色 G |
| `GodRayLightColorB` | 光色 B |
| `GodRayIntensity` | 光芒強度 |
| `GodRayVirtualProximityStrength` | 擬似接近効果の強さ |
| `GodRayLightPosX` | 光源位置 X |
| `GodRayLightPosY` | 光源位置 Y |
| `GodRayLightPosZ` | 光源位置 Z |

### サンプルアプリ側の光源・補助設定

| キー | 内容 |
|---|---|
| `ModelLoadScale` | ダイアログから読み込むモデルの既定スケール |
| `SunLightIntensity` | 平行光源の強さ |
| `SunLightColorR` | 平行光源色 R |
| `SunLightColorG` | 平行光源色 G |
| `SunLightColorB` | 平行光源色 B |
| `AmbientLightIntensity` | 環境光強度 |
| `AmbientLightColorR` | 環境光色 R |
| `AmbientLightColorG` | 環境光色 G |
| `AmbientLightColorB` | 環境光色 B |

## サンプル

```csv
SaturateEnable,1
SaturateLevel,0.4

GaussianEnable,0
GaussianSampleSize,101
MaskedGaussianEnable,0

FXAAEnable,0
FXAAQuality,4

MotionBlurCameraEnable,0
MotionBlurCameraQuality,4
MotionBlurCameraMaxBlurPixels,24
MotionBlurCameraSampleCount,13

SSAOEnable,1
SSAOBlurEnable,1
SSAOSeparableBlurEnable,0
SSAODepthScaledSampleDistanceEnable,0
SSAOSampleRadius,4.0
SSAOShadowStrength,1.0
SSAOShadowSaturationBoost,0.30
SSAOSampleCount,16
SSGIEnable,1
SSGIBlurEnable,1
SSGISeparableBlurEnable,1
SSGIDepthScaledSampleDistanceEnable,0
SSGIUseThickness,1
SSGISampleCount,16
SSGISampleRadius,1.0
SSGIBlurKernelSize,21
SSGIIndirectLightStrength,1.0
SSGIIndirectLightMaxContribution,1.0

FogEnable,1
FogIntensity,1.0
FogHeightEnable,1
FogHeightIntensity,0.3

BloomEnable,0
BloomThreshold,2.5

DepthOfFieldMode,0
DepthOfFieldFocalDistance,1.0

StarBurstEnable,0
StarBurstThreshold,2.8
```

## 補足

- `Sample\RenderSettings.csv`
  現在の標準設定である
- `Sample\RenderSettings.light.csv`
  明るめの調整例である
- `Sample\RenderSettings.night.csv`
  夜寄りの調整例である

キーを増やしたら、`Render::ApplySettings()` と `Sample::LoadSampleSettingsFromCsv()` の両方を確認して README も更新すること。
