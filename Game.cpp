#include "Game.h"
#include "GraphicsCore.h"
#include "CommandContext.h" // GraphicsContextを使う
#include "InputManager.h"
#include <numbers>
#include <format>
#include "TextureManager.h"
#include "Sphere.h"
#include "Camera.h"
#include "ModelData.h"

inline InputManager& Input() { return *InputManager::GetInstance(); }

void Game::Initialize() {
	ID3D12Device* device = GraphicsCore::GetInstance()->GetDevice();

	// ==================================
	// 1. グラフィックスパイプライン初期化
	// ==================================
	m_pipeline = std::make_unique<GraphicsPipeline>();
	m_pipeline->Initialize();

	// ==================================
	// 2. 汎用リソース生成 (定数バッファ等)
	// ==================================
    // WVPリソース
    m_wvpResource = CreateBufferResource(device, Align256(sizeof(TransformationMatrix)));
    TransformationMatrix* wvpData = nullptr;
    m_wvpResource.Get()->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));
    wvpData->WVP = Matrix4x4::MakeIdentity4x4();
    wvpData->World = Matrix4x4::MakeIdentity4x4();

    // マテリアルリソース
    m_materialResource = CreateBufferResource(device, Align256(sizeof(Material)));
    Material* materialData = nullptr;
    m_materialResource.Get()->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
    materialData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    materialData->enableLighting = true;
    materialData->uvTransform = Matrix4x4::MakeIdentity4x4();

    // 平行光源リソース
    m_directionalLightResource = CreateBufferResource(device, Align256(sizeof(DirectionalLight)));
    m_directionalLightResource.Get()->Map(0, nullptr, reinterpret_cast<void**>(&lightData_));
    lightData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    lightData_->direction = { 0.0f, -0.85f, 0.53f };
    lightData_->intensity = 2.6f;

    // Sprite用リソース
    ResourceObject vertexResourceSprite = CreateBufferResource(device, sizeof(VertexData) * 6);
    m_vertexBufferViewSprite.BufferLocation = vertexResourceSprite.Get()->GetGPUVirtualAddress();
    m_vertexBufferViewSprite.SizeInBytes = sizeof(VertexData) * 6;
    m_vertexBufferViewSprite.StrideInBytes = sizeof(VertexData);

    VertexData* vertexDataSprite = nullptr;
    vertexResourceSprite.Get()->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSprite));
    // (頂点データ設定は長いので省略せずそのまま残します)
    vertexDataSprite[0].position = { 0.0f, 360.0f, 0.0f, 1.0f }; vertexDataSprite[0].texcoord = { 0.0f, 1.0f };
    vertexDataSprite[1].position = { 0.0f, 0.0f, 0.0f, 1.0f };   vertexDataSprite[1].texcoord = { 0.0f, 0.0f };
    vertexDataSprite[2].position = { 640.0f, 360.0f, 0.0f, 1.0f }; vertexDataSprite[2].texcoord = { 1.0f, 1.0f };
    vertexDataSprite[3].position = { 0.0f, 0.0f, 0.0f, 1.0f };   vertexDataSprite[3].texcoord = { 0.0f, 0.0f };
    vertexDataSprite[4].position = { 640.0f, 0.0f, 0.0f, 1.0f }; vertexDataSprite[4].texcoord = { 1.0f, 0.0f };
    vertexDataSprite[5].position = { 640.0f, 360.0f, 0.0f, 1.0f }; vertexDataSprite[5].texcoord = { 1.0f, 1.0f };

    m_transformationMatrixResourceSprite = CreateBufferResource(device, Align256(sizeof(Matrix4x4)));
    Matrix4x4* transformationMatrixDataSprite = nullptr;
    m_transformationMatrixResourceSprite.Get()->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixDataSprite));
    *transformationMatrixDataSprite = Matrix4x4::MakeIdentity4x4();

    // ==================================
    // Sphere用のResourceの生成
    // ==================================
    //m_sphere = std::make_unique<Sphere>();
    //m_sphere->Initialize();

    // ==================================
    // 5. SRVヒープ & ImGui 初期化
    // ==================================
    // SRV用ヒープの作成
    m_srvHeap.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);

    // ImGui用にディスクリプタを1つ確保 (自動的に先頭が使われる)
    auto [imguiCpuHandle, imguiGpuHandle] = m_srvHeap.Allocate();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(Window::GetInstance()->GetHwnd());
    // 確保したハンドルを使ってImGui初期化
    ImGui_ImplDX12_Init(
        device,
        3,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        m_srvHeap.GetHeap(),
        imguiCpuHandle,
        imguiGpuHandle
    );

	// ==================================
	// パーティクル用のインスタンシングリソース生成
	// ==================================

	m_instancingResource = CreateBufferResource(
		device, sizeof(TransformationMatrix) * kNumInstance);

	m_instancingResource.Get()->Map(0, nullptr, reinterpret_cast<void**>(&instancingData_));

	for (uint32_t index = 0; index < kNumInstance; ++index) {
		instancingData_[index].WVP = Matrix4x4::MakeIdentity4x4();
		instancingData_[index].World = Matrix4x4::MakeIdentity4x4();
	}

	// ★ DescriptorHeapから自動割り当て
	auto [instancingSrvHandleCPU, instancingSrvHandleGPU] = m_srvHeap.Allocate();

	// StructuredBuffer用のSRV設定
	D3D12_SHADER_RESOURCE_VIEW_DESC instancingSrvDesc{};
	instancingSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
	instancingSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	instancingSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	instancingSrvDesc.Buffer.FirstElement = 0;
	instancingSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	instancingSrvDesc.Buffer.NumElements = kNumInstance;
	instancingSrvDesc.Buffer.StructureByteStride = sizeof(TransformationMatrix);

	// SRVを作成
	device->CreateShaderResourceView(
		m_instancingResource.Get(),
		&instancingSrvDesc,
		instancingSrvHandleCPU);

	// instancingSrvHandleGPUを保存しておく（描画時に使用）
	m_instancingSrvHandleGPU = instancingSrvHandleGPU;  // メンバ変数として保持

	for (uint32_t index = 0; index < kNumInstance; ++index) {
		transform_particles[index].scale = { 1.0f, 1.0f, 1.0f };
		transform_particles[index].rotate = { 0.0f, 0.0f, 0.0f };
		transform_particles[index].translate = { index * 0.1f,index * 0.1f,index * 0.1f };
	}

    // ==================================
    // 6. テクスチャ・モデル読み込み
    // ==================================
    // TextureManagerの初期化 (ヒープを渡す)
    TextureManager::GetInstance()->Initialize(&m_srvHeap);

    // コマンドリスト開始
    GraphicsContext& context = GraphicsContext::Begin(L"Load Models");
    ID3D12GraphicsCommandList* commandList = context.GetCommandList();

    // 1. uvCheckerテクスチャの読み込み
