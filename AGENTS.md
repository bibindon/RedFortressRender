# AGENTS.md

## 回答言語

- 日本語で回答すること。

## Build

```
# Fix duplicate Path/PATH env var (MSB6001 if both exist)
$env:PATH = $env:Path; Remove-Item Env:\PATH -ErrorAction SilentlyContinue

# Build x64 only
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" Render.sln /p:Configuration=Release /p:Platform=x64 /m
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" Render.sln /p:Configuration=Debug /p:Platform=x64 /m
```

## File encoding / line endings

| Extension | Encoding | BOM |
|-----------|----------|-----|
| `.fx`     | UTF-8    | No  |
| `.X`      | UTF-8    | No  |
| All other source (`.cpp`, `.h`, `.rc`, `.vcxproj`, etc.) | UTF-8 | Yes |

All files: CRLF line endings. Do not introduce LF-only files.

## Code conventions

- Namespace: `NSRender`. All library classes live inside it.
- DirectX 9. `d3d9.lib` / `d3dx9.lib` (debug: `d3dx9d.lib`).
- Y-axis is **up** (not depth). Z-axis points toward viewer.
- 1 world unit = 1 meter.
- No ternary operator (`?:`). Always use `if`/`else`.
- Use `SAFE_RELEASE`, `SAFE_DELETE`, `SAFE_DELETE_ARRAY` from `Common.h` for resource cleanup.
- Shader model 2.0, Effect type (not inline HLSL).
- x64 configs use `ConformanceMode=true`, `MultiProcessorCompilation=true`.

## Project structure

| Project | Type | Role |
|---------|------|------|
| `Render` | Static library | Core rendering engine: meshes, GBuffer, post effects, fonts, sprites |
| `Sample` | Win32 `.exe` | Test app consuming `Render`, with settings dialog |
| `UnitTest1` | Native test DLL (CppUnitTestFramework) | Tests for `Render` |
| `GrassLandSample` | Not in solution | Experimental, ignore |

### Entry points
- `Render\Render.cpp` — `NSRender::Render` class, the main public API
- `Render\Render.h` — public header (includes everything needed)
- `Sample\main.cpp` — Win32 message loop, calls `g_Render.Initialize()`, `g_Render.Draw()`, `g_Render.Finalize()`

## Shader pipeline

