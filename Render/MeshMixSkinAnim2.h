#pragma once

#include "MeshMixSkinAnim.h"

namespace NSRender
{

class MeshMixSkinAnim2 : public MeshMixSkinAnim
{
public:
    MeshMixSkinAnim2(const std::wstring& filename,
                     const D3DXVECTOR3& pos,
                     const D3DXVECTOR3& rotate,
                     const float scale,
                     const stMeshParam& param,
                     const AnimSetMap& animSetMap);
    MeshMixSkinAnim2(const std::wstring& meshFilename,
                     const std::wstring& animationFilename,
                     const D3DXVECTOR3& pos,
                     const D3DXVECTOR3& rotate,
                     const float scale,
                     const stMeshParam& param,
                     const AnimSetMap& animSetMap);
};

}
