#include "Model.h"
#include "GraphicsCore.h"
#include "ResourcesUtility.h"
#include "DescriptorUtility.h"
#include <cassert>
#include "TextureManager.h"

// 静的ファクトリメソッド
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

Model* Model::CreateFromOBJ(const std::string& directoryPath, const std::string& filename, ID3D12GraphicsCommandList* commandList) {
    Model* model = new Model();
    ModelData modelData = LoadObjFile(directoryPath, filename);
    model->Initialize(modelData, commandList);
    return model;
}


// モデルの初期化
void Model::Initialize(
    const ModelData& modelData,
    ID3D12GraphicsCommandList* commandList,
    D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle,
    D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle)
{
    modelData_ = modelData;

    ID3D12Device* device = GraphicsCore::GetInstance()->GetDevice();
    assert(device);

    // ===================================
    // 1. 頂点バッファの生成
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
    // Map したままにする

    // ===================================
    // 2. マテリアルリソースの生成
    // ===================================
    materialResource = CreateBufferResource(device, Align256(sizeof(Material)));

    // マテリアルデータを書き込む
    materialResource.Get()->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
    materialData->color = { 1.0f, 1.0f, 1.0f, 1.0f }; // デフォルトは白
    materialData->enableLighting = true;
    materialData->uvTransform = Matrix4x4::MakeIdentity4x4();

    // ===================================
    // 3. テクスチャの読み込み・転送・SRV生成
    // ===================================
    if (!modelData_.material.textureFilePath.empty()) {
        // テクスチャファイルを読み込む
        DirectX::ScratchImage mipImages = LoadTexture(modelData_.material.textureFilePath);
        const DirectX::TexMetadata& metadata = mipImages.GetMetadata();

        // GPUリソース（Default Heap）を生成
        textureResource = CreateTextureResource(device, metadata);

        // テクスチャデータをアップロード（中間リソースを受け取って保持する）
        ResourceObject intermediate = UploadTextureData(textureResource, mipImages, device, commandList);
        intermediateResources_.push_back(intermediate);

        // SRVの設定
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = metadata.format;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = static_cast<UINT>(metadata.mipLevels);

        // 渡されたハンドル位置にSRVを作成
        device->CreateShaderResourceView(textureResource.Get(), &srvDesc, srvCpuHandle);

        // 描画時に使うGPUハンドルを保存しておく
        textureSrvHandleGPU = srvGpuHandle;
    }
}


void Model::Initialize(const ModelData& modelData, ID3D12GraphicsCommandList* commandList) {
    modelData_ = modelData;

    ID3D12Device* device = GraphicsCore::GetInstance()->GetDevice();
    assert(device);

    // ===================================
    // 1. 頂点バッファの生成
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
    // Map したままにする

    // ===================================
    // 2. マテリアルリソースの生成
    // ===================================
    materialResource = CreateBufferResource(device, Align256(sizeof(Material)));

    // マテリアルデータを書き込む
    materialResource.Get()->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
    materialData->color = { 1.0f, 1.0f, 1.0f, 1.0f }; // デフォルトは白
    materialData->enableLighting = true;
    materialData->uvTransform = Matrix4x4::MakeIdentity4x4();

    // テクスチャ読み込み部分
    if (!modelData_.material.textureFilePath.empty()) {
        // ★ TextureManagerに頼むだけ！
        const Texture* tex = TextureManager::GetInstance()->Load(modelData_.material.textureFilePath, commandList);

        // ハンドルを保持
        if (tex) {
            textureSrvHandleGPU = tex->gpuHandle;
            // resourceメンバ変数は不要になる（Managerが持ってるので）
        }
    }
}


// 描画準備
void Model::PreDraw(ID3D12GraphicsCommandList* commandList) {
    // 頂点バッファをセット
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

    // マテリアル定数バッファをセット (RootIndex 0と仮定)
    commandList->SetGraphicsRootConstantBufferView(0, materialResource.Get()->GetGPUVirtualAddress());

    // プリミティブトポロジーをセット
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

// 描画実行
void Model::Draw(ID3D12GraphicsCommandList* commandList, uint32_t instanceCount, uint32_t rootParameterIndex) {
    // テクスチャがある場合、DescriptorTableをセット
    if (textureResource) {
        commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, textureSrvHandleGPU);
    }

    // 描画コマンド
    commandList->DrawInstanced(
        static_cast<UINT>(modelData_.vertices.size()),
        instanceCount,
        0,
        0
    );
}