- **Canonical source**: `Render\shader\*.fx`
- `Common.fx` is `ExcludedFromBuild`; it is `#include`d by other shaders.
- Building produces `.cso` files in the output directory.
- Sample's pre-build event copies `Render\shader\*.fx` to `Sample\res\shader`.
- Sample also compiles shaders from `..\Render\shader\` (same sources, double-compile).
- `.cso` files are gitignored; they must be regenerated on build.

## Testing

- Framework: `Microsoft::VisualStudio::CppUnitTestFramework`
- Tests require a **D3D9 device**; each test creates a hidden window + device.
- Tests search upward from CWD for compiled `.cso` shader files — run from a directory where `x64\Debug\` or `Render\x64\Debug\` is reachable.
- Run tests with Visual Studio Test Explorer or `vstest.console.exe` pointing at the built `UnitTest1.dll`.
- Test project references `Render` as a project reference, not a binary dependency.

## Settings CSV

- `Sample\RenderSettings.csv` is read by both Sample (window state) and Render (render parameters).
- Settings keys consumed by `Render::ApplySettings()` must stay in sync.
- Sample has variants: `RenderSettings.light.csv`, `RenderSettings.night.csv`.
- The demo's intended startup Lambert Darkness is `HalfLambertShadowDarkness,0.7` in `Sample\RenderSettings.csv`; changing only the dialog default does not control startup behavior.

## Blender 5.1.2 / DirectX `.x` export contract

These rules are the result of fixing the stage-select depth reversal. Do not compensate for an export-axis problem by changing the camera or adding ad-hoc rotations to CSV files.

- Use Blender **5.1.2** and its DirectX `.x` exporter.
- Always set the exporter axes explicitly: **Forward = `Z`**, **Up = `Y`**, **Scale = `1.0`**, text `.x` format. Do not rely on settings left in the exporter UI from a previous export.
- Export meshes with modifiers, normals, UVs, materials, and textures enabled. Keep the existing armature/weights/animation options when the source asset uses them. The stage assets were exported with triangulation and unweld enabled.
- Blender `(X, Y, Z)` is represented by exported `.x` data as `(X, Z, -Y)`. `MeshMix2::CorrectBlenderOfficialAxisTransforms()` converts it to this renderer's `(X, Z, Y)` convention. It corrects both the frame basis and the frame translation (`_43`). Do not add another axis conversion in the caller or CSV.
- Never mix assets exported with `Forward = Z` and `Forward = -Z`. The old stage-select 1, 2, and 4 files used `-Z`; objects baked at the origin could appear correct while objects positioned by frame transforms appeared depth-reversed. This was the cause of the misleading "only some objects are reversed" symptom.
- `.x`/`.X` output must remain UTF-8 without BOM and use CRLF. Texture paths must be relative and every referenced texture must exist. Keep textures with the model folder where practical.
- For the demo floor and ceiling, preserve the Blender source in each model folder, keep each folder's textures local, and export material specular power as `500`. The ceiling must remain a textured six-face box, not a single plane.

### Stage-select export boundaries

Stage source files live under `..\RedFortress2\MultiPassRendering\res\model\stage-select*`. Preserve the existing exported-file split; exporting every mesh into every output changes scene contents even when the axes are correct.

- Stage 1, `world1.blend`: `stageSelectSea.x` contains only `StageSelect_DeepSea.001`, `StageSelect_ShallowWaterRing.001`, `RF1_Portal_00_Ring`, and `RF1_Portal_09_Ring`. All other mesh objects go to `stageSelectIsland.x`.
- Stage 2, `world2.blend`: export all mesh objects to `stageSelectCave.x`.
- Stage 3, `world3.blend`: export only `RF3_StarField` to `stageSelectMoonMountainStars.x`; export all other mesh objects to `stageSelectMoonMountain.x`.
- Stage 4, `world4.blend`: export all mesh objects to `stageSelectDawnIsland.x`.
- Keep world-scene placement at the authored coordinates. A correctly exported world model normally uses zero position/rotation and scale `1` in its placement CSV. Do not use `RotY = 180` to hide a bad export.

### MeshMix2 loading and camera-facing placement

- Blender 5.1.2 static `.x` world models in placement CSV must use `loadType=meshmix2`. `normal` selects the older `MeshMixManager` path and has different transform behavior.
- The render settings dialog must also preserve `meshmix2`: CSV loading and the dialog's direct model-open action call `AddMeshMix2`, and CSV IDs are registered with `RegisterCsvMeshMix2IdMapping`.
- CSV-authored rotations are authoritative and are loaded unchanged.
- Only interactive "place at look-at point and face the camera" operations apply a facing correction. For MeshMix2 this is `atan2f(towardCamera.x, towardCamera.z) + D3DX_PI` in both `Sample\AppState.cpp::SpawnMeshMix2AtLookAt()` and the direct-open handler in `Render\RenderSettingsDialogHandlers.cpp`. Keep these two call sites consistent.
- Do not duplicate that 180-degree facing correction in the Blender model, the CSV, or `MeshMix2`; doing so makes either M-key placement or settings-dialog placement face backward again.

### Regression checks after re-export or transform changes

- Compare exported frame/object names with the previous `.x` file, especially for split stage files.
- Check several non-origin frame translations, not only geometry baked at the origin.
- Verify all referenced textures exist and material power values remain intact.
- Test both a stage loaded from CSV and `monkey.blend.x` placed interactively with the M key and through the settings dialog. The monkey must face the camera in both paths.
- For the RedFortress2 game, a link-time `LNK1201` on `RedFortress2\x64\Release\simple-directx9.pdb` can be caused by a running game or Visual Studio holding the PDB. Treat that as a file-lock problem, not an axis-conversion source failure.

## Known gotchas

- `MeshMixSkinAnim` / `MeshMixSkinAnim2` validate CSV-listed animation `.x` files at load: every frame in the animation hierarchy must exist in the mesh frame hierarchy. On mismatch they throw `std::runtime_error` (details in `CustomXLoader.log`), which is fatal to the host app. When bones are removed from a model, its animation `.x` files must be reduced to the same bone set (see `RedFortress2/.../marine_512_low`, kept in sync at 28 bones).
- `AddMeshInstansing` is intentionally misspelled (missing 't' from "Instancing").
- Resource paths are relative to the **runtime working directory**, not the project directory.
- Device loss handling: classes implement `IDeviceResettable::OnDeviceLost` / `OnDeviceReset`. `Common` tracks all registered resources.
- NuGet restore: the solution needs `Microsoft.DXSDK.D3DX.9.29.952.8` in `packages/`. Run `nuget restore` or let VS restore on first build.
