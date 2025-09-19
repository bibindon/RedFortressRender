#pragma once

#include <d3d9.h>
#include <d3dx9.h>
#include <string>
#include <vector>
#include <memory>
#include "AnimController.h"
#include <tchar.h>

namespace NSRender
{
class AnimMeshAllocator;

class AnimMesh
{
public:
    AnimMesh(const std::wstring&,
             const D3DXVECTOR3&,
             const D3DXVECTOR3&,
             const float&,
             const AnimSetMap&);

    ~AnimMesh();

    void Render();
    void SetPos(const D3DXVECTOR3& pos);
    D3DXVECTOR3 GetPos() const;

    float GetScale() const;

    void SetRotate(const D3DXVECTOR3& rotate);
    void SetAnim(const std::wstring& animName, const DOUBLE& pos = -1.f);
    void SetAnimSpeed(const float speed);
    void SetTrackPos(const DOUBLE& pos);
    void SetCenterPos(const D3DXVECTOR3& pos);
    void SetRotateLocal(const D3DXVECTOR3& rotate);

    // 解像度やウィンドウモードを変更したときのための関数
    void OnDeviceLost();
    void OnDeviceReset();

private:

    struct frameRootDeleterObject
    {
        std::shared_ptr<AnimMeshAllocator> allocator_;

        void operator()(const LPD3DXFRAME);
        void releaseMeshAllocator(const LPD3DXFRAME);
    };

    const std::wstring SHADER_FILENAME = _T("res\\shader\\AnimationMeshShader.fx");
    std::shared_ptr<AnimMeshAllocator> m_allocator;
    std::unique_ptr<D3DXFRAME, frameRootDeleterObject> m_frameRoot;
    D3DXMATRIX m_rotationMatrix;
    D3DXMATRIX m_viewMatrix;
    D3DXMATRIX m_projMatrix;

    D3DXVECTOR3 m_centerPos;
    D3DXVECTOR3 m_rotateLocal = D3DXVECTOR3(0.f, 0.f, 0.f);

    LPD3DXEFFECT m_D3DEffect = NULL;

    void UpdateFrameMatrix(const LPD3DXFRAME, const LPD3DXMATRIX);
    void RenderFrame(const LPD3DXFRAME);
    void RenderMeshContainer(const LPD3DXMESHCONTAINER, const LPD3DXFRAME);


    D3DXVECTOR3 m_position;
    D3DXVECTOR3 m_rotation;
    float m_scale = 1.0f;
    std::wstring m_meshName;

    AnimController m_animCtrlr;
};
}

