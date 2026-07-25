#pragma once

#include "MeshMixSkinAnim2.h"

namespace NSRender
{

class MeshMixAnimNoBone2 : public MeshMixSkinAnim2
{
public:
    MeshMixAnimNoBone2(const std::wstring& filename,
                       const D3DXVECTOR3& pos,
                       const D3DXVECTOR3& rotate,
                       float scale,
                       const stMeshParam& param,
                       const AnimSetMap& animSetMap);

    MeshMixAnimNoBone2(const std::wstring& meshFilename,
                       const std::wstring& animationFilename,
                       const D3DXVECTOR3& pos,
                       const D3DXVECTOR3& rotate,
                       float scale,
                       const stMeshParam& param,
                       const AnimSetMap& animSetMap);

    ~MeshMixAnimNoBone2() override = default;

    void RenderToEffect(LPD3DXEFFECT effect,
                        const D3DXMATRIX& viewProjectionMatrix);
};

}
