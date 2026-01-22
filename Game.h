#pragma once
#include "TomoEngine.h"
#include <memory>
#include <vector>
#include "GraphicsPipeline.h"
#include "Model.h"


// 前方宣言
class GraphicsContext;
class Sphere;
class Camera;

class Game {
public:
    // コンストラクタ・デストラクタ
    Game() = default;
    ~Game() = default;

    // ゲームの初期化
    void Initialize();

    // ゲームの更新
    void Update();

    // ゲームの描画
    void Render();

    // 終了処理
    void Shutdown();

private:

	// グラフィックスパイプライン
    std::unique_ptr<GraphicsPipeline> m_pipeline;

    // テクスチャリソース
    D3D12_GPU_DESCRIPTOR_HANDLE m_uvCheckerGpuHandle = {};
    //Microsoft::WRL::ComPtr<ID3D12Resource> m_textureResource2;

    DescriptorHeap m_srvHeap;

    // モデル・スプライト・球体などのリソース
    // ※ 本来は Model クラス等に隠蔽すべきですが、
    //    移行フェーズのため main.cpp の変数をここに移動させます
    ResourceObject m_vertexResource;
    ResourceObject m_vertexResourceSprite;
    ResourceObject m_indexResource;
    ResourceObject m_materialResource;
    ResourceObject m_materialResourceSprite;
    ResourceObject m_wvpResource;
    ResourceObject m_wvpResourceSphere;
    ResourceObject m_transformationMatrixResourceSprite;
    ResourceObject m_directionalLightResource;
   // ResourceObject m_objVertexResource;


    // ビュー関連
   // D3D12_VERTEX_BUFFER_VIEW m_objVertexBufferView{};
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferViewSprite{};
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferViewSphere{};
    D3D12_INDEX_BUFFER_VIEW m_indexBufferViewSprite{};


    // ===================================
	// オブジェクト
	// ===================================
    Model* m_modelCube_ = nullptr;
    Model* m_modelPlayer_ = nullptr;
    
    Camera* m_camera = nullptr;

    //std::unique_ptr<Sphere> m_sphere;

    DirectionalLight* lightData_{};

	// ===================================
    // ゲーム内変数
	// ===================================
    Transform m_transform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
    Transform m_transformSprite{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
   // Transform m_cameraTransform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,-5.0f} };
    Transform m_uvTransformSprite{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };

    bool m_useMonsterBall = true;
    float m_materialColor[4] = { 1.0f,1.0f,1.0f,1.0f };

    // モデルデータ
    //ModelData m_objModelData;


    // ImGui用ディスクリプタヒープ (GraphicsCore側で管理するまでの暫定)
    //Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvDescriptorHeap;

};