//    戻り値からGPUハンドルを取得して保持
    /*const Texture* uvCheckerTexture = TextureManager::GetInstance()->Load("resources/uvChecker.png", commandList);
    if (uvCheckerTexture) {
        m_uvCheckerGpuHandle = uvCheckerTexture->gpuHandle;
    }*/

	// モデルデータの初期化

	modelDataParticle_ = ModelData();
	// 頂点データの設定 (四角形2つで1枚の板ポリゴン)
	modelDataParticle_.vertices = {
		// 0番
		{{-0.5f, 0.5f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
		// 1番
		{{0.5f, 0.5f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
		// 2番
		{{-0.5f, -0.5f, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},
		// 3番
		{{0.5f, 0.5f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
		// 4番
		{{0.5f, -0.5f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},
		// 5番
		{{-0.5f, -0.5f, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},
	};
	modelDataParticle_.material.textureFilePath = "resources/uvChecker.png";


	// カメラの生成と初期化
	m_camera = new Camera();
	m_camera->Initialize();
	m_camera->SetTranslation({ 10.0f, 1.0f, -10.0f });
	m_camera->UpdateMatrix();

    // 2. Modelの生成
    m_modelPlayer_ = Model::CreateFromOBJ("resources/player", "player.obj", commandList);
    m_modelCube_ = Model::CreateFromOBJ("resources/cube", "cube.obj", commandList);
	m_modelFence_ = Model::CreateFromOBJ("resources/fence", "fence.obj", commandList);

	// 初期位置を設定
	m_modelPlayer_->GetWorldTransform().translation_ = { 10.0f, 10.0f, 0.0f };
	m_modelCube_->GetWorldTransform().translation_ = { -3.0f, 0.0f, 0.0f };
	m_modelFence_->GetWorldTransform().translation_ = { 0.0f, 0.0f, 0.0f };

	// ==================================
// 7. マップチップ用ブロック生成
// ==================================
	mapChipField_ = std::make_unique<MapChipField>();
	mapChipField_->LoadMapChipCsv("./resources/mapChip/blocks.csv");
	GenerateBlocks();

	Vector3 playerPositon = mapChipField_->GetMapChipPositionByIndex(3, 14);
	m_player.Initialize(m_modelPlayer_, m_camera, playerPositon);
	m_player.SetMapChipField(mapChipField_.get());

    // 転送コマンドの実行と待機
    context.Finish(true);

}

void Game::Update() {
	Input().Update();

	// ImGui更新
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// ゲームロジック
	/*if (Input().PushKey(VK_UP)) m_transform.translate.z += 0.1f;
	if (Input().PushKey(VK_DOWN)) m_transform.translate.z -= 0.1f;*/
	if (Input().PushKey(VK_LEFT)) m_modelPlayer_->GetWorldTransform().rotation_.y += 0.1f;
	if (Input().PushKey(VK_RIGHT)) m_modelPlayer_->GetWorldTransform().rotation_.y -= 0.1f;

	//m_camera->UpdateDebugCameraMove(1.0f);
	m_camera->UpdateMatrix();

	m_modelPlayer_->Update(*m_camera);
	m_modelCube_->Update(*m_camera);
	m_modelFence_->Update(*m_camera);

	// プレイヤー更新
	m_player.Update();

	for (uint32_t index = 0; index < kNumInstance; ++index) {
		Matrix4x4 worldMatrix = MakeAffineMatrix(
			transform_particles[index].scale,
			transform_particles[index].rotate,
			transform_particles[index].translate);
		Matrix4x4 viewProj = m_camera->GetViewProjectionMatrix();
		Matrix4x4 wvpMatrix = worldMatrix * viewProj;
		instancingData_[index].World = worldMatrix;
		instancingData_[index].WVP = wvpMatrix;
	}

	for (std::vector<WorldTransform*>& worldTransFomBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransFomBlockLine) {
			if (!worldTransformBlock) {
				continue;
			}
			worldTransformBlock->UpdateMatrix(*m_camera);
		}
	}

	// ImGui UI構成
	ImGui::Begin("Debug");
	ImGui::ColorEdit4("Material Color", m_materialColor);

	// 光源の調整
	ImGui::Text("Light Control");
	ImGui::ColorEdit4("Light Color", &lightData_->color.x);
	ImGui::DragFloat3("Light Direction", &lightData_->direction.x, 0.1f);
	ImGui::DragFloat("Light Intensity", &lightData_->intensity, 0.1f, 0.0f, 10.0f);

	// カメラのデバッグ
	ImGui::Text("Camera Control");
	ImGui::DragFloat3("Camera Position", &m_camera->GetTranslation().x, 0.1f);
	ImGui::DragFloat3("Camera Rotation", &m_camera->GetRotation().x, 0.1f);

	m_modelPlayer_->ShowDebugUI("Player Model");
	m_modelFence_->ShowDebugUI("Fence Model");

	ImGui::Text("Player Texture Handle: %llu", m_modelPlayer_->GetTextureSrvHandleGPU().ptr);
	ImGui::Text("Fence Texture Handle: %llu", m_modelFence_->GetTextureSrvHandleGPU().ptr);

	ImGui::End();
	ImGui::Render();


	// directionalLightData のdirectonを正規化して書き戻す
	lightData_->direction = Vector3::Normalize(lightData_->direction);
}

void Game::Render() {
	// 1. コンテキスト取得 (コマンドリストのリセット等を含む)
	GraphicsContext& context = GraphicsContext::Begin(L"Scene Render");

	// 2. 描画ターゲットの取得と遷移バリア
	ColorBuffer& backBuffer = GraphicsCore::GetInstance()->GetBackBuffer();
	DepthBuffer& depthBuffer = GraphicsCore::GetInstance()->GetDepthBuffer();

	// PRESENT -> RENDER_TARGET に遷移し、即座にフラッシュ
	context.TransitionResource(backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
	context.TransitionResource(depthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);

	// 3. 生のコマンドリストを取得して描画コマンドを積む
	ID3D12GraphicsCommandList* commandList = context.GetCommandList();

	// 画面クリア
	commandList->ClearRenderTargetView(backBuffer.GetRTV(), backBuffer.GetClearColor(), 0, nullptr);
	commandList->ClearDepthStencilView(depthBuffer.GetDSV(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// レンダーターゲット設定
	D3D12_CPU_DESCRIPTOR_HANDLE rtv = backBuffer.GetRTV();
	D3D12_CPU_DESCRIPTOR_HANDLE dsv = depthBuffer.GetDSV();
	commandList->OMSetRenderTargets(1, &rtv, false, &dsv);

	// ビューポート・シザー
	D3D12_VIEWPORT viewport = { 0.0f, 0.0f, 1280.0f, 720.0f, 0.0f, 1.0f };
	D3D12_RECT scissor = { 0, 0, 1280, 720 };
	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissor);

	// パイプライン設定 
	m_pipeline->SetState(commandList,PipelineType::Object3D);

	// ヒープ設定 (ImGui用含む)
    ID3D12DescriptorHeap* heaps[] = { m_srvHeap.GetHeap() };
    commandList->SetDescriptorHeaps(1, heaps);

	// ライト用CBV (Root Parameter Index [3])
	commandList->SetGraphicsRootConstantBufferView(3, m_directionalLightResource.Get()->GetGPUVirtualAddress());

	// テクスチャSRV (Root Parameter Index [2])
    D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU = m_srvHeap.GetHeap()->GetGPUDescriptorHandleForHeapStart();

	// ImGui用に1つずらす
	uint32_t descriptorSize = GraphicsCore::GetInstance()->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	textureSrvHandleGPU.ptr += descriptorSize;
	commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU);


	///
	/// ↓描画処理ここから
	///

	// ブロックの描画
	for (std::vector<WorldTransform*> worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			if (!worldTransformBlock) {
				continue;
			}

			m_modelCube_->Draw(commandList, *worldTransformBlock);
		}
	}

	// 2. モデルの描画
	//m_modelPlayer_->PreDraw(commandList);
	m_modelPlayer_->Draw(commandList);
	m_modelCube_->Draw(commandList);

	m_modelCube_->GetWorldTransform().translation_.x += 1.0f;
	m_modelCube_->Draw(commandList);
	m_modelCube_->GetWorldTransform().translation_.x -= 1.0f;

	m_modelFence_->Draw(commandList);

	m_player.Draw(commandList);

	// ===================================
   // Particle描画
   // ===================================
	m_pipeline->SetState(commandList, PipelineType::Particle);  // パイプライン切り替え

	// ライト再設定（ルートシグネチャが変わったため）
	commandList->SetGraphicsRootConstantBufferView(3, m_directionalLightResource.Get()->GetGPUVirtualAddress());

	// マテリアル設定
	commandList->SetGraphicsRootConstantBufferView(0, m_materialResource.Get()->GetGPUVirtualAddress());

	// StructuredBuffer設定（Root Parameter 1）
	commandList->SetGraphicsRootDescriptorTable(1, m_instancingSrvHandleGPU);

	// テクスチャ設定（Root Parameter 2）
	// uvCheckerのハンドルを取得して設定する必要がある
	const Texture* uvCheckerTexture = TextureManager::GetInstance()->Load("resources/uvChecker.png", commandList);
	if (uvCheckerTexture) {
		commandList->SetGraphicsRootDescriptorTable(2, uvCheckerTexture->gpuHandle);
	}

	// Particle描画
	commandList->DrawInstanced(UINT(modelDataParticle_.vertices.size()), kNumInstance, 0, 0);

	// ImGui描画
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

	context.TransitionResource(backBuffer, D3D12_RESOURCE_STATE_PRESENT, true);
	context.Finish();
}

void Game::Shutdown() {
	// GPU処理の完了を待機
	GraphicsCore::GetInstance()->GetCommandListManager().GetGraphicsQueue().WaitForIdle();

	// 生成したモデルの解放
	delete m_modelCube_;
	m_modelCube_ = nullptr;

	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	// リソースはComPtrやResourceObjectデストラクタで解放される
}

void Game::GenerateBlocks() {
	// 要素数
	uint32_t numBlockVirtical = mapChipField_->GetNumBlockVertical();
	uint32_t numBlockHorizontal = mapChipField_->GetNumBlockHorizontal();

	worldTransformBlocks_.resize(numBlockVirtical);
	for (uint32_t i = 0; i < numBlockVirtical; ++i) {
		worldTransformBlocks_[i].resize(numBlockHorizontal);
	}

	// ブロックの生成
	for (uint32_t vp = 0; vp < numBlockVirtical; ++vp) {
		for (uint32_t hp = 0; hp < numBlockHorizontal; ++hp) {
			MapChipType mapChipType = mapChipField_->GetMapChipTypeByIndex(hp, vp);
			if (mapChipType == MapChipType::kBlank) {
				// ブロック無し
				worldTransformBlocks_[vp][hp] = nullptr;
				continue;
			}
			// ブロック有り
			worldTransformBlocks_[vp][hp] = new WorldTransform();
			worldTransformBlocks_[vp][hp]->Initialize(GraphicsCore::GetInstance()->GetDevice());
			Vector3 blockPosition = mapChipField_->GetMapChipPositionByIndex(hp, vp);
			worldTransformBlocks_[vp][hp]->translation_ = blockPosition;
		}
	}
}