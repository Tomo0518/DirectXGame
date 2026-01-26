#pragma once
#include <d3d12.h>
#include <dxcapi.h>
#include <wrl.h>
#include <string>

enum class PipelineType {
    Object3D,
    Particle
};

class GraphicsPipeline {
public:
    void Initialize();
    void SetState(ID3D12GraphicsCommandList* commandList, PipelineType type);

private:
    void InitializeDXC();

    // Object3D用
    void CreateObject3DRootSignature();
    void CreateObject3DPSO();

    // Particle用
    void CreateParticleRootSignature();
    void CreateParticlePSO();

    Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(const std::wstring& filePath, const std::wstring& profile);
    ID3D12Device* GetDevice();

private:
    // DXC
    Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils_;
    Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler_;
    Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler_;

    // Object3D用
    Microsoft::WRL::ComPtr<ID3D12RootSignature> object3DRootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> object3DPipelineState_;

    // Particle用
    Microsoft::WRL::ComPtr<ID3D12RootSignature> particleRootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> particlePipelineState_;
};