# Diff Details

Date : 2026-04-29 21:53:48

Directory c:\\Users\\bibindon\\source\\repos\\bibindon\\RedfortressRender

Total : 89 files,  2483 codes, 87 comments, 484 blanks, all 3054 lines

[Summary](results.md) / [Details](details.md) / [Diff Summary](diff.md) / Diff Details

## Files
| filename | language | code | comment | blank | total |
| :--- | :--- | ---: | ---: | ---: | ---: |
| [GrassLandSample/GrassLandSample.vcxproj](/GrassLandSample/GrassLandSample.vcxproj) | XML | 1 | 0 | -6 | -5 |
| [README.md](/README.md) | Markdown | 20 | 0 | 16 | 36 |
| [Render/Camera.cpp](/Render/Camera.cpp) | C++ | 15 | 0 | 7 | 22 |
| [Render/Common.cpp](/Render/Common.cpp) | C++ | -48 | 0 | -6 | -54 |
| [Render/Common.h](/Render/Common.h) | C++ | -10 | 7 | 5 | 2 |
| [Render/Font.cpp](/Render/Font.cpp) | C++ | 17 | 3 | 10 | 30 |
| [Render/Font.h](/Render/Font.h) | C++ | 4 | 0 | 3 | 7 |
| [Render/GBuffer.cpp](/Render/GBuffer.cpp) | C++ | 139 | 12 | 35 | 186 |
| [Render/GBuffer.h](/Render/GBuffer.h) | C++ | 25 | 1 | 15 | 41 |
| [Render/Light.cpp](/Render/Light.cpp) | C++ | 1 | 0 | 0 | 1 |
| [Render/Light.h](/Render/Light.h) | C++ | 1 | 1 | 0 | 2 |
| [Render/Mesh.cpp](/Render/Mesh.cpp) | C++ | -247 | -166 | -78 | -491 |
| [Render/Mesh.h](/Render/Mesh.h) | C++ | -54 | -8 | -20 | -82 |
| [Render/MeshHierarchy.h](/Render/MeshHierarchy.h) | C++ | 1 | 0 | 1 | 2 |
| [Render/MeshInstancing.cpp](/Render/MeshInstancing.cpp) | C++ | -1 | 0 | -4 | -5 |
| [Render/MeshMix.cpp](/Render/MeshMix.cpp) | C++ | 45 | -4 | 7 | 48 |
| [Render/MeshMix.h](/Render/MeshMix.h) | C++ | 5 | -1 | 1 | 5 |
| [Render/MeshMixManager.cpp](/Render/MeshMixManager.cpp) | C++ | 1 | 0 | 1 | 2 |
| [Render/MeshMixManager.h](/Render/MeshMixManager.h) | C++ | 9 | 4 | 10 | 23 |
| [Render/MeshNormalMapping.cpp](/Render/MeshNormalMapping.cpp) | C++ | 1 | 0 | 1 | 2 |
| [Render/MeshNormalMapping.h](/Render/MeshNormalMapping.h) | C++ | 0 | 1 | 0 | 1 |
| [Render/MeshOld.cpp](/Render/MeshOld.cpp) | C++ | 263 | 166 | 81 | 510 |
| [Render/MeshOld.h](/Render/MeshOld.h) | C++ | 58 | 9 | 21 | 88 |
| [Render/MeshPOM.cpp](/Render/MeshPOM.cpp) | C++ | 1 | 0 | 2 | 3 |
| [Render/MeshPOM.h](/Render/MeshPOM.h) | C++ | 0 | 2 | 0 | 2 |
| [Render/MeshPointLight.cpp](/Render/MeshPointLight.cpp) | C++ | 1 | 0 | 1 | 2 |
| [Render/MeshPointLight.h](/Render/MeshPointLight.h) | C++ | 0 | 3 | 0 | 3 |
| [Render/MeshSSS.cpp](/Render/MeshSSS.cpp) | C++ | 3 | 0 | 1 | 4 |
| [Render/MeshSSS.h](/Render/MeshSSS.h) | C++ | 1 | 1 | 1 | 3 |
| [Render/MeshSSSLike.cpp](/Render/MeshSSSLike.cpp) | C++ | 1 | 0 | 0 | 1 |
| [Render/MeshSmooth.cpp](/Render/MeshSmooth.cpp) | C++ | 1 | 0 | 0 | 1 |
| [Render/MeshSmooth.h](/Render/MeshSmooth.h) | C++ | 0 | 2 | 0 | 2 |
| [Render/PostEffectBloom.cpp](/Render/PostEffectBloom.cpp) | C++ | 12 | 0 | 2 | 14 |
| [Render/PostEffectBloom.h](/Render/PostEffectBloom.h) | C++ | 1 | 0 | 1 | 2 |
| [Render/PostEffectEnd.cpp](/Render/PostEffectEnd.cpp) | C++ | 1 | 0 | 0 | 1 |
| [Render/PostEffectFog.cpp](/Render/PostEffectFog.cpp) | C++ | 160 | 8 | 43 | 211 |
| [Render/PostEffectFog.h](/Render/PostEffectFog.h) | C++ | 41 | 0 | 24 | 65 |
| [Render/PostEffectGauss.cpp](/Render/PostEffectGauss.cpp) | C++ | 14 | 0 | 2 | 16 |
| [Render/PostEffectGauss.h](/Render/PostEffectGauss.h) | C++ | 2 | 0 | 1 | 3 |
| [Render/PostEffectSSAO.cpp](/Render/PostEffectSSAO.cpp) | C++ | 35 | 1 | 5 | 41 |
| [Render/PostEffectSSAO.h](/Render/PostEffectSSAO.h) | C++ | 4 | 1 | 0 | 5 |
| [Render/PostEffectSaturate.cpp](/Render/PostEffectSaturate.cpp) | C++ | 9 | 0 | 3 | 12 |
| [Render/PostEffectSaturate.h](/Render/PostEffectSaturate.h) | C++ | 1 | 0 | 1 | 2 |
| [Render/PostEffectStarBurst.cpp](/Render/PostEffectStarBurst.cpp) | C++ | 14 | 0 | 3 | 17 |
| [Render/PostEffectStarBurst.h](/Render/PostEffectStarBurst.h) | C++ | 1 | 0 | 1 | 2 |
| [Render/PostEffectZShadow.cpp](/Render/PostEffectZShadow.cpp) | C++ | 44 | 0 | 5 | 49 |
| [Render/PostEffectZShadow.h](/Render/PostEffectZShadow.h) | C++ | 4 | 0 | 0 | 4 |
| [Render/PtrManager.h](/Render/PtrManager.h) | C++ | 0 | 2 | 0 | 2 |
| [Render/Render.cpp](/Render/Render.cpp) | C++ | 191 | -5 | 25 | 211 |
| [Render/Render.h](/Render/Render.h) | C++ | 27 | 8 | 6 | 41 |
| [Render/Render.vcxproj](/Render/Render.vcxproj) | XML | 8 | 0 | 0 | 8 |
| [Render/SkinAnimMesh.cpp](/Render/SkinAnimMesh.cpp) | C++ | 0 | -1 | 1 | 0 |
| [Render/SkinAnimMesh.h](/Render/SkinAnimMesh.h) | C++ | 1 | 0 | 2 | 3 |
| [Render/SkinAnimMeshAlloc.cpp](/Render/SkinAnimMeshAlloc.cpp) | C++ | 0 | -1 | 0 | -1 |
| [Render/Sprite.cpp](/Render/Sprite.cpp) | C++ | 6 | 3 | 4 | 13 |
| [Render/Sprite.h](/Render/Sprite.h) | C++ | 0 | -2 | 0 | -2 |
| [Render/Util.cpp](/Render/Util.cpp) | C++ | 12 | 0 | 3 | 15 |
| [Render/Util.h](/Render/Util.h) | C++ | 27 | 2 | 2 | 31 |
| [Render/WindowManager.cpp](/Render/WindowManager.cpp) | C++ | 33 | 0 | 14 | 47 |
| [Render/shader/GBuffer.fx](/Render/shader/GBuffer.fx) | HLSL | 7 | -4 | 3 | 6 |
| [Render/shader/MeshMix.fx](/Render/shader/MeshMix.fx) | HLSL | -24 | -3 | -9 | -36 |
| [Render/shader/MeshNoLighting.fx](/Render/shader/MeshNoLighting.fx) | HLSL | 33 | 0 | 5 | 38 |
| [Render/shader/MeshOld.fx](/Render/shader/MeshOld.fx) | HLSL | 105 | 59 | 42 | 206 |
| [Render/shader/MeshShader.fx](/Render/shader/MeshShader.fx) | HLSL | -105 | -59 | -42 | -206 |
| [Render/shader/MeshSmooth.fx](/Render/shader/MeshSmooth.fx) | HLSL | 0 | 3 | 0 | 3 |
| [Render/shader/PostEffectFog.fx](/Render/shader/PostEffectFog.fx) | HLSL | 77 | 12 | 19 | 108 |
| [Render/shader/PostEffectSSAO.fx](/Render/shader/PostEffectSSAO.fx) | HLSL | 25 | -1 | 3 | 27 |
| [Render/shader/PostEffectSaturate.fx](/Render/shader/PostEffectSaturate.fx) | HLSL | -1 | 0 | -1 | -2 |
| [Render/shader/PostEffectZShadow.fx](/Render/shader/PostEffectZShadow.fx) | HLSL | 11 | 0 | 1 | 12 |
| [Render/shader/simple.fx](/Render/shader/simple.fx) | HLSL | 1 | 0 | 0 | 1 |
| [Sample/AppState.cpp](/Sample/AppState.cpp) | C++ | 530 | 0 | 85 | 615 |
| [Sample/AppState.h](/Sample/AppState.h) | C++ | 106 | 0 | 11 | 117 |
| [Sample/InputHandlers.cpp](/Sample/InputHandlers.cpp) | C++ | 319 | 0 | 45 | 364 |
| [Sample/InputHandlers.h](/Sample/InputHandlers.h) | C++ | 4 | 0 | 3 | 7 |
| [Sample/Sample.vcxproj](/Sample/Sample.vcxproj) | XML | 15 | 0 | 1 | 16 |
| [Sample/SettingsDialog.cpp](/Sample/SettingsDialog.cpp) | C++ | 453 | 0 | 63 | 516 |
| [Sample/SettingsDialog.h](/Sample/SettingsDialog.h) | C++ | 6 | 0 | 3 | 9 |
| [Sample/main.cpp](/Sample/main.cpp) | C++ | -264 | -35 | -67 | -366 |
| [Sample/res/shader/GBuffer.fx](/Sample/res/shader/GBuffer.fx) | HLSL | 7 | -4 | 3 | 6 |
| [Sample/res/shader/MeshMix.fx](/Sample/res/shader/MeshMix.fx) | HLSL | -24 | -3 | -9 | -36 |
| [Sample/res/shader/MeshNoLighting.fx](/Sample/res/shader/MeshNoLighting.fx) | HLSL | 33 | 0 | 5 | 38 |
| [Sample/res/shader/MeshOld.fx](/Sample/res/shader/MeshOld.fx) | HLSL | 105 | 59 | 42 | 206 |
| [Sample/res/shader/MeshSmooth.fx](/Sample/res/shader/MeshSmooth.fx) | HLSL | 0 | 3 | 0 | 3 |
| [Sample/res/shader/PostEffectFog.fx](/Sample/res/shader/PostEffectFog.fx) | HLSL | 77 | 12 | 19 | 108 |
| [Sample/res/shader/PostEffectSSAO.fx](/Sample/res/shader/PostEffectSSAO.fx) | HLSL | 25 | -1 | 3 | 27 |
| [Sample/res/shader/PostEffectSaturate.fx](/Sample/res/shader/PostEffectSaturate.fx) | HLSL | -1 | 0 | -1 | -2 |
| [Sample/res/shader/PostEffectZShadow.fx](/Sample/res/shader/PostEffectZShadow.fx) | HLSL | 11 | 0 | 1 | 12 |
| [Sample/res/shader/simple.fx](/Sample/res/shader/simple.fx) | HLSL | 1 | 0 | 0 | 1 |
| [Sample/resource.h](/Sample/resource.h) | C++ | 49 | 0 | 3 | 52 |

[Summary](results.md) / [Details](details.md) / [Diff Summary](diff.md) / Diff Details