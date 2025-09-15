#pragma once

#include <d3d9.h>
#include <d3dx9.h>
#include <string>
#include <vector>
#include <memory>
#include "AnimController.h"

namespace NSRender

{

class SkinAnimMeshAlloc;

// A class that provides operations for a mesh file having animations.
class SkinAnimMesh
{
public:
    SkinAnimMesh(const std::shared_ptr<IDirect3DDevice9> &,
                 const std::wstring &,
                 const D3DXVECTOR3 &,
                 const D3DXVECTOR3 &,
                 const float &,
                 const AnimSetMap& animSetMap);

    ~SkinAnimMesh();

private:
    void render_impl(const D3DXMATRIX &, const D3DXMATRIX &);

    struct frame_root_deleter_object
    {
        std::shared_ptr<SkinAnimMeshAlloc> allocator_;
        void operator()(const LPD3DXFRAME);
        void release_mesh_allocator(const LPD3DXFRAME);
    };

    const static std::wstring SHADER_FILENAME;
    std::shared_ptr<IDirect3DDevice9> d3d_device_;
    std::shared_ptr<SkinAnimMeshAlloc> allocator_;
    std::unique_ptr<D3DXFRAME, frame_root_deleter_object> frame_root_;
    D3DXMATRIX rotation_matrix_;
    std::vector<D3DXMATRIX> world_matrix_array_;
    D3DXVECTOR3 center_coodinate_;
    float scale_;

    // For effect.
    D3DXHANDLE view_projection_handle_;
    D3DXHANDLE scale_handle_;

    void update_frame_matrix(const LPD3DXFRAME, const LPD3DXMATRIX);
    void render_frame(const LPD3DXFRAME);
    void render_mesh_container(const LPD3DXMESHCONTAINER);

    HRESULT allocate_bone_matrix(LPD3DXMESHCONTAINER);
    HRESULT allocate_all_bone_matrices(LPD3DXFRAME);

    LPD3DXEFFECT m_D3DEffect = nullptr;
    D3DXVECTOR3 position_;
    D3DXVECTOR3 rotation_;

    AnimController m_animCtrlr;
};

} // namespace early_go 

