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

Model* Model::CreateFromOBJ(
    const std::string& directoryPath,
    const std::string& filename,
    ID3D12GraphicsCommandList* commandList,
    D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle,
    D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle)
{
    Model* model = new Model();

    // OBJファイルを読み込む
    ModelData modelData = LoadObjFile(directoryPath, filename);

    // モデルを初期化
    model->Initialize(modelData, commandList, srvCpuHandle, srvGpuHandle);

    return model;
}

// ===================================
// 初期化（TextureManager使用版）
// ===================================

void Model::Initialize(
    const ModelData& modelData,
    ID3D12GraphicsCommandList* commandList)
{
    modelData_ = modelData;

    ID3D12Device* device = GraphicsCore::GetInstance()->GetDevice();
    assert(device && "Device is null");

    // ===================================
    // WorldTransformの初期化
    // ===================================
    worldTransform_.Initialize(device);

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

// ===================================
// 初期化（レガシー版：手動SRV管理）
// ===================================

void Model::Initialize(
    const ModelData& modelData,
    ID3D12GraphicsCommandList* commandList,
    D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle,
    D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle)
{
    modelData_ = modelData;

    ID3D12Device* device = GraphicsCore::GetInstance()->GetDevice();
    assert(device && "Device is null");

    // WorldTransformの初期化
    worldTransform_.Initialize(device);

    // 頂点バッファの生成（上と同じ）
    size_t vertexBufferSize = sizeof(VertexData) * modelData_.vertices.size();
    vertexBuffer_ = CreateBufferResource(device, vertexBufferSize);

    vertexBufferView_.BufferLocation = vertexBuffer_.Get()->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = static_cast<UINT>(vertexBufferSize);
    vertexBufferView_.StrideInBytes = sizeof(VertexData);

    VertexData* vertexData = nullptr;
    vertexBuffer_.Get()->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
    std::memcpy(vertexData, modelData_.vertices.data(), vertexBufferSize);

    // マテリアルリソースの生成（上と同じ）
    materialResource_ = CreateBufferResource(device, Align256(sizeof(Material)));
    materialResource_.Get()->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
    materialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    materialData_->enableLighting = true;
    materialData_->uvTransform = Matrix4x4::MakeIdentity4x4();

    // テクスチャの読み込み（手動管理版）
    if (!modelData_.material.textureFilePath.empty()) {
        DirectX::ScratchImage mipImages = LoadTexture(modelData_.material.textureFilePath);
        const DirectX::TexMetadata& metadata = mipImages.GetMetadata();

        textureResource_ = CreateTextureResource(device, metadata);

        ResourceObject intermediate = UploadTextureData(
            textureResource_.Get(),
            mipImages,
            device,
            commandList);
        intermediateResources_.push_back(intermediate);

        // SRVの作成
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = metadata.format;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = static_cast<UINT>(metadata.mipLevels);

        device->CreateShaderResourceView(textureResource_.Get().Get(), &srvDesc, srvCpuHandle);
        textureSrvHandleGPU_ = srvGpuHandle;
    }
}

// ===================================
// 更新処理
// ===================================

void Model::Update(const Camera& camera) {
    // WorldTransformの行列を更新
    worldTransform_.UpdateMatrix(camera);
}

// ===================================
// 描画処理
// ===================================

void Model::Draw(
    ID3D12GraphicsCommandList* commandList,
    uint32_t rootParameterIndexWVP,
    uint32_t rootParameterIndexMaterial,
    uint32_t rootParameterIndexTexture)
{
    // 頂点バッファをセット
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);

    // プリミティブトポロジーをセット
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // マテリアル定数バッファをセット
    commandList->SetGraphicsRootConstantBufferView(
        rootParameterIndexMaterial,
        materialResource_.Get()->GetGPUVirtualAddress());

    // WVP定数バッファをセット
    commandList->SetGraphicsRootConstantBufferView(
        rootParameterIndexWVP,
        worldTransform_.GetGPUVirtualAddress());

    // テクスチャをセット
    if (textureResource_.Get() || textureSrvHandleGPU_.ptr != 0) {
        commandList->SetGraphicsRootDescriptorTable(
            rootParameterIndexTexture,
            textureSrvHandleGPU_);
    }

    // 描画コマンド
    commandList->DrawInstanced(
        static_cast<UINT>(modelData_.vertices.size()),
        1,  // インスタンス数
        0,  // 開始頂点インデックス
        0); // 開始インスタンスインデックス
}