#include "CommandContext.h"
#include "CommandQueue.h"
#include <cassert>

void CommandContext::Initialize(ID3D12Device* device, CommandQueue* queue) {
	assert(device != nullptr && queue != nullptr);
	queue_ = queue;

	// コマンドアロケータの生成
	HRESULT hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator_));
	assert(SUCCEEDED(hr));

	// コマンドリストの生成
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator_.Get(), nullptr, IID_PPV_ARGS(&commandList_));
	assert(SUCCEEDED(hr));

	// 初期状態はCloseしておく（StartでResetするため）
	commandList_->Close();
}

void CommandContext::Shutdown() {
	commandAllocator_.Reset();
	commandList_.Reset();
	queue_ = nullptr;
}

void CommandContext::Start() {
	// アロケータとコマンドリストをリセットして記録開始状態にする
	// ※本来は実行完了待ちが必要だが、今回はFinishでWaitを入れる簡易実装とする
	HRESULT hr = commandAllocator_->Reset();
	assert(SUCCEEDED(hr));

	hr = commandList_->Reset(commandAllocator_.Get(), nullptr);
	assert(SUCCEEDED(hr));

	numBarriersToFlush_ = 0;
}

uint64_t CommandContext::Finish(bool waitForCompletion) {
	// 未発行のバリアがあれば発行
	FlushResourceBarriers();

	// 記録終了
	HRESULT hr = commandList_->Close();
	assert(SUCCEEDED(hr));

	// キューで実行
	uint64_t fenceValue = queue_->ExecuteCommandList(commandList_.Get());

	// 完了待ちフラグがあれば待機
	if (waitForCompletion) {
		queue_->WaitForFence(fenceValue);
	}

	return fenceValue;
}

void CommandContext::TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter) {
	if (stateBefore == stateAfter) return;

	// バッファが一杯になったら一度フラッシュ
	if (numBarriersToFlush_ >= kMaxResourceBarriers) {
		FlushResourceBarriers();
	}

	D3D12_RESOURCE_BARRIER& barrier = resourceBarriers_[numBarriersToFlush_++];
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = resource;
	barrier.Transition.StateBefore = stateBefore;
	barrier.Transition.StateAfter = stateAfter;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
}

void CommandContext::FlushResourceBarriers() {
	if (numBarriersToFlush_ > 0) {
		commandList_->ResourceBarrier(numBarriersToFlush_, resourceBarriers_);
		numBarriersToFlush_ = 0;
	}
}

// === 以下ラッパー関数 ===

void CommandContext::ClearRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtv, const float* color) {
	FlushResourceBarriers(); // 描画・クリア命令の前にバリアをフラッシュ
	commandList_->ClearRenderTargetView(rtv, color, 0, nullptr);
}

void CommandContext::ClearDepth(D3D12_CPU_DESCRIPTOR_HANDLE dsv) {
	FlushResourceBarriers();
	commandList_->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void CommandContext::SetRenderTargets(UINT numRTV, const D3D12_CPU_DESCRIPTOR_HANDLE* rtv, D3D12_CPU_DESCRIPTOR_HANDLE dsv) {
	commandList_->OMSetRenderTargets(numRTV, rtv, FALSE, &dsv);
}

void CommandContext::SetViewport(const D3D12_VIEWPORT& viewport) {
	commandList_->RSSetViewports(1, &viewport);
}

void CommandContext::SetScissorRect(const D3D12_RECT& rect) {
	commandList_->RSSetScissorRects(1, &rect);
}

void CommandContext::SetGraphicsRootSignature(ID3D12RootSignature* rootSig) {
	commandList_->SetGraphicsRootSignature(rootSig);
}

void CommandContext::SetPipelineState(ID3D12PipelineState* pso) {
	commandList_->SetPipelineState(pso);
}

void CommandContext::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology) {
	commandList_->IASetPrimitiveTopology(topology);
}

void CommandContext::SetVertexBuffer(UINT slot, const D3D12_VERTEX_BUFFER_VIEW& view) {
	commandList_->IASetVertexBuffers(slot, 1, &view);
}

void CommandContext::SetDescriptorHeap(ID3D12DescriptorHeap* heap) {
	ID3D12DescriptorHeap* heaps[] = { heap };
	commandList_->SetDescriptorHeaps(1, heaps);
}

void CommandContext::SetGraphicsRootConstantBufferView(UINT rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS address) {
	commandList_->SetGraphicsRootConstantBufferView(rootParameterIndex, address);
}

void CommandContext::SetGraphicsRootDescriptorTable(UINT rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE baseDescriptor) {
	commandList_->SetGraphicsRootDescriptorTable(rootParameterIndex, baseDescriptor);
}

void CommandContext::DrawInstanced(UINT vertexCount, UINT instanceCount, UINT startVertex, UINT startInstance) {
	FlushResourceBarriers(); // 描画前に必ずバリアをフラッシュ
	commandList_->DrawInstanced(vertexCount, instanceCount, startVertex, startInstance);
}