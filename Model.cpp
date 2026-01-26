#include "Model.h"
#include "GraphicsCore.h"
#include "ResourcesUtility.h"
#include "DescriptorUtility.h"
#include "TextureManager.h"
#include <cassert>

// ===================================
// ファクトリメソッド
// ===================================
Model* Model::CreateFromOBJ(
    const std::string& directoryPath,
    const std::string& filename,
    ID3D12GraphicsCommandList* commandList)
{
    Model* model = new Model();

    // OBJファイルを読み込む
    ModelData modelData = LoadObjFile(directoryPath, filename);

    // モデルを初期化
    model->Initialize(modelData, commandList);

    return model;
}

// ===================================
// 初期化
// ===================================
void Model::Initialize(
    const ModelData& modelData,
    ID3D12GraphicsCommandList* commandList)
{
    modelData_ = modelData;

    ID3D12Device* device = GraphicsCore::GetInstance()->GetDevice();
    assert(device && "Device is null");

    // ===================================
    // 頂点バッファの生成
    // ===================================
    size_t vertexBufferSize = sizeof(VertexData) * modelData_.vertices.size();
    vertexBuffer_ = CreateBufferResource(device, vertexBufferSize);

    // 頂点バッファビューの作成
    vertexBufferView_.BufferLocation = vertexBuffer_.Get()->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = static_cast<UINT>(vertexBufferSize);
    vertexBufferView_.StrideInBytes = sizeof(VertexData);

    // 頂点データをバッファにコピー
    VertexData* vertexData = nullptr;
    vertexBuffer_.Get()->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
    std::memcpy(vertexData, modelData_.vertices.data(), vertexBufferSize);
   
    // マップしたままにする（パフォーマンス向上）

    // ===================================
    // マテリアルリソースの生成
    // ===================================
    materialResource_ = CreateBufferResource(device, Align256(sizeof(Material)));
    materialResource_.Get()->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));

    // デフォルト値を設定
    materialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    materialData_->enableLighting = true;
    materialData_->uvTransform = Matrix4x4::MakeIdentity4x4();
    // マップしたままにする

    // ===================================
    // テクスチャの読み込み（TextureManager使用）
    // ===================================
    if (!modelData_.material.textureFilePath.empty()) {
        const Texture* tex = TextureManager::GetInstance()->Load(
            modelData_.material.textureFilePath,
            commandList);

        if (tex) {
            textureSrvHandleGPU_ = tex->gpuHandle;
        }
    }
}

void Model::Draw(
    ID3D12GraphicsCommandList* commandList,
    const WorldTransform& worldTransform,
    uint32_t rootParameterIndexWVP,
    uint32_t rootParameterIndexMaterial,
    uint32_t rootParameterIndexTexture)
{
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    commandList->SetGraphicsRootConstantBufferView(
        rootParameterIndexMaterial,
        materialResource_.Get()->GetGPUVirtualAddress());

    // 外部のWorldTransformを直接使う（コピーしない）
    commandList->SetGraphicsRootConstantBufferView(
        rootParameterIndexWVP,
        worldTransform.GetGPUVirtualAddress());

    if (textureResource_.Get() || textureSrvHandleGPU_.ptr != 0) {
        commandList->SetGraphicsRootDescriptorTable(
            rootParameterIndexTexture,
            textureSrvHandleGPU_);
    }

    commandList->DrawInstanced(
        static_cast<UINT>(modelData_.vertices.size()), 1, 0, 0);
}

void Model::ShowDebugUI(std::string tag, WorldTransform& worldTransform) {
    if (ImGui::TreeNode(tag.c_str())) {
        ImGui::DragFloat3("Position", &worldTransform.translation_.x, 0.1f);
        ImGui::DragFloat3("Rotation", &worldTransform.rotation_.x, 0.1f);
        ImGui::DragFloat3("Scale", &worldTransform.scale_.x, 0.1f);

        //// マテリアル情報の表示
        ImGui::Separator();
        ImGui::Text("Material:");
        ImGui::ColorEdit4("Color", reinterpret_cast<float*>(&materialData_->color));
        ImGui::Checkbox("Enable Lighting", reinterpret_cast<bool*>(&materialData_->enableLighting));
        ImGui::TreePop();
    }
}