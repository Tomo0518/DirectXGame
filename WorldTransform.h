#pragma once
#include "Math.h"
#include "ResourceObject.h"
#include "TransformationMatrix.h"

class Camera; // 前方宣言

class WorldTransform {
public:
    // トランスフォームデータ
    Vector3 scale_ = { 1.0f, 1.0f, 1.0f };
    Vector3 rotation_ = { 0.0f, 0.0f, 0.0f };
    Vector3 translation_ = { 0.0f, 0.0f, 0.0f };


    WorldTransform() = default;
    ~WorldTransform() = default;

    // 定数バッファ
    ResourceObject wvpResource;
    TransformationMatrix* mappedData = nullptr;

    // ローカル → ワールド変換行列
    Matrix4x4 matWorld_;
    // 親となるワールド変換へのポインタ
    const WorldTransform* parent_ = nullptr;

    void Initialize(ID3D12Device* device);

	// Cameraを受け取って行列更新
    void UpdateMatrix(const Camera& camera);

    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const {
        return wvpResource.Get()->GetGPUVirtualAddress();
    }
};