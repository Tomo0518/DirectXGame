#pragma once
#include "TomoEngine.h"

struct Transform {
    Vector3 scale = { 1.0f, 1.0f, 1.0f };
    Vector3 rotate = { 0.0f, 0.0f, 0.0f };
	Vector3 translate = { 0.0f, 0.0f, 0.0f };
};

//class WorldTransform {
//public:
//    void Initialize();
//    void UpdateMatrix(); // 行列を更新
//
//    // トランスフォーム
//    Vector3 scale = { 1.0f, 1.0f, 1.0f };
//    Vector3 rotation = { 0.0f, 0.0f, 0.0f };
//    Vector3 translation = { 0.0f, 0.0f, 0.0f };
//
//    // 定数バッファ
//    ResourceObject constantBuffer;
//    TransformationMatrix* mappedData = nullptr;
//
//    Matrix4x4 matWorld; // ワールド行列
//};