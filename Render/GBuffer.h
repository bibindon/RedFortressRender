#pragma once

#include "Common.h"
#include "MeshMix.h"

namespace NSRender
{

// 各ピクセルの深度とワールド座標を表した画像を生成
class GBuffer : public IDeviceResettable
{

public:

    void Initialize();

    void Draw(const std::vector<MeshMix>& meshList,
              LPDIRECT3DTEXTURE9* Z,
              LPDIRECT3DTEXTURE9* Pos,
              LPDIRECT3DTEXTURE9* Normal);

    void Finalize();

    void OnDeviceLost();
    void OnDeviceReset();

private:

    LPDIRECT3DTEXTURE9 m_texRenderTargetZ = NULL;
    LPDIRECT3DTEXTURE9 m_texRenderTargetPos = NULL;
    LPDIRECT3DTEXTURE9 m_texRenderTargetNormal = NULL;

    LPD3DXEFFECT m_fxGBuffer = NULL;

    void CreateRawResource();
};

}

