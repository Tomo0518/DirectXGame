#include "Game.h"
#include "GraphicsCore.h"
#include "CommandContext.h" // GraphicsContextを使う
#include "InputManager.h"
#include <numbers>
#include <format>

// インライン関数などのヘルパーも必要ならここに移動、あるいはUtilityへ
inline InputManager& Input() { return *InputManager::GetInstance(); }

void Game::Initialize() {
	ID3D12Device* device = GraphicsCore::GetInstance()->GetDevice();

	// ==================================
	// DXCの初期化
	// ==================================
	HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_dxcUtils));
	assert(SUCCEEDED(hr));
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_dxcCompiler));
	assert(SUCCEEDED(hr));
	hr = m_dxcUtils->CreateDefaultIncludeHandler(&m_includeHandler);
	assert(SUCCEEDED(hr));

	// ==================================
	// RootSignature / PSO 生成
	// ==================================
	// (main.cpp にあったコードをそのまま移植)
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature = {};
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0;
	descriptorRange[0].NumDescriptors = 1;
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderRegister = 0;
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

	D3D12_ROOT_PARAMETER rootParameters[4] = {};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[0].Descriptor.ShaderRegister = 0;

	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters[1].Descriptor.ShaderRegister = 0;

	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
	rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);

	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[3].Descriptor.ShaderRegister = 1;

	descriptionRootSignature.pParameters = rootParameters;
	descriptionRootSignature.NumParameters = _countof(rootParameters);

	// シリアライズとRootSignature生成
	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
	hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr)) {
		Log(ConvertString(static_cast<const char*>(errorBlob->GetBufferPointer())));
		assert(false);
	}
	hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature));
	assert(SUCCEEDED(hr));

	// InputLayout
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
	inputElementDescs[0].SemanticName = "POSITION"; inputElementDescs[0].SemanticIndex = 0; inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT; inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[1].SemanticName = "TEXCOORD"; inputElementDescs[1].SemanticIndex = 0; inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT; inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[2].SemanticName = "NORMAL";   inputElementDescs[2].SemanticIndex = 0; inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;    inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);

	// BlendState
	D3D12_BLEND_DESC blendDesc = {};
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;

	// RasterizerState
	D3D12_RASTERIZER_DESC rasterizerDesc = {};
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	// Shader Compile
	Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = CompileShader(L"Object3D.VS.hlsl", L"vs_6_0", m_dxcUtils.Get(), m_dxcCompiler.Get(), m_includeHandler.Get());
	Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = CompileShader(L"Object3D.PS.hlsl", L"ps_6_0", m_dxcUtils.Get(), m_dxcCompiler.Get(), m_includeHandler.Get());

	// PSO生成
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = m_RootSignature.Get();
	psoDesc.InputLayout = inputLayoutDesc;
	psoDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
	psoDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
	psoDesc.BlendState = blendDesc;
	psoDesc.RasterizerState = rasterizerDesc;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // GraphicsCoreの形式に合わせる
	psoDesc.DepthStencilState.DepthEnable = true;
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT; // GraphicsCoreのDepthBuffer形式に合わせる
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_PipelineState));
	assert(SUCCEEDED(hr));

	// ==================================
	// リソース生成 (WVP, Material, Light, VertexBuffer等)
	// ==================================
	m_wvpResource = CreateBufferResource(device, Align256(sizeof(TransformationMatrix)));
	TransformationMatrix* wvpData = nullptr;
	m_wvpResource.Get()->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));
	wvpData->WVP = Matrix4x4::MakeIdentity4x4();
	wvpData->World = Matrix4x4::MakeIdentity4x4();


	// ==================================
	// 平行光源用リソースの生成
	// ==================================
	m_directionalLightResource = CreateBufferResource(device, Align256(sizeof(DirectionalLight)));
	DirectionalLight* lightData = nullptr;
	m_directionalLightResource.Get()->Map(0, nullptr, reinterpret_cast<void**>(&lightData));
	lightData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	lightData->direction = { 0.0f, -1.0f, 0.0f };
	lightData->intensity = 1.0f;

	// ==================================
	// Sprite用のResourceの生成
	// ==================================
	ResourceObject vertexResourceSprite = CreateBufferResource(device, sizeof(VertexData) * 6); // 頂点6つ分のサイズ

	// 頂点バッファビューを作成する
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite = {};
	// リソースの先頭のアドレスから使う
	vertexBufferViewSprite.BufferLocation = vertexResourceSprite.Get()->GetGPUVirtualAddress();
	// 使用するリソースのサイズは頂点6つ分のサイズ
	vertexBufferViewSprite.SizeInBytes = sizeof(VertexData) * 6;
	// 1頂点あたりのサイズ
	vertexBufferViewSprite.StrideInBytes = sizeof(VertexData);

	/*頂点データを設定する*/
	VertexData* vertexDataSprite = nullptr;
	vertexResourceSprite.Get()->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSprite));
	// 一枚目の三角形
	vertexDataSprite[0].position = { 0.0f, 360.0f, 0.0f, 1.0f }; // 左下
	vertexDataSprite[0].texcoord = { 0.0f, 1.0f };
	vertexDataSprite[1].position = { 0.0f, 0.0f, 0.0f, 1.0f }; // 左上
	vertexDataSprite[1].texcoord = { 0.0f, 0.0f };
	vertexDataSprite[2].position = { 640.0f, 360.0f, 0.0f, 1.0f }; // 右下
	vertexDataSprite[2].texcoord = { 1.0f, 1.0f };

	// 二枚目の三角形
	vertexDataSprite[3].position = { 0.0f, 0.0f, 0.0f, 1.0f }; // 左上
	vertexDataSprite[3].texcoord = { 0.0f, 0.0f };
	vertexDataSprite[4].position = { 640.0f, 0.0f, 0.0f, 1.0f }; // 右上
	vertexDataSprite[4].texcoord = { 1.0f, 0.0f };
	vertexDataSprite[5].position = { 640.0f, 360.0f, 0.0f, 1.0f }; // 右下
	vertexDataSprite[5].texcoord = { 1.0f, 1.0f };

	// Sprite用のTransformationMatrix用のリソースを作る
	//ID3D12Resource* transformationMatrixResourceSprite = CreateBufferResource(device.Get(), sizeof(Matrix4x4));
	ResourceObject transformationMatrixResourceSprite = CreateBufferResource(device, Align256(sizeof(Matrix4x4)));
	Matrix4x4* transformationMatrixDataSprite = nullptr;
	// 書き込むためのアドレスを取得 vertexBufferView
	transformationMatrixResourceSprite.Get()->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixDataSprite));
	// 単位行列を書き込む
	*transformationMatrixDataSprite = Matrix4x4::MakeIdentity4x4();

	// CPUで動かす用の行列を用意
	Transform transformSprite{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };

	// ==================================
	// Objファイル形式で読み込むオブジェクト
	// ==================================
	// !\モデル読み込み
	ModelData objModelData = LoadObjFile("resources/cube", "cube.obj");

	const UINT objVertexCount = static_cast<UINT>(objModelData.vertices.size());
	// 頂点リソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> objVertexResource = CreateBufferResource(device, sizeof(VertexData) * objModelData.vertices.size());

	//頂点バッファビューを作成する
	D3D12_VERTEX_BUFFER_VIEW objVertexBufferView{};
	objVertexBufferView.BufferLocation = objVertexResource->GetGPUVirtualAddress();
	objVertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * objModelData.vertices.size());
	objVertexBufferView.StrideInBytes = sizeof(VertexData);

	//頂点リソースにデータを書き込む
	VertexData* objVertexData = nullptr;
	objVertexResource->Map(0, nullptr, reinterpret_cast<void**>(&objVertexData));
	std::memcpy(objVertexData, objModelData.vertices.data(),
		sizeof(VertexData) * objModelData.vertices.size());

	// ==================================
	// Sphere用のResourceの生成
	// ==================================
	// 分割数
	const uint32_t kSubdivision = 16;
	// 頂点数 (分割数 * 分割数 * 6頂点)
	const uint32_t kSphereVertexCount = kSubdivision * kSubdivision * 6;


	// 頂点リソースの作成
	ResourceObject vertexResourceSphere = CreateBufferResource(device, sizeof(VertexData) * kSphereVertexCount);

	// 頂点バッファビューを作成する
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSphere = {};
	vertexBufferViewSphere.BufferLocation = vertexResourceSphere.Get()->GetGPUVirtualAddress();
	vertexBufferViewSphere.SizeInBytes = sizeof(VertexData) * kSphereVertexCount;
	vertexBufferViewSphere.StrideInBytes = sizeof(VertexData);

	// 頂点データを書き込む
	VertexData* vertexDataSphere = nullptr;
	vertexResourceSphere.Get()->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSphere));

	// 経度分割1つ分の角度 phi
	const float kLonEvery = 2.0f * std::numbers::pi_v<float> / float(kSubdivision);
	// 緯度分割1つ分の角度 theta
	const float kLatEvery = std::numbers::pi_v<float> / float(kSubdivision);

	// 緯度の方向に分割
	for (uint32_t latIndex = 0; latIndex < kSubdivision; ++latIndex) {
		float lat = -std::numbers::pi_v<float> / 2.0f + kLatEvery * latIndex; // theta

		// 経度の方向に分割
		for (uint32_t lonIndex = 0; lonIndex < kSubdivision; ++lonIndex) {
			float lon = lonIndex * kLonEvery; // phi

			// 書き込む頂点の先頭インデックス
			uint32_t start = (latIndex * kSubdivision + lonIndex) * 6;

			// 基準点a, b, c, dの計算に必要な角度
			float lat_b = lat + kLatEvery;
			float lon_c = lon + kLonEvery;

			// UV座標の計算用
			float u_curr = float(lonIndex) / float(kSubdivision);
			float u_next = float(lonIndex + 1) / float(kSubdivision);
			float v_curr = 1.0f - float(latIndex) / float(kSubdivision);
			float v_next = 1.0f - float(latIndex + 1) / float(kSubdivision);

			// 頂点a (左下)
			vertexDataSphere[start].position.x = std::cos(lat) * std::cos(lon);
			vertexDataSphere[start].position.y = std::sin(lat);
			vertexDataSphere[start].position.z = std::cos(lat) * std::sin(lon);
			vertexDataSphere[start].position.w = 1.0f;
			vertexDataSphere[start].texcoord = { u_curr, v_curr };
			vertexDataSphere[start].normal = { vertexDataSphere[start].position.x, vertexDataSphere[start].position.y, vertexDataSphere[start].position.z };

			// 頂点b (左上)
			vertexDataSphere[start + 1].position.x = std::cos(lat_b) * std::cos(lon);
			vertexDataSphere[start + 1].position.y = std::sin(lat_b);
			vertexDataSphere[start + 1].position.z = std::cos(lat_b) * std::sin(lon);
			vertexDataSphere[start + 1].position.w = 1.0f;
			vertexDataSphere[start + 1].texcoord = { u_curr, v_next };
			vertexDataSphere[start + 1].normal = { vertexDataSphere[start + 1].position.x, vertexDataSphere[start + 1].position.y, vertexDataSphere[start + 1].position.z };

			// 頂点c (右下)
			vertexDataSphere[start + 2].position.x = std::cos(lat) * std::cos(lon_c);
			vertexDataSphere[start + 2].position.y = std::sin(lat);
			vertexDataSphere[start + 2].position.z = std::cos(lat) * std::sin(lon_c);
			vertexDataSphere[start + 2].position.w = 1.0f;
			vertexDataSphere[start + 2].texcoord = { u_next, v_curr };
			vertexDataSphere[start + 2].normal = { vertexDataSphere[start + 2].position.x, vertexDataSphere[start + 2].position.y, vertexDataSphere[start + 2].position.z };

			// 頂点d (右上) - 2枚目の三角形用
			vertexDataSphere[start + 3] = vertexDataSphere[start + 1]; // b
			vertexDataSphere[start + 4].position.x = std::cos(lat_b) * std::cos(lon_c);
			vertexDataSphere[start + 4].position.y = std::sin(lat_b);
			vertexDataSphere[start + 4].position.z = std::cos(lat_b) * std::sin(lon_c);
			vertexDataSphere[start + 4].position.w = 1.0f;
			vertexDataSphere[start + 4].texcoord = { u_next, v_next };
			vertexDataSphere[start + 4].normal = { vertexDataSphere[start + 4].position.x, vertexDataSphere[start + 4].position.y, vertexDataSphere[start + 4].position.z };
			vertexDataSphere[start + 5] = vertexDataSphere[start + 2]; // c
		}
	}
	// 書き込み終了
	vertexResourceSphere.Get()->Unmap(0, nullptr);

	// Sphere用のTransformationMatrix用のリソースを作る
	ResourceObject wvpResourceSphere = CreateBufferResource(device, Align256(sizeof(TransformationMatrix)));
	TransformationMatrix* wvpDataSphere = nullptr;
	wvpResourceSphere.Get()->Map(0, nullptr, reinterpret_cast<void**>(&wvpDataSphere));
	wvpDataSphere->WVP = Matrix4x4::MakeIdentity4x4();
	wvpDataSphere->World = Matrix4x4::MakeIdentity4x4();

	// ==================================
	// ImGui初期化
	// ==================================
	// 専用のSRVヒープを作成
	m_srvDescriptorHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(Window::GetInstance()->GetHwnd());
	ImGui_ImplDX12_Init(
		device,
		3, // GraphicsCore::BufferCount
		DXGI_FORMAT_R8G8B8A8_UNORM,
		m_srvDescriptorHeap.Get(),
		m_srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		m_srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart()
	);

	// テクスチャロードなどもここで行う

	// ==================================
	// Textureを読んで転送する
	// ==================================

	// ==================================
	// 1. テクスチャファイルの読み込み
	DirectX::ScratchImage mipImage1 = LoadTexture("resources/uvChecker.png");
	DirectX::ScratchImage mipImages2 = LoadTexture(objModelData.material.textureFilePath);
	const DirectX::TexMetadata& metadata = mipImage1.GetMetadata();
	const DirectX::TexMetadata& metadata2 = mipImages2.GetMetadata();


	// ==================================
	// 2. GPUリソース（Texture）の作成
	// ==================================
	m_textureResource = CreateTextureResource(device, metadata);
	m_textureResource2 = CreateTextureResource(device, metadata2);

	// ==================================
	// 3. 転送コマンドの記録
	// ==================================
	GraphicsContext& context = GraphicsContext::Begin(L"Texture Upload");
	ID3D12GraphicsCommandList* commandList = context.GetCommandList();

	ResourceObject intermediateResource = UploadTextureData(m_textureResource, mipImage1,device,commandList);
	ResourceObject intermediateResource2 = UploadTextureData(m_textureResource2, mipImages2, device, commandList);
	
	// ==================================
	// 4. 転送コマンドの実行と完了待ち
	// ==================================
	context.Finish(true);

	// DescriptorのSizeを取得
	const uint32_t descriptorSizeSRV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	const uint32_t descriptorSizeRTV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	const uint32_t descriptorSizeDSV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	// ==================================
	// ShaderResourceViewの生成(SRV)の生成
	// ==================================
	// metaDataをもとにSRVの設定を行う
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
	srvDesc.Texture2D.MipLevels = static_cast<UINT>(metadata.mipLevels);

	// SRVを作成するDescriptorHeapの場所を決める
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU = m_srvDescriptorHeap.Get()->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU = m_srvDescriptorHeap.Get()->GetGPUDescriptorHandleForHeapStart();
	// 先頭はImGuiが使うので1つ分ずらす
	textureSrvHandleCPU.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	textureSrvHandleGPU.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	// SRVを生成
	device->CreateShaderResourceView(m_textureResource.Get(), &srvDesc, textureSrvHandleCPU);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2 = {};
	srvDesc2.Format = metadata2.format;
	srvDesc2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc2.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
	srvDesc2.Texture2D.MipLevels = static_cast<UINT>(metadata2.mipLevels);

	// SRVを作成するDescriptorHeapの場所を決める
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU2 = GetCPUDescriptorHandle(m_srvDescriptorHeap.Get(), descriptorSizeSRV, 2);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU2 = GetGPUDescriptorHandle(m_srvDescriptorHeap.Get(), descriptorSizeSRV, 2);
	// SRVを生成
	device->CreateShaderResourceView(m_textureResource2.Get(), &srvDesc2, textureSrvHandleCPU2);

}

