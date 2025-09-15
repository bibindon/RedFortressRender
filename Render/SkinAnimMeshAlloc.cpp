#include "SkinAnimMesh.hpp"

#include "Common.h"
#include "Util.h"
#include "SkinAnimMeshAlloc.hpp"

using std::vector;
using std::string;

namespace NSRender
{
// c'tor 
SkinAnimMesh_frame::SkinAnimMesh_frame(const string &name)
    : D3DXFRAME{}, // Initializes member with zero. 
      combined_matrix_{}
{
    Name = NEW char[name.length() + 1];
    strcpy_s(Name, name.length() + 1, name.c_str());

    // Make an argument an identity matrix. 
    D3DXMatrixIdentity(&TransformationMatrix);
    D3DXMatrixIdentity(&combined_matrix_);
}

// A constructor which only initializes member variables from the beginning to the end.
SkinAnimMesh_container::SkinAnimMesh_container(
    const std::wstring &x_filename,
    const string &mesh_name,
    LPD3DXMESH mesh,
    const D3DXMATERIAL *materials,
    const DWORD materials_count,
    const DWORD *adjacency,
    LPD3DXSKININFO skin_info)
    : D3DXMESHCONTAINER{}, // Initializes with zero. 
      texture_{},
      palette_size_{},
      influence_count_{},
      bone_count_{},
      bone_buffer_{nullptr},
      frame_combined_matrix_{},
      bone_offset_matrices_{}
{
    Name = NEW char[mesh_name.length() + 1];
    strcpy_s(Name, mesh_name.length() + 1, mesh_name.c_str());

    LPDIRECT3DDEVICE9 d3d_device{nullptr};
    mesh->GetDevice(&d3d_device);

    // This IF sentence is just initializing the 'MeshData' of a member variable.
    // When this mesh doesn't have normal vector, add it.
    HRESULT result{};
    if (!(mesh->GetFVF() & D3DFVF_NORMAL))
    {
        MeshData.Type = D3DXMESHTYPE_MESH;
        result = mesh->CloneMeshFVF(
            mesh->GetOptions(),
            mesh->GetFVF() | D3DFVF_NORMAL,
            d3d_device,
            &MeshData.pMesh);
        if (FAILED(result))
        {
            throw std::exception("Failed 'CloneMeshFVF' function.");
        }
        mesh = MeshData.pMesh;
        D3DXComputeNormals(mesh, nullptr);
    }
    else
    {
        MeshData.pMesh = mesh;
        MeshData.Type = D3DXMESHTYPE_MESH;
        mesh->AddRef();
    }

    // Initialize the 'pAdjacency' of a member variable. 
    DWORD adjacency_count{mesh->GetNumFaces() * 3};
    pAdjacency = NEW DWORD[adjacency_count];

    for (DWORD i{}; i < adjacency_count; ++i)
    {
        pAdjacency[i] = adjacency[i];
    }

    initialize_materials(materials_count, materials, x_filename, d3d_device);
    initialize_bone(skin_info, mesh);
    initialize_FVF(d3d_device);
    initialize_vertex_element();
}

void SkinAnimMesh_container::initialize_materials(const DWORD &materials_count,
                                                  const D3DXMATERIAL *materials,
                                                  const std::wstring &x_filename,
                                                  const LPDIRECT3DDEVICE9 &d3d_device)
{
    // This strange bracket is measures of being interpretered as WinAPI macro. 
    NumMaterials = (std::max)(1UL, materials_count);
    pMaterials = NEW D3DXMATERIAL[NumMaterials];
    vector<std::shared_ptr<IDirect3DTexture9> > temp_texture(NumMaterials);
    texture_.swap(temp_texture);

    // Initialize the 'pMaterials' and the 'texture_' of member variables if there are.
    if (materials_count > 0)
    {
        for (DWORD i{}; i < materials_count; ++i)
        {
            pMaterials[i] = materials[i];
            if (pMaterials[i].pTextureFilename != nullptr)
            {
                LPDIRECT3DTEXTURE9 temp_texture{};
                std::wstring filename = Util::Utf8ToWstring(pMaterials[i].pTextureFilename);
                if (FAILED(D3DXCreateTextureFromFile(d3d_device,
                                                     filename.c_str(),
                                                     &temp_texture)))
                {
                    throw std::exception("texture file is not found.");
                }
                else
                {
                    texture_.at(i).reset(temp_texture);
                }
            }
        }
    }
    else
    {
        pMaterials[0].MatD3D.Diffuse = D3DCOLORVALUE{0.5f, 0.5f, 0.5f, 0};
        pMaterials[0].MatD3D.Ambient = D3DCOLORVALUE{0.5f, 0.5f, 0.5f, 0};
        pMaterials[0].MatD3D.Specular = pMaterials[0].MatD3D.Diffuse;
    }
}

void SkinAnimMesh_container::initialize_bone(
    const LPD3DXSKININFO &skin_info, const LPD3DXMESH &mesh)
{
    if (skin_info == nullptr)
    {
        throw std::exception("Failed to get skin info.");
    }
    pSkinInfo = skin_info;
    pSkinInfo->AddRef();

    UINT bone_count = pSkinInfo->GetNumBones();
    bone_offset_matrices_.resize(bone_count);

    for (DWORD i = 0; i < bone_count; ++i)
    {
        bone_offset_matrices_[i] = *pSkinInfo->GetBoneOffsetMatrix(i);
    }

    // TODO Improve.
    DWORD MAX_MATRICES = 26;
    palette_size_ = (std::min)(MAX_MATRICES, pSkinInfo->GetNumBones());

    // generate skinned mesh
    SAFE_RELEASE(MeshData.pMesh);

    LPD3DXBUFFER bone_buffer{};
    if (FAILED(pSkinInfo->ConvertToIndexedBlendedMesh(mesh,
                                                      0 /* not used */, 
                                                      palette_size_,
                                                      pAdjacency,
                                                      nullptr,
                                                      nullptr,
                                                      nullptr,
                                                      &influence_count_,
                                                      &bone_count_,
                                                      &bone_buffer,
                                                      &MeshData.pMesh)))
    {
        throw std::exception("Failed to get skin info.");
    }
    bone_buffer_ = bone_buffer;
}

void SkinAnimMesh_container::initialize_FVF(
    const LPDIRECT3DDEVICE9 &d3d_device)
{
    DWORD new_FVF = (MeshData.pMesh->GetFVF() & D3DFVF_POSITION_MASK) |
                    D3DFVF_NORMAL | D3DFVF_TEX1 | D3DFVF_LASTBETA_UBYTE4;

    if (new_FVF != MeshData.pMesh->GetFVF())
    {
        LPD3DXMESH p_mesh{};
        HRESULT hresult = MeshData.pMesh->CloneMeshFVF(
            MeshData.pMesh->GetOptions(),
            new_FVF,
            d3d_device,
            &p_mesh);
        if (SUCCEEDED(hresult))
        {
            MeshData.pMesh->Release();
            MeshData.pMesh = p_mesh;
            p_mesh = NULL;
        }
    }
}

void SkinAnimMesh_container::initialize_vertex_element()
{
    D3DVERTEXELEMENT9 decl[MAX_FVF_DECL_SIZE];
    LPD3DVERTEXELEMENT9 current_decl;
    HRESULT result = MeshData.pMesh->GetDeclaration(decl);
    if (FAILED(result))
    {
        throw std::exception("Failed to get skin info.");
    }

    current_decl = decl;
    while (current_decl->Stream != 0xff)
    {
        if ((current_decl->Usage == D3DDECLUSAGE_BLENDINDICES) && (current_decl->UsageIndex == 0))
        {
            current_decl->Type = D3DDECLTYPE_D3DCOLOR;
        }
        current_decl++;
    }

    result = MeshData.pMesh->UpdateSemantics(decl);
    if (FAILED(result))
    {
        throw std::exception("Failed to get skin info.");
    }
}

SkinAnimMeshAlloc::SkinAnimMeshAlloc(const std::wstring &x_filename)
    : ID3DXAllocateHierarchy{},
      x_filename_(x_filename) {}

// Alghough it's camel case and a strange type name, because this function is a pure virtual
// function of 'ID3DXAllocateHierarchy'.
STDMETHODIMP SkinAnimMeshAlloc::CreateFrame(LPCSTR name, LPD3DXFRAME *new_frame)
{
    *new_frame = NEW SkinAnimMesh_frame(name);
    return S_OK;
}

// Alghough it's camel case and a strange type name, because this function is a pure virtual
// function of 'ID3DXAllocateHierarchy'.
STDMETHODIMP SkinAnimMeshAlloc::CreateMeshContainer(
    LPCSTR mesh_name,
    CONST D3DXMESHDATA *mesh_data,
    CONST D3DXMATERIAL *materials,
    CONST D3DXEFFECTINSTANCE *,
    DWORD materials_count,
    CONST DWORD *adjacency,
    LPD3DXSKININFO skin_info,
    LPD3DXMESHCONTAINER *mesh_container)
{
    try
    {
        *mesh_container = NEW SkinAnimMesh_container(x_filename_,
                                                     mesh_name,
                                                     mesh_data->pMesh,
                                                     materials,
                                                     materials_count,
                                                     adjacency,
                                                     skin_info);
    }
    catch (const std::exception&)
    {
        return E_FAIL;
    }
    return S_OK;
}

// Alghough it's camel case and a strange type name, because this function is a pure virtual
// function of 'ID3DXAllocateHierarchy'.
STDMETHODIMP SkinAnimMeshAlloc::DestroyFrame(LPD3DXFRAME frame)
{
    SAFE_DELETE_ARRAY(frame->Name);
    frame->~D3DXFRAME();
    SAFE_DELETE(frame);
    return S_OK;
}

// Alghough it's camel case and a strange type name, because this function is a pure virtual
// function of 'ID3DXAllocateHierarchy'.
STDMETHODIMP SkinAnimMeshAlloc::DestroyMeshContainer(
    LPD3DXMESHCONTAINER mesh_container_base)
{
    SkinAnimMesh_container *mesh_container{
        static_cast<SkinAnimMesh_container *>(mesh_container_base)};

    SAFE_RELEASE(mesh_container->pSkinInfo);
    SAFE_DELETE_ARRAY(mesh_container->Name);
    SAFE_DELETE_ARRAY(mesh_container->pAdjacency);
    SAFE_DELETE_ARRAY(mesh_container->pMaterials);
    SAFE_RELEASE(mesh_container->MeshData.pMesh);
    SAFE_DELETE(mesh_container);

    return S_OK;
}
} // namespace early_go 
