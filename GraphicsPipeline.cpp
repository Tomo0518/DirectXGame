#include "GraphicsPipeline.h"
#include "GraphicsCore.h"
#include "ConvertString.h"
#include <cassert>
#include <format>
#include "Logger.h"

void GraphicsPipeline::Initialize() {
    InitializeDXC();

    // Object3D用の初期化
    CreateObject3DRootSignature();
    CreateObject3DPSO();

    // Particle用の初期化
    CreateParticleRootSignature();
    CreateParticlePSO();
}

void GraphicsPipeline::SetState(ID3D12GraphicsCommandList* commandList, PipelineType type) {
    if (type == PipelineType::Object3D) {
        commandList->SetGraphicsRootSignature(object3DRootSignature_.Get());
        commandList->SetPipelineState(object3DPipelineState_.Get());
    }
    else if (type == PipelineType::Particle) {
        commandList->SetGraphicsRootSignature(particleRootSignature_.Get());
        commandList->SetPipelineState(particlePipelineState_.Get());
    }
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

ID3D12Device* GraphicsPipeline::GetDevice() {
    return GraphicsCore::GetInstance()->GetDevice();
}

void GraphicsPipeline::InitializeDXC() {
    HRESULT hr = S_OK;
    hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils_));
    assert(SUCCEEDED(hr));
    hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler_));
    assert(SUCCEEDED(hr));
    hr = dxcUtils_->CreateDefaultIncludeHandler(&includeHandler_);
    assert(SUCCEEDED(hr));
}

// ===================================
// Object3D用
// ===================================

void GraphicsPipeline::CreateObject3DRootSignature() {
    ID3D12Device* device = GetDevice();

    D3D12_ROOT_PARAMETER rootParameters[4] = {};

    // [0] Material (CBV)
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[0].Descriptor.ShaderRegister = 0;

    // [1] WVP (CBV)
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    rootParameters[1].Descriptor.ShaderRegister = 0;

    // [2] Texture (Descriptor Table)
    D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
    descriptorRange[0].BaseShaderRegister = 0;
    descriptorRange[0].NumDescriptors = 1;
    descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
    rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;

    // [3] Light (CBV)
    rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[3].Descriptor.ShaderRegister = 1;

    D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
    staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
    staticSamplers[0].ShaderRegister = 0;
    staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    rootSignatureDesc.pParameters = rootParameters;
    rootSignatureDesc.NumParameters = _countof(rootParameters);
    rootSignatureDesc.pStaticSamplers = staticSamplers;
    rootSignatureDesc.NumStaticSamplers = _countof(staticSamplers);

    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    if (FAILED(hr)) {
        Log(ConvertString(static_cast<const char*>(errorBlob->GetBufferPointer())));
        assert(false);
    }
    hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&object3DRootSignature_));
    assert(SUCCEEDED(hr));
}

void GraphicsPipeline::CreateObject3DPSO() {
    ID3D12Device* device = GetDevice();

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
    inputElementDescs[0].SemanticName = "POSITION"; inputElementDescs[0].SemanticIndex = 0; inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT; inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    inputElementDescs[1].SemanticName = "TEXCOORD"; inputElementDescs[1].SemanticIndex = 0; inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT; inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    inputElementDescs[2].SemanticName = "NORMAL";   inputElementDescs[2].SemanticIndex = 0; inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT; inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
    inputLayoutDesc.pInputElementDescs = inputElementDescs;
    inputLayoutDesc.NumElements = _countof(inputElementDescs);

    D3D12_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;

    D3D12_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

    Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = CompileShader(L"Object3d.VS.hlsl", L"vs_6_0");
    Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = CompileShader(L"Object3d.PS.hlsl", L"ps_6_0");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = object3DRootSignature_.Get();
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

    HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&object3DPipelineState_));
    assert(SUCCEEDED(hr));
}

// ===================================
// Particle用
// ===================================

