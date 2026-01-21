//#pragma once
//#include "TomoEngine.h"
//#include <numbers>
//
//class Sphere {
//public:
//    void Initialize();
//    void Update();
//    void Draw(ID3D12GraphicsCommandList* commandList);
//
//	Sphere() = default;
//
//private:
//    void CreateVertexResource();
//    void CreateWVPResource();
//
//    // 定数
//    static const uint32_t kSubdivision = 16;
//    static const uint32_t kVertexCount = kSubdivision * kSubdivision * 6;
//
//    // リソース
//    ResourceObject vertexResource_;
//    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
//    ResourceObject wvpResource_;
//
//    // トランスフォーム
//    Transform transform_{ {1.0f,1.0f,1.0f}, {0.0f,0.0f,0.0f}, {0.0f,0.0f,0.0f} };
//};