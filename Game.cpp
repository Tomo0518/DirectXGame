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

    m_directionalLightResource = CreateBufferResource(device, Align256(sizeof(DirectionalLight)));
    DirectionalLight* lightData = nullptr;
    m_directionalLightResource.Get()->Map(0, nullptr, reinterpret_cast<void**>(&lightData));
    lightData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    lightData->direction = { 0.0f, -1.0f, 0.0f };
    lightData->intensity = 1.0f;

    // Sprite, Sphere, Model等のリソース生成コードをここに移植
    // (省略しますが、main.cppにある VertexResource作成処理などをすべてここに移動します)
    // ...
    // m_objModelData = LoadObjFile("resources/cube", "cube.obj");
    // ...

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
    // ...
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
    // ...

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

    context.TransitionResource(backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // 3. 生のコマンドリストを取得して描画コマンドを積む
    // (GraphicsContextの機能拡張後は context.ClearColor 等を使いますが、
    //  今は既存コードを活かすためにコマンドリストを直接操作します)
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

    // ImGui描画
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

    // 4. 表示用状態へ遷移
    context.TransitionResource(backBuffer, D3D12_RESOURCE_STATE_PRESENT);

    // 5. コマンドリスト終了と実行
    context.Finish();
}

void Game::Shutdown() {
    // GPU処理の完了を待機
    GraphicsCore::GetInstance()->GetCommandListManager().GetGraphicsQueue().WaitForIdle();

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    // リソースはComPtrやResourceObjectデストラクタで解放される
}