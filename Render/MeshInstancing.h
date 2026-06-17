#pragma once

#include "Common.h"

#include <atomic>
#include <thread>

namespace NSRender
{

// インスタンシング可能なメッシュ
// 大量に描画しても軽い
class MeshInstancing : public IDeviceResettable
{

public:

    MeshInstancing();
    ~MeshInstancing();

    void Initialize(const std::wstring& filePath, bool async = true);
    void Initialize(const std::wstring& filePath, const std::wstring& csvPath, bool async = true);
    void WaitForLoad();

    void Finalize();

    void AddInstance(const D3DXVECTOR3& pos, float rotationY = 0.0f);
    void SetInstanceOffset(const D3DXVECTOR3& offset);

    void SetHighQuality(bool enabled);

    void Draw();
    void RenderToGBufferEffect(LPD3DXEFFECT effect, const char* techniqueName);
    void RenderToShadowOccluderEffect(LPD3DXEFFECT effect,
                                      const char* techniqueName,
                                      float alphaClipThreshold);

    void OnDeviceLost();
    void OnDeviceReset();

    enum class SwayMode
    {
        Off = 0,
        Normal = 1,
        Wave = 2
    };


private:

    std::wstring m_filePath;
    std::wstring m_customPlacementCsvPath;

    LPD3DXMESH m_pMesh = NULL;

    std::vector<D3DMATERIAL9> m_pMaterials;

    std::vector<LPDIRECT3DTEXTURE9> m_pTextures;
    std::vector<bool> m_materialUsesAlpha;
    std::vector<D3DXATTRIBUTERANGE> m_attributeTable;

    DWORD m_dwNumMaterials = 0;

    LPD3DXEFFECT m_pEffect = NULL;

    struct InstanceData
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float rotationYRadians = 0.0f;
        float scale = 1.0f;
        float padding0 = 0.0f;
        float padding1 = 0.0f;
        float padding2 = 0.0f;
    };

    IDirect3DVertexBuffer9* m_worldPosBuf = nullptr;

    IDirect3DVertexDeclaration9* m_decl = nullptr;

    void copyBuf(unsigned sz, void* src, IDirect3DVertexBuffer9* buf);
    void UpdateVisibleInstances();
    void SortInstancesBackToFront();
    void UpdateInstanceBuffer();
    bool LoadPlacementCsv();
    bool LoadPlacementCsv(const std::wstring& csvPath);
    DWORD GetSubsetMaterialIndex(DWORD subsetIndex) const;
    bool IsSubsetAlphaMaterial(DWORD subsetIndex) const;
    HRESULT DrawInstancedSubset(DWORD subsetIndex) const;
    void InitializeInternal();

    std::vector<InstanceData> m_allInstances;
    std::vector<InstanceData> m_instances;
    bool m_loadedPlacementCsv = false;
    bool m_highQualityEnabled = true;
    bool m_autoHide = false;
    SwayMode m_swayMode = SwayMode::Off;
    D3DXVECTOR3 m_instanceOffset = D3DXVECTOR3(0.0f, 0.0f, 0.0f);

    std::atomic<bool> m_bLoaded { false };
    std::thread m_loadThread;

};

}

