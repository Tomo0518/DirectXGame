#include "CommandListManager.h"
#include <cassert>

// --- CommandAllocatorPool Implementation ---

CommandAllocatorPool::CommandAllocatorPool(D3D12_COMMAND_LIST_TYPE type)
    : m_cListType(type), m_device(nullptr) {
}

CommandAllocatorPool::~CommandAllocatorPool() { Shutdown(); }

void CommandAllocatorPool::Create(ID3D12Device* device) {
    m_device = device;
}

void CommandAllocatorPool::Shutdown() {
    m_allocatorPool.clear();
}

ID3D12CommandAllocator* CommandAllocatorPool::RequestAllocator(uint64_t completedFenceValue) {
    std::lock_guard<std::mutex> lock(m_allocatorMutex);

    ID3D12CommandAllocator* allocator = nullptr;

    if (!m_readyAllocators.empty()) {
        std::pair<uint64_t, ID3D12CommandAllocator*>& allocatorPair = m_readyAllocators.front();

        // GPUの実行が完了しているか確認
        if (allocatorPair.first <= completedFenceValue) {
            allocator = allocatorPair.second;
            allocator->Reset();
            m_readyAllocators.pop();
        }
    }

    // 再利用できるものがなければ新規作成
    if (allocator == nullptr) {
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> newAllocator;
        HRESULT hr = m_device->CreateCommandAllocator(m_cListType, IID_PPV_ARGS(&newAllocator));
        assert(SUCCEEDED(hr));
        newAllocator->SetName(L"CommandAllocator");
        allocator = newAllocator.Get();
        m_allocatorPool.push_back(newAllocator);
    }

    return allocator;
}

void CommandAllocatorPool::DiscardAllocator(uint64_t fenceValue, ID3D12CommandAllocator* allocator) {
    std::lock_guard<std::mutex> lock(m_allocatorMutex);
    m_readyAllocators.push(std::make_pair(fenceValue, allocator));
}

// --- CommandQueue Implementation ---

CommandQueue::CommandQueue(D3D12_COMMAND_LIST_TYPE type)
    : m_type(type), m_allocatorPool(type) {
}

CommandQueue::~CommandQueue() { Shutdown(); }

void CommandQueue::Create(ID3D12Device* device) {
    assert(device != nullptr);
    m_allocatorPool.Create(device);

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = m_type;
    queueDesc.NodeMask = 1;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    HRESULT hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue));
    assert(SUCCEEDED(hr));
    m_commandQueue->SetName(L"CommandQueue");

    hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
    assert(SUCCEEDED(hr));
    m_fence->SetName(L"CommandQueueFence");
    m_fence->Signal(0);

    m_fenceEventHandle = CreateEvent(nullptr, false, false, nullptr);
    assert(m_fenceEventHandle != nullptr);
}

void CommandQueue::Shutdown() {
    if (m_commandQueue == nullptr) return;
    m_allocatorPool.Shutdown();
    CloseHandle(m_fenceEventHandle);
    m_fence.Reset();
    m_commandQueue.Reset();
}

uint64_t CommandQueue::ExecuteCommandList(ID3D12CommandList* list) {
    m_commandQueue->ExecuteCommandLists(1, &list);

    std::lock_guard<std::mutex> lock(m_fenceMutex);
    m_commandQueue->Signal(m_fence.Get(), m_nextFenceValue);
    return m_nextFenceValue++;
}

ID3D12CommandAllocator* CommandQueue::RequestAllocator() {
    uint64_t completedFence = m_fence->GetCompletedValue();
    return m_allocatorPool.RequestAllocator(completedFence);
}

void CommandQueue::DiscardAllocator(uint64_t fenceValueForReset, ID3D12CommandAllocator* allocator) {
    m_allocatorPool.DiscardAllocator(fenceValueForReset, allocator);
}

void CommandQueue::WaitForFence(uint64_t fenceValue) {
    if (IsFenceComplete(fenceValue)) return;

    std::lock_guard<std::mutex> lock(m_eventMutex);
    if (IsFenceComplete(fenceValue)) return;

    m_fence->SetEventOnCompletion(fenceValue, m_fenceEventHandle);
    WaitForSingleObject(m_fenceEventHandle, INFINITE);
    m_lastCompletedFenceValue = fenceValue;
}

bool CommandQueue::IsFenceComplete(uint64_t fenceValue) {
    if (fenceValue <= m_lastCompletedFenceValue) return true;
    m_lastCompletedFenceValue = (std::max)(m_lastCompletedFenceValue, m_fence->GetCompletedValue());
    return fenceValue <= m_lastCompletedFenceValue;
}

// --- CommandListManager Implementation ---

CommandListManager::CommandListManager()
    : m_graphicsQueue(D3D12_COMMAND_LIST_TYPE_DIRECT)
    , m_computeQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE)
    , m_copyQueue(D3D12_COMMAND_LIST_TYPE_COPY) {
}

CommandListManager::~CommandListManager() { Shutdown(); }

void CommandListManager::Create(ID3D12Device* device) {
    m_device = device;
    m_graphicsQueue.Create(device);
    m_computeQueue.Create(device);
    m_copyQueue.Create(device);
}

void CommandListManager::Shutdown() {
    m_graphicsQueue.Shutdown();
    m_computeQueue.Shutdown();
    m_copyQueue.Shutdown();
}

void CommandListManager::CreateNewCommandList(
    D3D12_COMMAND_LIST_TYPE type,
    ID3D12GraphicsCommandList** list,
    ID3D12CommandAllocator** allocator)
{
    switch (type) {
    case D3D12_COMMAND_LIST_TYPE_DIRECT: *allocator = m_graphicsQueue.RequestAllocator(); break;
    case D3D12_COMMAND_LIST_TYPE_COMPUTE: *allocator = m_computeQueue.RequestAllocator(); break;
    case D3D12_COMMAND_LIST_TYPE_COPY: *allocator = m_copyQueue.RequestAllocator(); break;
    }

    HRESULT hr = m_device->CreateCommandList(1, type, *allocator, nullptr, IID_PPV_ARGS(list));
    assert(SUCCEEDED(hr));
    (*list)->SetName(L"CommandList");
}