void Game::Update() {
	Input().Update();

	// ImGui更新
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// ゲームロジック
	if (Input().PushKey(VK_UP)) m_transform.translate.z += 0.1f;
	if (Input().PushKey(VK_DOWN)) m_transform.translate.z -= 0.1f;
	if (Input().PushKey(VK_LEFT)) m_transform.rotate.y += 0.1f;
	if (Input().PushKey(VK_RIGHT)) m_transform.rotate.y -= 0.1f;

	// 行列更新
	Matrix4x4 worldMatrix = MakeAffineMatrix(m_transform.scale, m_transform.rotate, m_transform.translate);
	Matrix4x4 cameraMatrix = MakeAffineMatrix(m_cameraTransform.scale, m_cameraTransform.rotate, m_cameraTransform.translate);
	Matrix4x4 viewMatrix = Matrix4x4::Inverse(cameraMatrix);
	Matrix4x4 projectionMatrix = Matrix4x4::MakeParspectiveFovMatrix(0.45f, 1280.0f / 720.0f, 0.1f, 100.0f);
	Matrix4x4 wvpMatrix = Matrix4x4::Multiply(worldMatrix, Matrix4x4::Multiply(viewMatrix, projectionMatrix));

	// リソース更新
	TransformationMatrix* wvpData = nullptr;
	m_wvpResource.Get()->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));
	wvpData->World = worldMatrix;
	wvpData->WVP = wvpMatrix;
	m_wvpResource.Get()->Unmap(0, nullptr);

	// ImGui UI構成
	ImGui::Begin("Debug");
	ImGui::ColorEdit4("Material Color", m_materialColor);
	ImGui::End();

	ImGui::Render();
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
	commandList->SetGraphicsRootSignature(m_RootSignature.Get());
	commandList->SetPipelineState(m_PipelineState.Get());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// ヒープ設定 (ImGui用含む)
	ID3D12DescriptorHeap* heaps[] = { m_srvDescriptorHeap.Get() };
	commandList->SetDescriptorHeaps(1, heaps);

	// 定数バッファ・描画 (実際の描画処理)
	// commandList->SetGraphicsRootConstantBufferView(1, m_wvpResource.Get()->GetGPUVirtualAddress());
	// commandList->IASetVertexBuffers(...)
	// commandList->DrawInstanced(...)

	// Object描画コマンド
	commandList->DrawInstanced(UINT(m_objModelData.vertices.size()), 1, 0, 0);

	// ImGui描画
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

	// 4. 表示用状態へ遷移 (即座にフラッシュ)
	context.TransitionResource(backBuffer, D3D12_RESOURCE_STATE_PRESENT, true);

	// 5. コマンドリスト終了と実行
	context.Finish();

	GraphicsCore::GetInstance()->Present();
}

void Game::Shutdown() {
	// GPU処理の完了を待機
	GraphicsCore::GetInstance()->GetCommandListManager().GetGraphicsQueue().WaitForIdle();

	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	// リソースはComPtrやResourceObjectデストラクタで解放される
}