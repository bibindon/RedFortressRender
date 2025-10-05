#pragma once

#include <d3d9.h>
#include <d3dx9.h>
#include <string>
#include <vector>
#include <memory>
#include <atlbase.h>

namespace NSRender
{

class SkinAnimMeshAlloc : public ID3DXAllocateHierarchy
{

public:
    SkinAnimMeshAlloc(const std::wstring &);

    STDMETHOD(CreateFrame)(THIS_ LPCSTR, LPD3DXFRAME *);
    STDMETHOD(CreateMeshContainer)(THIS_ LPCSTR,
                                   CONST D3DXMESHDATA *,
                                   CONST D3DXMATERIAL *,
                                   CONST D3DXEFFECTINSTANCE *,
                                   DWORD,
                                   CONST DWORD *,
                                   LPD3DXSKININFO,
                                   LPD3DXMESHCONTAINER *);

    STDMETHOD(DestroyFrame)(THIS_ LPD3DXFRAME);
    STDMETHOD(DestroyMeshContainer)(THIS_ LPD3DXMESHCONTAINER);

private:

    std::wstring m_xFilename;

};

struct SkinAnimMeshFrame : public D3DXFRAME
{
    explicit SkinAnimMeshFrame(const std::string& name);

    D3DXMATRIX m_combinedMatrix;
};

struct SkinAnimMeshContainer : public D3DXMESHCONTAINER
{
    std::vector<std::shared_ptr<IDirect3DTexture9> > m_textureList;

    DWORD m_paletteSize = 0;
    DWORD m_influenceCount = 0;
    DWORD m_boneCount = 0;
    LPD3DXBUFFER m_boneBuffer = NULL;
    std::vector<LPD3DXMATRIX> m_frameCombinedMatrix;
    std::vector<D3DXMATRIX> m_boneOffsetMatrices;

    SkinAnimMeshContainer(const std::wstring&,
                          const std::string&,
                          LPD3DXMESH,
                          const D3DXMATERIAL*,
                          const DWORD,
                          const DWORD *,
                          LPD3DXSKININFO);

    void InitializeMaterials(const DWORD&,
                             const D3DXMATERIAL*,
                             const std::wstring&,
                             const LPDIRECT3DDEVICE9&);

    void InitializeBone(const LPD3DXSKININFO&, const LPD3DXMESH&);
    void InitializeFVF(const LPDIRECT3DDEVICE9&);
    void InitializeVertexElement();
};

}

