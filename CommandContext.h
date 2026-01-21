#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <vector>
#include <string>
#include <queue>
#include <mutex>

// 前方宣言
class CommandListManager;
class GpuResource;
class CommandContext;
class GraphicsContext;

// ==================================================================================
// コンテキスト管理プール
// ==================================================================================
class ContextManager {
public:
    CommandContext* AllocateContext(D3D12_COMMAND_LIST_TYPE type);
    void ReturnToPool(CommandContext* context, D3D12_COMMAND_LIST_TYPE type);

private:
    std::queue<CommandContext*> m_availableContexts[4];
    std::mutex m_contextMutex;
};

// ==================================================================================
// コマンドコンテキスト
// ==================================================================================
class CommandContext {
public:
    // コンテキストの取得と開始
    static CommandContext& Begin(const std::wstring& ID = L"");

    // コンテキストの終了と実行
    uint64_t Finish(bool waitForCompletion = false);

    // リソースバリア
    void TransitionResource(GpuResource& resource, D3D12_RESOURCE_STATES newState, bool flushImmediate = false);
    void FlushResourceBarriers();

    // コマンドリスト取得
    ID3D12GraphicsCommandList* GetCommandList() const { return m_commandList; }

    // ContextManager用のヘルパーメソッド（内部使用）
    void ResetForReuse(); // Reset()をpublicにラップ
    void SetDebugName(const std::wstring& name); // デバッグ名設定

protected:
    CommandContext(D3D12_COMMAND_LIST_TYPE type);
    virtual ~CommandContext();

    void Initialize();
    void Reset();

    CommandListManager* m_owningManager = nullptr;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandListPtr;
    ID3D12GraphicsCommandList* m_commandList = nullptr;
    ID3D12CommandAllocator* m_currentAllocator = nullptr;

    static const int MAX_RESOURCE_BARRIERS = 16;
    D3D12_RESOURCE_BARRIER m_resourceBarrierBuffer[MAX_RESOURCE_BARRIERS];
    uint32_t m_numBarriersToFlush = 0;

    D3D12_COMMAND_LIST_TYPE m_type;

    friend class ContextManager;
};

// ==================================================================================
// グラフィックスコンテキスト
// ==================================================================================
class GraphicsContext : public CommandContext {
public:
    static GraphicsContext& Begin(const std::wstring& ID = L"");

protected:
    GraphicsContext() : CommandContext(D3D12_COMMAND_LIST_TYPE_DIRECT) {}

    friend class ContextManager;
};