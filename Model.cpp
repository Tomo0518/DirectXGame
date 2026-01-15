#include "Model.h"
#include "GraphicsCore.h"
#include "ResourcesUtility.h"
#include "DescriptorUtility.h"
#include <cassert>

// 静的ファクトリメソッド: OBJファイルからモデル生成
Model* Model::CreateFromOBJ(const std::string& directoryPath, const std::string& filename) {
    Model* model = new Model();

    // OBJファイルを読み込む
    ModelData modelData = LoadObjFile(directoryPath, filename);

    // モデルを初期化
    model->Initialize(modelData);

    return model;
}

// モデルの初期化
void Model::Initialize(const ModelData& modelData) {
    modelData_ = modelData;

    ID3D12Device* device = GraphicsCore::GetInstance()->GetDevice();
    assert(device);

    // ===================================
    // 頂点バッファの生成
    // ===================================
    vertexBuffer = CreateBufferResource(device, sizeof(VertexData) * modelData_.vertices.size());

    // 頂点バッファビューの作成
    vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vertexBufferView.SizeInBytes = static_cast<UINT>(sizeof(VertexData) * modelData_.vertices.size());
    vertexBufferView.StrideInBytes = sizeof(VertexData);

    // 頂点データをバッファにコピー
    VertexData* vertexData = nullptr;
    vertexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
    std::memcpy(vertexData, modelData_.vertices.data(), sizeof(VertexData) * modelData_.vertices.size());
    // Map したままにする（毎フレーム更新する場合は Unmap しない方が効率的）

    // ===================================
    // マテリアルリソースの生成
    // ===================================
    materialResource = CreateBufferResource(device, Align256(sizeof(Material)));

    // マテリアルデータを書き込む
    materialResource.Get()->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
    materialData->color = { 1.0f, 1.0f, 1.0f, 1.0f }; // デフォルトは白
    materialData->enableLighting = true;
    materialData->uvTransform = Matrix4x4::MakeIdentity4x4();
    // Map したままにする

    // ===================================
    // テクスチャの読み込み
    // ===================================
    if (!modelData_.material.textureFilePath.empty()) {
        // テクスチャファイルを読み込む
        DirectX::ScratchImage mipImages = LoadTexture(modelData_.material.textureFilePath);
        const DirectX::TexMetadata& metadata = mipImages.GetMetadata();

        // テクスチャリソースを生成
        textureResource = CreateTextureResource(device, metadata);

		// コマンドリストを取得
    }
}

// 描画準備（頂点バッファ、マテリアル、テクスチャのセット）
void Model::PreDraw(ID3D12GraphicsCommandList* commandList) {
    // 頂点バッファをセット
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

    // プリミティブトポロジーをセット
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

// 描画実行
void Model::Draw(ID3D12GraphicsCommandList* commandList, uint32_t instanceCount) {
    // 描画コマンド
    commandList->DrawInstanced(
        static_cast<UINT>(modelData_.vertices.size()),
        instanceCount,
        0,
        0
    );
}