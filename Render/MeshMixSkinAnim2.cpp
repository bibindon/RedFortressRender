#include "MeshMixSkinAnim2.h"

namespace NSRender
{

MeshMixSkinAnim2::MeshMixSkinAnim2(const std::wstring& filename,
                                   const D3DXVECTOR3& pos,
                                   const D3DXVECTOR3& rotate,
                                   const float scale,
                                   const stMeshParam& param,
                                   const AnimSetMap& animSetMap)
    : MeshMixSkinAnim(filename,
                      pos,
                      D3DXVECTOR3(rotate.x, rotate.y + D3DX_PI, rotate.z),
                      scale,
                      param,
                      animSetMap,
                      MeshMixSkinAnimLoadMode::Blender512Custom)
{
}

MeshMixSkinAnim2::MeshMixSkinAnim2(const std::wstring& meshFilename,
                                   const std::wstring& animationFilename,
                                   const D3DXVECTOR3& pos,
                                   const D3DXVECTOR3& rotate,
                                   const float scale,
                                   const stMeshParam& param,
                                   const AnimSetMap& animSetMap)
    : MeshMixSkinAnim(meshFilename,
                      animationFilename,
                      pos,
                      D3DXVECTOR3(rotate.x, rotate.y + D3DX_PI, rotate.z),
                      scale,
                      param,
                      animSetMap,
                      MeshMixSkinAnimLoadMode::Blender512Custom)
{
}

}
