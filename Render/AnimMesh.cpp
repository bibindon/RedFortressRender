#include "AnimMesh.h"

#include "AnimMeshAlloc.h"
#include "Common.h"
#include "Light.h"
#include "Camera.h"
#include <cassert>

void NSRender::AnimMesh::frame_root_deleter_object::operator()(const LPD3DXFRAME frameRoot)
{
    release_mesh_allocator(frameRoot);
}

void NSRender::AnimMesh::frame_root_deleter_object::release_mesh_allocator(const LPD3DXFRAME frame)
{
    if (frame->pMeshContainer != nullptr)
    {
        allocator_->DestroyMeshContainer(frame->pMeshContainer);
    }
    if (frame->pFrameSibling != nullptr)
    {
        release_mesh_allocator(frame->pFrameSibling);
    }
    if (frame->pFrameFirstChild != nullptr)
    {
        release_mesh_allocator(frame->pFrameFirstChild);
    }
    allocator_->DestroyFrame(frame);
}

NSRender::AnimMesh::AnimMesh(
    const std::wstring& xFilename,
    const D3DXVECTOR3& position,
    const D3DXVECTOR3& rotation,
    const float& scale,
    const AnimSetMap& animSetMap)
    : m_allocator { NEW AnimMeshAllocator { xFilename } }
    , m_frameRoot { nullptr, frame_root_deleter_object { m_allocator } }
    , m_rotationMatrix { D3DMATRIX { } }
    , m_position { position }
    , m_rotation { rotation }
    , m_centerPos { 0.0f, 0.0f, 0.0f }
{
    HRESULT result { 0 };
    result = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                      SHADER_FILENAME.c_str(),
                                      nullptr,
                                      nullptr,
                                      D3DXSHADER_OPTIMIZATION_LEVEL3,
                                      nullptr,
                                      &m_D3DEffect,
                                      nullptr);

    if (FAILED(result))
    {
        throw std::exception("Failed to create an effect file.");
    }

    m_worldHandle = m_D3DEffect->GetParameterByName(nullptr, "g_matWorld");
    m_worldViewProjHandle = m_D3DEffect->GetParameterByName(nullptr, "g_matWorldViewProj");
    m_lightNormalHandle = m_D3DEffect->GetParameterByName(nullptr, "g_vecLightNormal");
    m_brightnessHandle = m_D3DEffect->GetParameterByName(nullptr, "g_fLightBrigntness");
    m_meshTextureHandle = m_D3DEffect->GetParameterByName(nullptr, "g_texture");
    m_diffuseHandle = m_D3DEffect->GetParameterByName(nullptr, "g_vecDiffuse");

    LPD3DXFRAME tempRootFrame = nullptr;
    LPD3DXANIMATIONCONTROLLER tempAnimController = NULL;

    result = D3DXLoadMeshHierarchyFromX(xFilename.c_str(),
                                        D3DXMESH_MANAGED,
                                        Common::D3DDevice(),
                                        m_allocator.get(),
                                        nullptr,
                                        &tempRootFrame,
                                        &tempAnimController);

    if (FAILED(result))
    {
        throw std::exception("Failed to load a x-file.");
    }

    if (tempAnimController == nullptr)
    {
        throw std::exception("Failed to load a x-file.2");
    }
    // lazy initialization 
    m_frameRoot.reset(tempRootFrame);
    m_animCtrlr.Init(tempAnimController, animSetMap);

    m_scale = scale;
    m_meshName = xFilename;
}

NSRender::AnimMesh::~AnimMesh()
{
    m_animCtrlr.Finalize();
    SAFE_RELEASE(m_D3DEffect);
}

