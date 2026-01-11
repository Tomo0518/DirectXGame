#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <vector>

// 前方宣言
class CommandQueue;

// コマンドリストへの記録を管理するクラス
// ResourceBarrierのバッチ処理や、RenderResourceの管理を行う
class CommandContext {
public:
	// コンストラクタでデバイスとキューを受け取る
	void Initialize(ID3D12Device* device, CommandQueue* queue);

	// 終了処理
	void Shutdown();

	// コマンドリストの記録を開始する（アロケータのリセットなど）
	void Start();

	// コマンドリストの記録を終了し、キューで実行する
	// 戻り値: 実行完了を待つためのフェンス値
	uint64_t Finish(bool waitForCompletion = false);

	// リソースバリアの設定（即時発行せず、ドローコール直前まで溜め込む）
	void TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter);

	// 溜め込んだバリアをコマンドリストに積む
	void FlushResourceBarriers();

	// === コマンドリストのラッパー関数群 ===
	void ClearRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtv, const float* color);
	void ClearDepth(D3D12_CPU_DESCRIPTOR_HANDLE dsv);

	void SetRenderTargets(UINT numRTV, const D3D12_CPU_DESCRIPTOR_HANDLE* rtv, D3D12_CPU_DESCRIPTOR_HANDLE dsv);
	void SetViewport(const D3D12_VIEWPORT& viewport);
	void SetScissorRect(const D3D12_RECT& rect);

	void SetGraphicsRootSignature(ID3D12RootSignature* rootSig);
	void SetPipelineState(ID3D12PipelineState* pso);
	void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology);

	void SetVertexBuffer(UINT slot, const D3D12_VERTEX_BUFFER_VIEW& view);
	void SetDescriptorHeap(ID3D12DescriptorHeap* heap);
	void SetGraphicsRootConstantBufferView(UINT rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS address);
	void SetGraphicsRootDescriptorTable(UINT rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE baseDescriptor);

	void DrawInstanced(UINT vertexCount, UINT instanceCount, UINT startVertex, UINT startInstance);

	// 生のリスト取得（緊急回避用）
	ID3D12GraphicsCommandList* GetCommandList() const { return commandList_.Get(); }

private:
	CommandQueue* queue_ = nullptr; // 所属するキュー
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator_;

	// バリアのバッチ処理用
	static const int kMaxResourceBarriers = 16;
	D3D12_RESOURCE_BARRIER resourceBarriers_[kMaxResourceBarriers];
	UINT numBarriersToFlush_ = 0;
};