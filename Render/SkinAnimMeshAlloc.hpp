#pragma once

#include <d3d9.h>
#include <d3dx9.h>
#include <string>
#include <vector>
#include <memory>
#include <atlbase.h>

namespace NSRender
{
// A struct inheriting the 'D3DXFRAME' for owing a transform matrix.
struct SkinAnimMesh_frame : public D3DXFRAME
{
    D3DXMATRIX combined_matrix_;
    explicit SkinAnimMesh_frame(const std::string &);
};

// A struct inheriting the 'D3DXMESHCONTAINER' for owing textures.
struct SkinAnimMesh_container : public D3DXMESHCONTAINER
{
    std::vector<std::shared_ptr<IDirect3DTexture9> > texture_;

    DWORD palette_size_;
    DWORD influence_count_;
    DWORD bone_count_;
    CComPtr<ID3DXBuffer> bone_buffer_;
    std::vector<LPD3DXMATRIX> frame_combined_matrix_;
    std::vector<D3DXMATRIX> bone_offset_matrices_;

    SkinAnimMesh_container(const std::wstring &,
                           const std::string &,
                           LPD3DXMESH,
                           const D3DXMATERIAL *,
                           const DWORD,
                           const DWORD *,
                           LPD3DXSKININFO);

    void initialize_materials(const DWORD &,
                              const D3DXMATERIAL *,
                              const std::wstring &,
                              const LPDIRECT3DDEVICE9 &);

    void initialize_bone(const LPD3DXSKININFO &, const LPD3DXMESH &);
    void initialize_FVF(const LPDIRECT3DDEVICE9 &);
    void initialize_vertex_element();
};

// A class inheriting the 'ID3DXAllocateHierarchy' for implementing an animation mesh.
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

    std::wstring x_filename_;
};

}

