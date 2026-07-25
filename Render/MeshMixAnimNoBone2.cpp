#include "MeshMixAnimNoBone2.h"

namespace NSRender
{

MeshMixAnimNoBone2::MeshMixAnimNoBone2(const std::wstring& filename,
                                       const D3DXVECTOR3& pos,
                                       const D3DXVECTOR3& rotate,
                                       const float scale,
                                       const stMeshParam& param,
                                       const AnimSetMap& animSetMap)
    : MeshMixSkinAnim2(filename,
                       pos,
                       rotate,
                       scale,
                       param,
                       animSetMap,
                       MeshMixSkinAnimLoadMode::Blender512Custom)
{
}

MeshMixAnimNoBone2::MeshMixAnimNoBone2(const std::wstring& meshFilename,
                                       const std::wstring& animationFilename,
                                       const D3DXVECTOR3& pos,
                                       const D3DXVECTOR3& rotate,
                                       const float scale,
                                       const stMeshParam& param,
                                       const AnimSetMap& animSetMap)
    : MeshMixSkinAnim2(meshFilename,
                       animationFilename,
                       pos,
                       rotate,
                       scale,
                       param,
                       animSetMap,
                       MeshMixSkinAnimLoadMode::Blender512Custom)
{
}

void MeshMixAnimNoBone2::RenderToEffect(LPD3DXEFFECT effect,
                                        const D3DXMATRIX& viewProjectionMatrix)
{
    if (effect == nullptr)
    {
        return;
    }

    const D3DXHANDLE viewProjectionHandle =
        effect->GetParameterByName(nullptr, "g_matViewProj");
    if (viewProjectionHandle != nullptr)
    {
        effect->SetMatrix(viewProjectionHandle, &viewProjectionMatrix);
    }

    MeshMixSkinAnim2::RenderToEffect(effect);
}

}
