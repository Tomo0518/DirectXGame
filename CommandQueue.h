#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>

// コマンドキューを管理するクラス
// GPUへの命令投入(Execute)と同期(Signal/Wait)を担当する
class CommandQueue {
public:
	// キューの種類に応じた初期化を行う
	void Initialize(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);

	// 終了処理
	void Shutdown();

	// コマンドリストを実行し、その完了を待つためのフェンス値を返す
	uint64_t ExecuteCommandList(ID3D12CommandList* commandList);

	// 特定のフェンス値になるまでGPUを待機させる（GPU側でのWait）
	// ※主にComputeQueueとDirectQueueの同期などで使用
	void WaitOnGPU(uint64_t fenceValue);

	// 指定したフェンス値の完了をCPU側で待つ
	void WaitForFence(uint64_t fenceValue);

	// コマンドキューの生ポインタ取得
	ID3D12CommandQueue* GetD3D12CommandQueue() const { return commandQueue_.Get(); }

	// 最後にシグナルしたフェンス値を取得
	uint64_t GetLastCompletedFenceValue() { return fence_->GetCompletedValue(); }
	uint64_t GetNextFenceValue() const { return nextFenceValue_; }

private:
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_;
	Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
	HANDLE fenceEvent_ = nullptr;
	uint64_t nextFenceValue_ = 0;
	D3D12_COMMAND_LIST_TYPE type_ = D3D12_COMMAND_LIST_TYPE_DIRECT;
};