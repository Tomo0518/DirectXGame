#pragma once
#include "ModelData.h"
#include "ResourceObject.h"
#include "WorldTransform.h"
#include "Camera.h"
#include <d3d12.h>
#include <string>
#include <vector>

class Model {
public:
    /// <summary>
    /// OBJファイルからモデルを生成（テクスチャはTextureManagerで管理）
    /// </summary>
    static Model* CreateFromOBJ(
        const std::string& directoryPath,
        const std::string& filename,
        ID3D12GraphicsCommandList* commandList);

    /// <summary>
    /// OBJファイルからモデルを生成（レガシー版：手動でSRVハンドルを指定）
    /// </summary>
    static Model* CreateFromOBJ(
        const std::string& directoryPath,
        const std::string& filename,
        ID3D12GraphicsCommandList* commandList,
        D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle,
        D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle);

    /// <summary>
    /// モデルの更新（Transform行列の計算）
    /// </summary>
    /// <param name="camera">カメラ</param>
    void Update(const Camera& camera);

    /// <summary>
    /// モデルの描画
    /// </summary>
    /// <param name="commandList">コマンドリスト</param>
    /// <param name="rootParameterIndexWVP">WVPのルートパラメータインデックス</param>
    /// <param name="rootParameterIndexMaterial">マテリアルのルートパラメータインデックス</param>
    /// <param name="rootParameterIndexTexture">テクスチャのルートパラメータインデックス</param>
    void Draw(
        ID3D12GraphicsCommandList* commandList,
        uint32_t rootParameterIndexWVP = 1,
        uint32_t rootParameterIndexMaterial = 0,
        uint32_t rootParameterIndexTexture = 2);

    // WorldTransformへのアクセス
    WorldTransform& GetWorldTransform() { return worldTransform_; }
    const WorldTransform& GetWorldTransform() const { return worldTransform_; }

    // モデルデータへのアクセス
    const ModelData& GetModelData() const { return modelData_; }

    // マテリアルデータへのアクセス
    Material* GetMaterialData() { return materialData_; }
    const Material* GetMaterialData() const { return materialData_; }

private:
    Model() = default;

    /// <summary>
    /// モデルの初期化（TextureManager使用版）
    /// </summary>
    void Initialize(
        const ModelData& modelData,
        ID3D12GraphicsCommandList* commandList);

    /// <summary>
    /// モデルの初期化（レガシー版）
    /// </summary>
    void Initialize(
        const ModelData& modelData,
        ID3D12GraphicsCommandList* commandList,
        D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle,
        D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle);

private:
    // モデルデータ
    ModelData modelData_;

    // 頂点バッファ
    ResourceObject vertexBuffer_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

    // マテリアル
    ResourceObject materialResource_;
    Material* materialData_ = nullptr;

    // テクスチャ（TextureManager管理の場合は空）
    ResourceObject textureResource_;
    D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU_{};

    // Transform（各モデル固有）
    WorldTransform worldTransform_;

    // アップロード用中間リソース
    std::vector<ResourceObject> intermediateResources_;
};