#pragma once
#include "ResourceObject.h"

#include <wrl/client.h>
#include <string>

// DXGI
#include <dxgi1_6.h>
#pragma comment(lib, "dxgi.lib")

#include "CommandQueue.h"
#include "CommandContext.h"

// DirectX12デバイスの生成と管理を行うコアシステム
// MiniEngineにおけるGraphicsCore名前空間の役割を担う
class GraphicsCore {
public:
	// シングルトンインスタンスを取得する
	static GraphicsCore* GetInstance();

	// デバイス、ファクトリー、デバッグレイヤーの初期化を行う
	void Initialize();

	// 明示的に保持リソースを破棄する
	void Shutdown();

	// 生成されたD3D12デバイスを取得する
	ID3D12Device* GetDevice() const { return device_.Get(); }

	// 生成されたDXGIファクトリーを取得する
	IDXGIFactory7* GetFactory() const { return dxgiFactory_.Get(); }

	HRESULT GetHr() { return hr_; }
	
	// コマンドキューを取得する
	CommandQueue& GetCommandQueue() { return commandQueue_; }

	// コマンドコンテキストを取得する
	CommandContext& GetCommandContext() { return commandContext_; }

private:
	GraphicsCore() = default;
	~GraphicsCore() = default;
	GraphicsCore(const GraphicsCore&) = delete;
	GraphicsCore& operator=(const GraphicsCore&) = delete;
	
	HRESULT hr_;

	// ComPtrを使用してリソースを自動解放する
	Microsoft::WRL::ComPtr<ID3D12Device> device_;
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_;

#ifdef _DEBUG
	Microsoft::WRL::ComPtr<ID3D12Debug1> debugController_;
	Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue_;
#endif

	// コマンドキューとコマンドコンテキスト
	CommandQueue commandQueue_;
	CommandContext commandContext_;
};