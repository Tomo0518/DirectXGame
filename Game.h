#pragma once
#include "TomoEngine.h"
#include <memory>
#include <vector>
#include "GraphicsPipeline.h"
#include "Model.h"
#include "MapChipField.h"
#include "Player.h"
#include "Skydome.h"
#include "CameraController.h"

// 前方宣言
class GraphicsContext;
class Sphere;
class CameraController;
class Camera;
struct ModelData;

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

	// マップチップ用ブロック生成
    void GenerateBlocks();

private:

	// グラフィックスパイプライン
    std::unique_ptr<GraphicsPipeline> m_pipeline;

    // テクスチャリソース
    D3D12_GPU_DESCRIPTOR_HANDLE m_uvCheckerGpuHandle = {};
    D3D12_GPU_DESCRIPTOR_HANDLE m_instancingSrvHandleGPU{};
    Microsoft::WRL::ComPtr<ID3D12Resource> m_instancingResource; 

    //Microsoft::WRL::ComPtr<ID3D12Resource> m_textureResource2;

    DescriptorHeap m_srvHeap;

    // モデル・スプライト・球体などのリソース
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
    Model* modelCube_ = nullptr;
    Model* modelPlayer_ = nullptr;
    Model* modelSkydome_ = nullptr;
	Model* modelFence_ = nullptr;
    

	bool isDebugCameraActive_ = false;
    Camera* camera_ = nullptr;
    std::unique_ptr<Camera> debugCamera_ = nullptr;
    std::unique_ptr<CameraController> cameraController_ = nullptr;

    //std::unique_ptr<Sphere> m_sphere;

    DirectionalLight* lightData_{};

	// ===================================
    // プレイヤー
	// ===================================
	Player player_;

    // ==================================
    // スカイドーム
    // ==================================
    std::unique_ptr<Skydome> skydome_ = nullptr;

	// ===================================
    // ゲーム内変数
	// ===================================
    Transform m_transform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
    Transform m_transformSprite{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
   // Transform m_cameraTransform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,-5.0f} };
    Transform m_uvTransformSprite{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };

    bool useMonsterBall_ = true;
    float materialColor_[4] = { 1.0f,1.0f,1.0f,1.0f };

    // モデルデータ
    //ModelData m_objModelData;

    static const uint32_t kNumInstance = 10;
    Transform transform_particles[kNumInstance];
    TransformationMatrix* instancingData_ = nullptr;

    ModelData modelDataParticle_;

	// ===================================
    // マップチップ用ブロック
	// ===================================
	std::unique_ptr<MapChipField> mapChipField_;
    std::vector<std::vector<WorldTransform*>> worldTransformBlocks_;
};