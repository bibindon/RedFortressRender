# `.x` と同名の `.csv` について

`MeshMix` 系で `.x` ファイルを読むとき、同じ場所に同名の `.csv` があれば追加設定として読み込む。

例:

- `Sample\sphereEnv.x`
- `Sample\sphereEnv.csv`

この CSV は `Render\MeshMixManager.cpp` で読む。
キー名と値は小文字化して比較されるため、実質的に大文字小文字は区別されない。

## 書式

1 行につき `キー,値` である。

```csv
meshtype,envmapping
smooth,y
shadow,n
```

bool 系は次の値が使える。

- `true` 扱い: `y`, `yes`, `true`, `1`
- `false` 扱い: `n`, `no`, `false`, `0`

## キー一覧

### meshtype

メッシュの描画系統を切り替える。

| 値 | 内容 |
|---|---|
| `pom` | Parallax Occlusion Mapping |
| `normalmapping` | Normal Mapping |
| `envmapping` | 環境マッピング |
| `glass` | ガラス表現 |
| `emit` | 発光表現 |

未指定時は通常の `MeshMix` として扱われる。

### smooth

スムーズ法線を使うかどうかを指定する。

| 値 | 内容 |
|---|---|
| `y` | 有効 |
| `n` | 無効 |

### sss / sssintensity / ssscolor

SSS 風表現の設定である。

```csv
sss,y
sssintensity,32.0
ssscolor,255,255,255
```

- `ssscolor` は `R,G,B` の 0 から 255 である

### sway / swayintensity

草木のような左右の揺れ表現である。

```csv
sway,y
swayintensity,0.5
```

### wave / waveintensity

波打ち表現である。

```csv
wave,y
waveintensity,0.2
```

### litbypointlight

ポイントライトの影響を受けるかどうかを指定する。

| 値 | 内容 |
|---|---|
| `y` | 受ける |
| `n` | 受けない |

### shadow / zshadow

Depth Buffer Shadow の対象にするかどうかを指定する。
`shadow` と `zshadow` は同じ意味である。

| 値 | 内容 |
|---|---|
| `y` | 対象にする |
| `n` | 対象から外す |

### lambertshadow

半ランバート影を使うかどうかを指定する。

| 値 | 内容 |
|---|---|
| `y` | 有効 |
| `n` | 無効 |

### ssao

SSAO の対象にするかどうかを指定する。

| 値 | 内容 |
|---|---|
| `y` | 対象にする |
| `n` | 対象から外す |

### collision

当たり判定用として使うかどうかを指定する。

| 値 | 内容 |
|---|---|
| `y` | 有効 |
| `n` | 無効 |

### cubemappingrate / cubemappinggauss

環境マッピングやガラス表現で使う係数である。
どちらも 0.0 から 1.0 にクランプされる。

```csv
cubemappingrate,1.0
cubemappinggauss,0.2
```

- `cubemappingrate`
  反射の乗り方を指定する
- `cubemappinggauss`
  反射のぼかし量を指定する

## 実例

### 環境マッピング

```csv
meshtype,envmapping
smooth,n
cubemappingrate,1.0
cubemappinggauss,0.0
ssao,n
shadow,n
```

### ガラス

```csv
meshtype,glass
smooth,n
cubemappingrate,1.0
cubemappinggauss,0.0
```

### SSS

```csv
meshtype,none
smooth,y
sss,y
sssintensity,32.0
ssscolor,255,255,255
```

## 補足

- `.csv` がなくても `.x` は読み込める
- `.csv` は `MeshMix` 系の追加設定なので、通常メッシュ系 API では使われない
- キーを増やしたら `Render\MeshMixManager.cpp` とこの README を一緒に更新すること