void NSRender::AnimMesh::Render()
{
    D3DXVECTOR4 lightNormal = Light::GetLightNormal();

    // モデルが太陽のほうを向いているか
    D3DXVECTOR4 dirToLight { 0.f,0.f,0.f,0.f };

    // モデルの方向ベクトル
    D3DXVECTOR4 modelDir { 0.f,0.f,0.f,0.f };

    modelDir.x = std::sin(m_rotation.y);
    modelDir.z = std::cos(m_rotation.y + D3DX_PI);
    modelDir.y = 0.0f;

    // どういうわけか、zが上になっている
    dirToLight.x = modelDir.x + lightNormal.x;
    dirToLight.y = modelDir.z + lightNormal.z;
    dirToLight.z = modelDir.y + lightNormal.y;

    D3DXVec4Normalize(&dirToLight, &dirToLight);

    m_D3DEffect->SetVector(m_lightNormalHandle, &dirToLight);

    m_D3DEffect->SetFloat(m_brightnessHandle, Light::GetBrightness());

    m_viewMatrix = Camera::GetViewMatrix();
    m_projMatrix = Camera::GetProjMatrix();

    m_animCtrlr.Update();

    D3DXMATRIX worldMatrix { };
    D3DXMatrixIdentity(&worldMatrix);
    {
        D3DXMATRIX mat { };
        D3DXMatrixTranslation(&mat, -m_centerPos.x, -m_centerPos.y, -m_centerPos.z);
        worldMatrix *= mat;

        D3DXMatrixScaling(&mat, m_scale, m_scale, m_scale);
        worldMatrix *= mat;

        D3DXMatrixRotationYawPitchRoll(&mat, m_rotateLocal.y, m_rotateLocal.x, m_rotateLocal.z);
        worldMatrix *= mat;

        D3DXMatrixTranslation(&mat, m_centerPos.x, m_centerPos.y, m_centerPos.z);
        worldMatrix *= mat;

        D3DXMatrixRotationYawPitchRoll(&mat, m_rotation.y, m_rotation.x, m_rotation.z);
        worldMatrix *= mat;

        D3DXMatrixTranslation(&mat, m_position.x, m_position.y, m_position.z);
        worldMatrix *= mat;
    }
    UpdateFrameMatrix(m_frameRoot.get(), &worldMatrix);
    RenderFrame(m_frameRoot.get());
}

void NSRender::AnimMesh::SetPos(const D3DXVECTOR3& pos)
{
    m_position = pos;
}

D3DXVECTOR3 NSRender::AnimMesh::GetPos() const
{
    return m_position;
}

float NSRender::AnimMesh::GetScale() const
{
    return m_scale;
}

void NSRender::AnimMesh::SetRotate(const D3DXVECTOR3& rotate)
{
    m_rotation.x = fmod(rotate.x, D3DX_PI * 2);
    m_rotation.y = fmod(rotate.y, D3DX_PI * 2);
    m_rotation.z = fmod(rotate.z, D3DX_PI * 2);
}

void NSRender::AnimMesh::SetAnim(const std::wstring& animName, const DOUBLE& pos)
{
    m_animCtrlr.SetAnim(animName, pos);
}

void NSRender::AnimMesh::SetAnimSpeed(const float speed)
{
    m_animCtrlr.SetAnimSpeed(speed);
}

void NSRender::AnimMesh::SetTrackPos(const DOUBLE& pos)
{
    // TODO remove
//    m_animationStrategy->SetTrackPos(m_pos);
//    m_animCtrlr.SetTrackPos();
}

void NSRender::AnimMesh::SetCenterPos(const D3DXVECTOR3& pos)
{
    m_centerPos = pos;
}

void NSRender::AnimMesh::SetRotateLocal(const D3DXVECTOR3& rotate)
{
    m_rotateLocal = rotate;
}

