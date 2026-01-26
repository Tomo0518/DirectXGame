//#include "Sphere.h"
//#include "GraphicsCore.h"
//#include <cmath>
//
//void Sphere::Initialize() {
//    CreateVertexResource();
//    CreateWVPResource();
//}
//
//void Sphere::Update() {
//    // 行列更新
//    Matrix4x4 worldMatrix = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
//    // ※カメラ行列などはGame側から貰うか、共通の定数バッファを参照する設計が理想ですが、
//    //   一旦は簡易的にここで計算、あるいはIdentityにしておきます
//    Matrix4x4 wvpMatrix = worldMatrix; // 本来は View * Proj も掛ける
//
//    TransformationMatrix* wvpData = nullptr;
//    wvpResource_.Get()->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));
//    wvpData->World = worldMatrix;
//    wvpData->WVP = wvpMatrix;
//    wvpResource_.Get()->Unmap(0, nullptr);
//}
//
//void Sphere::Draw(ID3D12GraphicsCommandList* commandList) {
//    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
//    // ルートパラメータのインデックスは実際の構成に合わせて変更してください
//    commandList->SetGraphicsRootConstantBufferView(1, wvpResource_.Get()->GetGPUVirtualAddress());
//    commandList->DrawInstanced(kVertexCount, 1, 0, 0);
//}
//
//void Sphere::CreateVertexResource() {
//    ID3D12Device* device = GraphicsCore::GetInstance()->GetDevice();
//    vertexResource_ = CreateBufferResource(device, sizeof(VertexData) * kVertexCount);
//
//    vertexBufferView_.BufferLocation = vertexResource_.Get()->GetGPUVirtualAddress();
//    vertexBufferView_.SizeInBytes = sizeof(VertexData) * kVertexCount;
//    vertexBufferView_.StrideInBytes = sizeof(VertexData);
//
//    VertexData* vertexData = nullptr;
//    vertexResource_.Get()->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
//
//    // 頂点計算ロジック (Game.cppから移動)
//    const float kLonEvery = 2.0f * std::numbers::pi_v<float> / float(kSubdivision);
//    const float kLatEvery = std::numbers::pi_v<float> / float(kSubdivision);
//
//    // 緯度の方向に分割
//    for (uint32_t latIndex = 0; latIndex < kSubdivision; ++latIndex) {
//        float lat = -std::numbers::pi_v<float> / 2.0f + kLatEvery * latIndex; // theta
//
//        // 経度の方向に分割
//        for (uint32_t lonIndex = 0; lonIndex < kSubdivision; ++lonIndex) {
//            float lon = lonIndex * kLonEvery; // phi
//
//            // 書き込む頂点の先頭インデックス
//            uint32_t start = (latIndex * kSubdivision + lonIndex) * 6;
//
//            // 基準点a, b, c, dの計算に必要な角度
//            float lat_b = lat + kLatEvery;
//            float lon_c = lon + kLonEvery;
//
//            // UV座標の計算用
//            float u_curr = float(lonIndex) / float(kSubdivision);
//            float u_next = float(lonIndex + 1) / float(kSubdivision);
//            float v_curr = 1.0f - float(latIndex) / float(kSubdivision);
//            float v_next = 1.0f - float(latIndex + 1) / float(kSubdivision);
//
//            // 頂点a (左下)
//            vertexData[start].position.x = std::cos(lat) * std::cos(lon);
//            vertexData[start].position.y = std::sin(lat);
//            vertexData[start].position.z = std::cos(lat) * std::sin(lon);
//            vertexData[start].position.w = 1.0f;
//            vertexData[start].texcoord = { u_curr, v_curr };
//            vertexData[start].normal = { vertexData[start].position.x, vertexData[start].position.y, vertexData[start].position.z };
//
//            // 頂点b (
//            vertexData[start + 1].position.x = std::cos(lat_b) * std::cos(lon);
//            vertexData[start + 1].position.y = std::sin(lat_b);
//            vertexData[start + 1].position.z = std::cos(lat_b) * std::sin(lon);
//            vertexData[start + 1].position.w = 1.0f;
//            vertexData[start + 1].texcoord = { u_curr, v_next };
//            vertexData[start + 1].normal = { vertexData[start + 1].position.x, vertexData[start + 1].position.y, vertexData[start + 1].position.z };
//
//            // 頂点c (
//            vertexData[start + 2].position.x = std::cos(lat) * std::cos(lon_c);
//            vertexData[start + 2].position.y = std::sin(lat);
//            vertexData[start + 2].position.z = std::cos(lat) * std::sin(lon_c);
//            vertexData[start + 2].position.w = 1.0f;
//            vertexData[start + 2].texcoord = { u_next, v_curr };
//            vertexData[start + 2].normal = { vertexData[start + 2].position.x, vertexData[start + 2].position.y, vertexData[start + 2].position.z };
//
//            // 頂点d ( 2枚目の三角形用
//            vertexData[start + 3] = vertexData[start + 1]; // b
//            vertexData[start + 4].position.x = std::cos(lat_b) * std::cos(lon_c);
//            vertexData[start + 4].position.y = std::sin(lat_b);
//            vertexData[start + 4].position.z = std::cos(lat_b) * std::sin(lon_c);
//            vertexData[start + 4].position.w = 1.0f;
//            vertexData[start + 4].texcoord = { u_next, v_next };
//            vertexData[start + 4].normal = { vertexData[start + 4].position.x, vertexData[start + 4].position.y, vertexData[start + 4].position.z };
//            vertexData[start + 5] = vertexData[start + 2]; // c
//        }
//    }
//    vertexResource_.Get()->Unmap(0, nullptr);
//}
//
//void Sphere::CreateWVPResource() {
//    ID3D12Device* device = GraphicsCore::GetInstance()->GetDevice();
//    wvpResource_ = CreateBufferResource(device, Align256(sizeof(TransformationMatrix)));
//    Update(); // 初期値を書き込み
//}