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
    Model() = default;

    /// <summary>
    /// OBJファイルからモデルを生成（テクスチャはTextureManagerで管理）
    /// </summary>
    static Model* CreateFromOBJ(
        const std::string& directoryPath,
        const std::string& filename,
        ID3D12GraphicsCommandList* commandList);

    /// <summary>
    /// 描画
    /// </summary>
    /// <param name="worldTransform">ワールドトランスフォーム</param>
    /// <param name="camera">カメラ</param>
    /// <param name="objectColor">オブジェクトカラー</param>
    void Draw(
        ID3D12GraphicsCommandList* commandList,
        const WorldTransform& worldTransform,
        uint32_t rootParameterIndexWVP = 1,
        uint32_t rootParameterIndexMaterial = 0,
        uint32_t rootParameterIndexTexture = 2);

    /// <summary>
	/// デバッグ用GUI表示
    /// </summary>
    /// <param name="tag">表示するタグ(ツリーの名前)</param>
    /// <param name="worldTransform">描画に使用するWorldTransform</param>
    void ShowDebugUI(std::string tag, WorldTransform& worldTransform);

    const ModelData& GetModelData() const { return modelData_; }  // モデルデータへのアクセス
    Material* GetMaterialData() { return materialData_; }  // マテリアルデータへのアクセス
    const Material* GetMaterialData() const { return materialData_; } 
	D3D12_GPU_DESCRIPTOR_HANDLE GetTextureSrvHandleGPU() const { return textureSrvHandleGPU_; } // テクスチャSRVハンドルへのアクセス

private:

    /// <summary>
    /// モデルの初期化
    /// </summary>
    void Initialize(
        const ModelData& modelData,
        ID3D12GraphicsCommandList* commandList);
	
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

    // アップロード用中間リソース
    std::vector<ResourceObject> intermediateResources_;
};