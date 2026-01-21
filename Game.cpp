#include "Game.h"
#include "GraphicsCore.h"
#include "CommandContext.h" // GraphicsContextを使う
#include "InputManager.h"
#include <numbers>
#include <format>
#include "TextureManager.h"

// インライン関数などのヘルパーも必要ならここに移動、あるいはUtilityへ
inline InputManager& Input() { return *InputManager::GetInstance(); }

void Game::Initialize() {
    ID3D12Device* device = GraphicsCore::GetInstance()->GetDevice();
    // TextureManager初期化

    // ==================================
    // 1. DXC (Shader Compiler) の初期化
    // ==================================
    HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_dxcUtils));
    assert(SUCCEEDED(hr));
    hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_dxcCompiler));
    assert(SUCCEEDED(hr));
    hr = m_dxcUtils->CreateDefaultIncludeHandler(&m_includeHandler);
    assert(SUCCEEDED(hr));

    // ==================================
    // 2. RootSignature の生成
    // ==================================
    D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature = {};
    descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // テクスチャ用サンプラー設定
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

    // ルートパラメータ設定
    D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
    descriptorRange[0].BaseShaderRegister = 0;
    descriptorRange[0].NumDescriptors = 1;
    descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER rootParameters[4] = {};
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // Material
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[0].Descriptor.ShaderRegister = 0;

    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // WVP
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    rootParameters[1].Descriptor.ShaderRegister = 0;

    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // Texture
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
    rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);

    rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // Light
    rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[3].Descriptor.ShaderRegister = 1;

    descriptionRootSignature.pParameters = rootParameters;
    descriptionRootSignature.NumParameters = _countof(rootParameters);

    // シリアライズと生成
    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
    hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    if (FAILED(hr)) {
        Log(ConvertString(static_cast<const char*>(errorBlob->GetBufferPointer())));
        assert(false);
    }
    hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature));
    assert(SUCCEEDED(hr));

    // ==================================
    // 3. InputLayout / PSO 生成
    // ==================================
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

    // PSO構築
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = m_RootSignature.Get();
    psoDesc.InputLayout = inputLayoutDesc;
    psoDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
    psoDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
    psoDesc.BlendState = blendDesc;
    psoDesc.RasterizerState = rasterizerDesc;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.DepthStencilState.DepthEnable = true;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

    hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_PipelineState));
    assert(SUCCEEDED(hr));

    // ==================================
    // 4. 各種定数バッファ・リソース生成
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
    lightData_->direction = { 0.0f, -1.0f, 0.0f };
    lightData_->intensity = 1.0f;

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

    // 2. Modelの生成
    //    Modelクラス側もTextureManagerを使うように修正されている前提です
    //    引数から「ディスクリプタハンドル」が消え、コマンドリストのみを渡します
    m_modelCube_ = Model::CreateFromOBJ("resources/cube", "cube.obj", commandList);

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

	// 光源の調整
	ImGui::Text("Light Control");
	ImGui::ColorEdit4("Light Color", &lightData_->color.x);
	ImGui::DragFloat3("Light Direction", &lightData_->direction.x, 0.1f);
	ImGui::DragFloat("Light Intensity", &lightData_->intensity, 0.1f, 0.0f, 10.0f);
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
	commandList->SetGraphicsRootSignature(m_RootSignature.Get());
	commandList->SetPipelineState(m_PipelineState.Get());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// ヒープ設定 (ImGui用含む)
    ID3D12DescriptorHeap* heaps[] = { m_srvHeap.GetHeap() };
    commandList->SetDescriptorHeaps(1, heaps);

	// 定数バッファ・描画 (実際の描画処理)
	// commandList->SetGraphicsRootConstantBufferView(1, m_wvpResource.Get()->GetGPUVirtualAddress());
	// commandList->IASetVertexBuffers(...)
	// commandList->DrawInstanced(...)

	// ピクセルシェーダー用マテリアルCBV (Root Parameter Index [0])
	commandList->SetGraphicsRootConstantBufferView(0, m_materialResource.Get()->GetGPUVirtualAddress());

	// 頂点シェーダー用定数バッファ設定 (Root Parameter Index [1])
	commandList->SetGraphicsRootConstantBufferView(1, m_wvpResource.Get()->GetGPUVirtualAddress());

	// ライト用CBV (Root Parameter Index [3])
	commandList->SetGraphicsRootConstantBufferView(3, m_directionalLightResource.Get()->GetGPUVirtualAddress());

	// テクスチャSRV (Root Parameter Index [2])
    D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU = m_srvHeap.GetHeap()->GetGPUDescriptorHandleForHeapStart();

	// ImGui用に1つずらす
	uint32_t descriptorSize = GraphicsCore::GetInstance()->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	textureSrvHandleGPU.ptr += descriptorSize;
	commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU);

	// 2. モデルの描画
	//    PreDraw: 頂点バッファとマテリアル(RootIndex 0)のセット
	m_modelCube_->PreDraw(commandList);

	//    Draw: テクスチャ(RootIndex 2)のセットとドローコール
	m_modelCube_->Draw(commandList);

	// ImGui描画
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

	// 4. 表示用状態へ遷移 (即座にフラッシュ)
	context.TransitionResource(backBuffer, D3D12_RESOURCE_STATE_PRESENT, true);

	// 5. コマンドリスト終了と実行
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