void NSRender::AnimMesh::UpdateFrameMatrix(const LPD3DXFRAME frameBase, const LPD3DXMATRIX parentMatrix)
{
    AnimMeshFrame* frame { static_cast<AnimMeshFrame*>(frameBase) };
    if (parentMatrix != nullptr)
    {
        frame->m_combinedMatrix = frame->TransformationMatrix * (*parentMatrix);
//        std::string work = "RightHand";
//        if (work == frame->Name)
//        {
//            SharedObj::SetRightHandMat(frame->m_combinedMatrix);
//        }
    }
    else
    {
        frame->m_combinedMatrix = frame->TransformationMatrix;
    }

    if (frame->pFrameSibling != nullptr)
    {
        UpdateFrameMatrix(frame->pFrameSibling, parentMatrix);
    }
    if (frame->pFrameFirstChild != nullptr)
    {
        UpdateFrameMatrix(frame->pFrameFirstChild, &frame->m_combinedMatrix);
    }
}

void NSRender::AnimMesh::RenderFrame(const LPD3DXFRAME frame)
{
    {
        LPD3DXMESHCONTAINER mesh_container { frame->pMeshContainer };
        while (mesh_container != nullptr)
        {
            RenderMeshContainer(mesh_container, frame);
            mesh_container = mesh_container->pNextMeshContainer;
        }
    }

    if (frame->pFrameSibling != nullptr)
    {
        RenderFrame(frame->pFrameSibling);
    }
    if (frame->pFrameFirstChild != nullptr)
    {
        RenderFrame(frame->pFrameFirstChild);
    }
}

void NSRender::AnimMesh::RenderMeshContainer(
    const LPD3DXMESHCONTAINER meshContainerBase, const LPD3DXFRAME frameBase)
{
    AnimMeshFrame* frame { static_cast<AnimMeshFrame*>(frameBase) };

    D3DXMATRIX worldViewProjMatrix { frame->m_combinedMatrix };

    m_D3DEffect->SetMatrix(m_worldHandle, &worldViewProjMatrix);

    worldViewProjMatrix *= m_viewMatrix;
    worldViewProjMatrix *= m_projMatrix;

    m_D3DEffect->SetMatrix(m_worldViewProjHandle, &worldViewProjMatrix);

    m_D3DEffect->Begin(nullptr, 0);

    HRESULT result = m_D3DEffect->BeginPass(0);
    if (FAILED(result))
    {
        m_D3DEffect->End();
        throw std::exception("Failed 'BeginPass' function.");
    }

    AnimMeshContainer* meshContainer { static_cast<AnimMeshContainer*>(meshContainerBase) };

    for (DWORD i = 0; i < meshContainer->NumMaterials; ++i)
    {
        D3DXVECTOR4 vecDiffuse { 0.f, 0.f, 0.f, 0.f };

        if (true)
        {
            vecDiffuse.x = 1.f;
            vecDiffuse.y = 1.f;
            vecDiffuse.z = 1.f;
            vecDiffuse.w = 1.f;
        }
        else
        {
            vecDiffuse.x = meshContainer->pMaterials[i].MatD3D.Diffuse.r;
            vecDiffuse.y = meshContainer->pMaterials[i].MatD3D.Diffuse.g;
            vecDiffuse.z = meshContainer->pMaterials[i].MatD3D.Diffuse.b;
            vecDiffuse.w = meshContainer->pMaterials[i].MatD3D.Diffuse.a;
        }

        m_D3DEffect->SetVector(m_diffuseHandle, &vecDiffuse);
        m_D3DEffect->SetTexture(m_meshTextureHandle, meshContainer->m_vecTexture.at(i));

        m_D3DEffect->CommitChanges();
        meshContainer->MeshData.pMesh->DrawSubset(i);
    }
    m_D3DEffect->EndPass();
    m_D3DEffect->End();
}

// 解像度やウィンドウモードを変更したときのための関数
void NSRender::AnimMesh::OnDeviceLost()
{
    HRESULT hr = m_D3DEffect->OnLostDevice();
    assert(hr == S_OK);
}

// 解像度やウィンドウモードを変更したときのための関数
void NSRender::AnimMesh::OnDeviceReset()
{
    HRESULT hr = m_D3DEffect->OnResetDevice();
    assert(hr == S_OK);
}

