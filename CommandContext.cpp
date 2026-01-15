#include "CommandContext.h"
#include "GpuResource.h"
#include "CommandListManager.h"
#include "GraphicsCore.h"

#include <cassert>

// グローバルなマネージャインスタンス
static ContextManager g_ContextManager;

// ==================================================================================
// ContextManager 実装
// ==================================================================================

CommandContext* ContextManager::AllocateContext(D3D12_COMMAND_LIST_TYPE type) {
    std::lock_guard<std::mutex> lock(m_contextMutex);

    // 利用可能なコンテキストがあれば再利用
    auto& pool = m_availableContexts[type];
    if (!pool.empty()) {
        CommandContext* ret = pool.front();
        pool.pop();
        return ret;
    }

    // なければ新規作成
    CommandContext* ret = nullptr;
    if (type == D3D12_COMMAND_LIST_TYPE_DIRECT) {
        ret = new GraphicsContext();
    }
    else {
        ret = new CommandContext(type);
    }

    ret->Initialize();
    return ret;
}

void ContextManager::ReturnToPool(CommandContext* context, D3D12_COMMAND_LIST_TYPE type) {
    std::lock_guard<std::mutex> lock(m_contextMutex);
    m_availableContexts[type].push(context);
}

// ==================================================================================
// CommandContext 実装
// ==================================================================================

CommandContext::CommandContext(D3D12_COMMAND_LIST_TYPE type)
    : m_type(type)
    , m_commandList(nullptr)
    , m_currentAllocator(nullptr)
    , m_numBarriersToFlush(0)
    , m_owningManager(nullptr)
{
    m_owningManager = &GraphicsCore::GetInstance()->GetCommandListManager();
}

CommandContext::~CommandContext()
{
    m_commandListPtr.Reset();
}

void CommandContext::Initialize()
{
    ID3D12Device* device = GraphicsCore::GetInstance()->GetDevice();
    assert(device != nullptr);

    CommandQueue* queue = nullptr;
    switch (m_type) {
    case D3D12_COMMAND_LIST_TYPE_DIRECT:
        queue = &m_owningManager->GetGraphicsQueue();
        break;
    case D3D12_COMMAND_LIST_TYPE_COMPUTE:
        queue = &m_owningManager->GetComputeQueue();
        break;
    case D3D12_COMMAND_LIST_TYPE_COPY:
        queue = &m_owningManager->GetCopyQueue();
        break;
    }
    assert(queue != nullptr);

    ID3D12CommandAllocator* initialAllocator = queue->RequestAllocator();

    HRESULT hr = device->CreateCommandList(
        0, m_type, initialAllocator, nullptr, IID_PPV_ARGS(&m_commandListPtr));
    assert(SUCCEEDED(hr));

    m_commandList = m_commandListPtr.Get();

    switch (m_type) {
    case D3D12_COMMAND_LIST_TYPE_DIRECT:
        m_commandListPtr->SetName(L"GraphicsContext");
        break;
    case D3D12_COMMAND_LIST_TYPE_COMPUTE:
        m_commandListPtr->SetName(L"ComputeContext");
        break;
    case D3D12_COMMAND_LIST_TYPE_COPY:
        m_commandListPtr->SetName(L"CopyContext");
        break;
    }

    m_commandList->Close();
    queue->DiscardAllocator(0, initialAllocator);
}

void CommandContext::Reset()
{
    CommandQueue* queue = nullptr;
    switch (m_type) {
    case D3D12_COMMAND_LIST_TYPE_DIRECT:
        queue = &m_owningManager->GetGraphicsQueue();
        break;
    case D3D12_COMMAND_LIST_TYPE_COMPUTE:
        queue = &m_owningManager->GetComputeQueue();
        break;
    case D3D12_COMMAND_LIST_TYPE_COPY:
        queue = &m_owningManager->GetCopyQueue();
        break;
    }
    assert(queue != nullptr);

    m_currentAllocator = queue->RequestAllocator();

    HRESULT hr = m_commandList->Reset(m_currentAllocator, nullptr);
    assert(SUCCEEDED(hr));

    m_numBarriersToFlush = 0;
}

// ContextManager用のヘルパーメソッド
void CommandContext::ResetForReuse()
{
    Reset(); // protectedメソッドを呼び出す
}

void CommandContext::SetDebugName(const std::wstring& name)
{
    if (!name.empty() && m_commandList) {
        m_commandList->SetName(name.c_str());
    }
}

CommandContext& CommandContext::Begin(const std::wstring& ID)
{
    CommandContext* newContext = g_ContextManager.AllocateContext(D3D12_COMMAND_LIST_TYPE_DIRECT);
    newContext->ResetForReuse(); // publicメソッドを使用
    newContext->SetDebugName(ID); // publicメソッドを使用

    return *newContext;
}

uint64_t CommandContext::Finish(bool waitForCompletion)
{
    FlushResourceBarriers();

    HRESULT hr = m_commandList->Close();
    assert(SUCCEEDED(hr));

    CommandQueue* queue = nullptr;
    switch (m_type) {
    case D3D12_COMMAND_LIST_TYPE_DIRECT:
        queue = &m_owningManager->GetGraphicsQueue();
        break;
    case D3D12_COMMAND_LIST_TYPE_COMPUTE:
        queue = &m_owningManager->GetComputeQueue();
        break;
    case D3D12_COMMAND_LIST_TYPE_COPY:
        queue = &m_owningManager->GetCopyQueue();
        break;
    }
    assert(queue != nullptr);

    uint64_t fenceValue = queue->ExecuteCommandList(m_commandList);

    queue->DiscardAllocator(fenceValue, m_currentAllocator);
    m_currentAllocator = nullptr;

    if (waitForCompletion) {
        queue->WaitForFence(fenceValue);
    }

    g_ContextManager.ReturnToPool(this, m_type);

    return fenceValue;
}

void CommandContext::TransitionResource(GpuResource& resource, D3D12_RESOURCE_STATES newState, bool flushImmediate)
{
    D3D12_RESOURCE_STATES oldState = resource.m_UsageState;

    if (oldState != newState)
    {
        assert(m_numBarriersToFlush < MAX_RESOURCE_BARRIERS);

        D3D12_RESOURCE_BARRIER& barrierDesc = m_resourceBarrierBuffer[m_numBarriersToFlush++];

        barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrierDesc.Transition.pResource = resource.GetResource();
        barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrierDesc.Transition.StateBefore = oldState;
        barrierDesc.Transition.StateAfter = newState;

        resource.m_UsageState = newState;
    }

    if (flushImmediate || m_numBarriersToFlush == MAX_RESOURCE_BARRIERS)
    {
        FlushResourceBarriers();
    }
}

void CommandContext::FlushResourceBarriers()
{
    if (m_numBarriersToFlush > 0)
    {
        m_commandList->ResourceBarrier(m_numBarriersToFlush, m_resourceBarrierBuffer);
        m_numBarriersToFlush = 0;
    }
}

// ==================================================================================
// GraphicsContext 実装
// ==================================================================================

GraphicsContext& GraphicsContext::Begin(const std::wstring& ID)
{
    CommandContext* newContext = g_ContextManager.AllocateContext(D3D12_COMMAND_LIST_TYPE_DIRECT);
    newContext->ResetForReuse(); // publicメソッドを使用
    newContext->SetDebugName(ID); // publicメソッドを使用

    return static_cast<GraphicsContext&>(*newContext);
}