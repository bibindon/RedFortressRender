#pragma once

#include <d3d9.h>
#include <d3dx9.h>
#include <string>
#include <vector>
#include <memory>
#include <atlbase.h>

namespace NSRender
{

struct SkinAnimMeshContainer;

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

    void InitializeMaterials(const DWORD,
                             const D3DXMATERIAL*,
                             const std::wstring&);

    void InitializeBone(const LPD3DXSKININFO, const LPD3DXMESH);
    void InitializeFVF();
    void InitializeVertexElement();

private:

    std::wstring m_xFilename;

    SkinAnimMeshContainer* m_container = nullptr;
};

struct SkinAnimMeshFrame : public D3DXFRAME
{
    D3DXMATRIX m_combinedMatrix;
};

struct SkinAnimMeshContainer : public D3DXMESHCONTAINER
{
    std::vector<LPDIRECT3DTEXTURE9> m_textureList;

    DWORD m_paletteSize = 0;
    DWORD m_influenceCount = 0;
    DWORD m_boneCount = 0;
    LPD3DXBUFFER m_boneBuffer = NULL;
    std::vector<LPD3DXMATRIX> m_frameCombinedMatrix;
    std::vector<D3DXMATRIX> m_boneOffsetMatrices;
};

}

