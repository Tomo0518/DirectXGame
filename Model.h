#pragma once
#include "TomoEngine.h"
#include "ModelData.h"

class Model {
public:
    // モデル読み込み
    static Model* CreateFromOBJ(const std::string& directoryPath, const std::string& filename);

	// 初期化
    void Initialize(const ModelData& modelData);

    // 描画準備（頂点バッファ、マテリアル、テクスチャのセット）
    void PreDraw(ID3D12GraphicsCommandList* commandList);

    // 描画実行
    void Draw(ID3D12GraphicsCommandList* commandList, uint32_t instanceCount = 1);

private:
    ModelData modelData_;

    // リソース
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

    ResourceObject materialResource;
    Material* materialData = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> textureResource;
    D3D12_GPU_DESCRIPTOR_HANDLE textureHandle;
};