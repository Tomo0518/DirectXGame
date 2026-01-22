#pragma once
#include "Math.h"
#include "ResourceObject.h"
#include "TransformationMatrix.h"

class Camera; // 前方宣言

class WorldTransform {
public:
    // トランスフォームデータ
    Vector3 scale = { 1.0f, 1.0f, 1.0f };
    Vector3 rotate = { 0.0f, 0.0f, 0.0f };
    Vector3 translate = { 0.0f, 0.0f, 0.0f };

    // 定数バッファ
    ResourceObject wvpResource;
    TransformationMatrix* mappedData = nullptr;

    void Initialize(ID3D12Device* device);

	// Cameraを受け取って行列更新
    void UpdateMatrix(const Camera& camera);

    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const {
        return wvpResource.Get()->GetGPUVirtualAddress();
    }
};