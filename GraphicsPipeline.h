#pragma once
#include "TomoEngine.h"
#include <d3d12.h>
#include <dxcapi.h>
#include <wrl/client.h>
#include <string>

class GraphicsPipeline {
public:
    GraphicsPipeline() = default;
    ~GraphicsPipeline() = default;

    // 初期化 (DXC初期化 -> RootSignature生成 -> PSO生成)
    void Initialize();

    // 描画時にコマンドリストにセットする
    void SetState(ID3D12GraphicsCommandList* commandList);

    // ゲッター (必要に応じて)
    ID3D12RootSignature* GetRootSignature() const { return rootSignature_.Get(); }
    ID3D12PipelineState* GetPipelineState() const { return pipelineState_.Get(); }

private:
    // 内部処理
    void InitializeDXC();
    void CreateRootSignature();
    void CreatePSO();

    // ヘルパー: シェーダーコンパイル
    Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(
        const std::wstring& filePath,
        const std::wstring& profile);

    // デバイス取得用
    ID3D12Device* GetDevice();

private:
    // DXC関連 (シェーダーコンパイラ)
    Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils_;
    Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler_;
    Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler_;

    // パイプラインステート関連
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;
};