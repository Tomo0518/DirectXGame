#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <vector>
#include <queue>
#include <mutex>
#include <cstdint>

// ==================================================================================
// CommandAllocatorPool
// コマンドリストのメモリ領域(アロケータ)を再利用するためのプール
// ==================================================================================
class CommandAllocatorPool {
public:
    CommandAllocatorPool(D3D12_COMMAND_LIST_TYPE type);
    ~CommandAllocatorPool();

    void Create(ID3D12Device* device);
    void Shutdown();

    // 利用可能なアロケータを要求
    ID3D12CommandAllocator* RequestAllocator(uint64_t completedFenceValue);
    // 使用済みアロケータを返却
    void DiscardAllocator(uint64_t fenceValue, ID3D12CommandAllocator* allocator);

private:
    D3D12_COMMAND_LIST_TYPE m_cListType;
    ID3D12Device* m_device = nullptr;
    std::vector<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> m_allocatorPool;
    std::queue<std::pair<uint64_t, ID3D12CommandAllocator*>> m_readyAllocators;
    std::mutex m_allocatorMutex;
};

// ==================================================================================
// CommandQueue
// ID3D12CommandQueueのラッパー。フェンス同期とアロケータ管理を行う
// ==================================================================================
class CommandQueue {
public:
    CommandQueue(D3D12_COMMAND_LIST_TYPE type);
    ~CommandQueue();

    void Create(ID3D12Device* device);
    void Shutdown();

    // コマンドリストの実行とフェンスシグナル発行
    uint64_t ExecuteCommandList(ID3D12CommandList* list);

    // アロケータ操作
    ID3D12CommandAllocator* RequestAllocator();
    void DiscardAllocator(uint64_t fenceValueForReset, ID3D12CommandAllocator* allocator);

    // 同期・待機
    void WaitForFence(uint64_t fenceValue);
    bool IsFenceComplete(uint64_t fenceValue);
    uint64_t IncrementFence() {
        return ++m_nextFenceValue;
    }

    uint64_t GetLastCompletedFenceValue() {
        std::lock_guard<std::mutex> lock(m_fenceMutex);
		return m_lastCompletedFenceValue;
    }

	// GPUの処理完了を待機する
	void WaitForIdle() { WaitForFence(IncrementFence()); }

    uint64_t GetNextFenceValue() const { return m_nextFenceValue; }

    ID3D12CommandQueue* GetD3D12CommandQueue() const { return m_commandQueue.Get(); }

private:
    D3D12_COMMAND_LIST_TYPE m_type;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
    Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
    uint64_t m_nextFenceValue = 0;
    uint64_t m_lastCompletedFenceValue = 0;
    HANDLE m_fenceEventHandle = nullptr;

    CommandAllocatorPool m_allocatorPool;
    std::mutex m_fenceMutex;
    std::mutex m_eventMutex;
};

// ==================================================================================
// CommandListManager
// GraphicsCoreが持つメンバ。3種類のキューを保持する
// ==================================================================================
class CommandListManager {
public:
    CommandListManager();
    ~CommandListManager();

    void Create(ID3D12Device* device);
    void Shutdown();

    CommandQueue& GetGraphicsQueue() { return m_graphicsQueue; }
    CommandQueue& GetComputeQueue() { return m_computeQueue; }
    CommandQueue& GetCopyQueue() { return m_copyQueue; }

    void CreateNewCommandList(
        D3D12_COMMAND_LIST_TYPE type,
        ID3D12GraphicsCommandList** list,
        ID3D12CommandAllocator** allocator);

private:
    ID3D12Device* m_device = nullptr;
    CommandQueue m_graphicsQueue;
    CommandQueue m_computeQueue;
    CommandQueue m_copyQueue;
};