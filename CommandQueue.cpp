#include "CommandQueue.h"
#include <cassert>

void CommandQueue::Initialize(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type) {
	assert(device != nullptr);
	type_ = type;

	// コマンドキューの生成
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = type_;
	queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.NodeMask = 0;

	HRESULT hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue_));
	assert(SUCCEEDED(hr));

	// フェンスの生成
	hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
	assert(SUCCEEDED(hr));
	fence_->Signal(0);

	// フェンスイベントの生成
	fenceEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	assert(fenceEvent_ != nullptr);

	nextFenceValue_ = 1;
}

void CommandQueue::Shutdown() {
	if (fenceEvent_) {
		CloseHandle(fenceEvent_);
		fenceEvent_ = nullptr;
	}
	commandQueue_.Reset();
	fence_.Reset();
}

uint64_t CommandQueue::ExecuteCommandList(ID3D12CommandList* commandList) {
	// コマンドリストを実行
	commandQueue_->ExecuteCommandLists(1, &commandList);

	// フェンスにシグナルを送る
	// "このコマンドリストが終わったら、この値(nextFenceValue_)を書き込んでね" という命令
	uint64_t fenceValue = nextFenceValue_;
	commandQueue_->Signal(fence_.Get(), fenceValue);

	// 次回のフェンス値を更新
	nextFenceValue_++;

	return fenceValue;
}

void CommandQueue::WaitOnGPU(uint64_t fenceValue) {
	// 指定したフェンス値になるまで、このキューをGPU側で待機させる
	commandQueue_->Wait(fence_.Get(), fenceValue);
}

void CommandQueue::WaitForFence(uint64_t fenceValue) {
	// 指定した値にまだ達していない場合、CPUを止めて待つ
	if (fence_->GetCompletedValue() < fenceValue) {
		fence_->SetEventOnCompletion(fenceValue, fenceEvent_);
		WaitForSingleObject(fenceEvent_, INFINITE);
	}
}