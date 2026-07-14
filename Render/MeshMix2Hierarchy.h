#pragma once

#include <d3d9.h>
#include <d3dx9.h>
#include <map>
#include <string>
#include <vector>
#include <memory>
#include <atlbase.h>

namespace NSRender
{

struct MeshMix2MeshContainer;

class MeshMix2MeshAlloc : public ID3DXAllocateHierarchy
{

public:
    MeshMix2MeshAlloc(const std::wstring &);
    ~MeshMix2MeshAlloc();

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

private:

    std::wstring m_xFilename;
    std::wstring m_baseDirectory;

    MeshMix2MeshContainer* m_container = nullptr;

    std::map<std::wstring, LPDIRECT3DTEXTURE9> m_textureCache;

    LPDIRECT3DTEXTURE9 LoadTextureCached(const std::wstring& texturePath);
    std::wstring ResolveTexturePath(const char* textureFilename) const;
    void ClearTextureCache();
};

struct MeshMix2Frame : public D3DXFRAME
{
    D3DXMATRIX m_combinedMatrix;
};

struct MeshMix2MeshContainer : public D3DXMESHCONTAINER
{
    std::vector<LPDIRECT3DTEXTURE9> m_textureList;
};

}


