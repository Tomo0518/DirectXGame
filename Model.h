#pragma once
#include "TomoEngine.h"
#include "ModelData.h"
#include <vector>

class Model {
public:
    // モデル読み込み
    // ※ commandList: テクスチャ転送用。呼び出し元でBegin済みのものを渡す
    // ※ srvCpuHandle/srvGpuHandle: このモデルのテクスチャ用SRVを作るディスクリプタヒープ上の場所
    static Model* CreateFromOBJ(
        const std::string& directoryPath,
        const std::string& filename,
        ID3D12GraphicsCommandList* commandList,
        D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle,
        D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle
    );

    // 初期化
    void Initialize(
        const ModelData& modelData,
        ID3D12GraphicsCommandList* commandList,
        D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle,
        D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle
    );

    // 描画準備（頂点バッファ、マテリアルのセット）
    void PreDraw(ID3D12GraphicsCommandList* commandList);

    // 描画実行
    // rootParameterIndex: テクスチャ(SRV)のDescriptorTableを設定するルートパラメータ番号 (デフォルトは2)
    void Draw(ID3D12GraphicsCommandList* commandList, uint32_t instanceCount = 1, uint32_t rootParameterIndex = 2);

private:
    ModelData modelData_;

    // リソース
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};

    ResourceObject materialResource;
    Material* materialData = nullptr;

    // テクスチャ関連
    Microsoft::WRL::ComPtr<ID3D12Resource> textureResource;
    D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU{}; // 描画時にセットするGPUハンドル

    // テクスチャ転送用の中間リソース
    // (GPU転送が完了するまで保持しておく必要があるため、メンバ変数として延命させる)
    std::vector<ResourceObject> intermediateResources_;
};