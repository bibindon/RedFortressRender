# AGENTS.md

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

## Known gotchas

- `AddMeshInstansing` is intentionally misspelled (missing 't' from "Instancing").
- Resource paths are relative to the **runtime working directory**, not the project directory.
- Device loss handling: classes implement `IDeviceResettable::OnDeviceLost` / `OnDeviceReset`. `Common` tracks all registered resources.
- NuGet restore: the solution needs `Microsoft.DXSDK.D3DX.9.29.952.8` in `packages/`. Run `nuget restore` or let VS restore on first build.