void GraphicsPipeline::CreateParticleRootSignature() {
    ID3D12Device* device = GetDevice();

    D3D12_ROOT_PARAMETER rootParameters[4] = {};

    // [0] Material (CBV)
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[0].Descriptor.ShaderRegister = 0;

    // [1] Instancing Data (StructuredBuffer SRV)
    D3D12_DESCRIPTOR_RANGE descriptorRangeForInstancing[1] = {};
    descriptorRangeForInstancing[0].BaseShaderRegister = 0;
    descriptorRangeForInstancing[0].NumDescriptors = 1;
    descriptorRangeForInstancing[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRangeForInstancing[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    rootParameters[1].DescriptorTable.pDescriptorRanges = descriptorRangeForInstancing;
    rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;

    // [2] Texture (Descriptor Table)
    D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
    descriptorRange[0].BaseShaderRegister = 0;
    descriptorRange[0].NumDescriptors = 1;
    descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
    rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;

    // [3] Light (CBV)
    rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[3].Descriptor.ShaderRegister = 1;

    D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
    staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
    staticSamplers[0].ShaderRegister = 0;
    staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    rootSignatureDesc.pParameters = rootParameters;
    rootSignatureDesc.NumParameters = _countof(rootParameters);
    rootSignatureDesc.pStaticSamplers = staticSamplers;
    rootSignatureDesc.NumStaticSamplers = _countof(staticSamplers);

    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    if (FAILED(hr)) {
        Log(ConvertString(static_cast<const char*>(errorBlob->GetBufferPointer())));
        assert(false);
    }
    hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&particleRootSignature_));
    assert(SUCCEEDED(hr));
}

void GraphicsPipeline::CreateParticlePSO() {
    ID3D12Device* device = GetDevice();

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
    inputElementDescs[0].SemanticName = "POSITION"; inputElementDescs[0].SemanticIndex = 0; inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT; inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    inputElementDescs[1].SemanticName = "TEXCOORD"; inputElementDescs[1].SemanticIndex = 0; inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT; inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    inputElementDescs[2].SemanticName = "NORMAL";   inputElementDescs[2].SemanticIndex = 0; inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT; inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
    inputLayoutDesc.pInputElementDescs = inputElementDescs;
    inputLayoutDesc.NumElements = _countof(inputElementDescs);

    D3D12_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;

    D3D12_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

    Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = CompileShader(L"Particle.VS.hlsl", L"vs_6_0");
    Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = CompileShader(L"Particle.PS.hlsl", L"ps_6_0");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = particleRootSignature_.Get();
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

    HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&particlePipelineState_));
    assert(SUCCEEDED(hr));
}

Microsoft::WRL::ComPtr<IDxcBlob> GraphicsPipeline::CompileShader(const std::wstring& filePath, const std::wstring& profile) {
    Log(ConvertString(std::format(L"Begin CompileShader, path:{}, profile:{}\n", filePath, profile)));

    Microsoft::WRL::ComPtr<IDxcBlobEncoding> shaderSource = nullptr;
    HRESULT hr = dxcUtils_->LoadFile(filePath.c_str(), nullptr, &shaderSource);
    assert(SUCCEEDED(hr));

    DxcBuffer shaderSourceBuffer;
    shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
    shaderSourceBuffer.Size = shaderSource->GetBufferSize();
    shaderSourceBuffer.Encoding = DXC_CP_UTF8;

    LPCWSTR arguments[] = {
        filePath.c_str(),
        L"-E", L"main",
        L"-T", profile.c_str(),
        L"-Zi", L"-Qembed_debug",
        L"-Od",
        L"-Zpr",
    };

    Microsoft::WRL::ComPtr<IDxcResult> shaderResult = nullptr;
    hr = dxcCompiler_->Compile(
        &shaderSourceBuffer,
        arguments,
        _countof(arguments),
        includeHandler_.Get(),
        IID_PPV_ARGS(&shaderResult)
    );
    assert(SUCCEEDED(hr));

    Microsoft::WRL::ComPtr<IDxcBlobUtf8> shaderError = nullptr;
    shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
    if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
        Log(ConvertString(shaderError->GetStringPointer()));
        assert(false);
    }

    Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob = nullptr;
    hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
    assert(SUCCEEDED(hr));
    Log(ConvertString(std::format(L"Compile Succeeded, path:{}, profile:{}\n", filePath, profile)));

    return shaderBlob;
}