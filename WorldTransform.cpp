#include "WorldTransform.h"
#include "ResourcesUtility.h"
#include "Camera.h"

void WorldTransform::Initialize(ID3D12Device* device) {
    // WVP用定数バッファの作成
    wvpResource = CreateBufferResource(device, Align256(sizeof(TransformationMatrix)));

    // マッピング（ずっとマップしたまま）
    wvpResource.Get()->Map(0, nullptr, reinterpret_cast<void**>(&mappedData));

    // 初期値設定
    mappedData->WVP = Matrix4x4::MakeIdentity4x4();
    mappedData->World = Matrix4x4::MakeIdentity4x4();
}

void WorldTransform::UpdateMatrix(const Camera& camera) {
    // ワールド行列を計算
    Matrix4x4 worldMatrix = MakeAffineMatrix(scale, rotate, translate);

    // カメラからViewProjection行列を取得
    Matrix4x4 viewProjectionMatrix = camera.GetViewProjectionMatrix();

    // WVP行列を計算
    Matrix4x4 wvpMatrix = Matrix4x4::Multiply(worldMatrix, viewProjectionMatrix);

    // 定数バッファに書き込み
    mappedData->World = worldMatrix;
    mappedData->WVP = wvpMatrix;
}