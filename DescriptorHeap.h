#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <vector>
#include <queue>
#include <mutex>
#include <cstdint>

// ==================================================================================
// DescriptorHandle
// CPU/GPUハンドルをペアで管理するヘルパー構造体
// ==================================================================================
struct DescriptorHandle {
    D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle;

    DescriptorHandle() {
        // ハンドルを 0 (無効値) で初期化
        CpuHandle.ptr = 0;
        GpuHandle.ptr = 0;
    }

    DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE gpu)
        : CpuHandle(cpu), GpuHandle(gpu) {
    }

    // ハンドルが有効かチェック（0でなければ有効）
    bool IsValid() const { return CpuHandle.ptr != 0; }

    // シェーダーから見える（GPUハンドルがある）かチェック
    bool IsShaderVisible() const { return GpuHandle.ptr != 0; }
};

// ==================================================================================
// DescriptorAllocator
// RTV, DSV, SRVなどのディスクリプタをヒープから切り出すアロケータ
// ==================================================================================
class DescriptorAllocator {
public:
    DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type);
    ~DescriptorAllocator();

    // 初期化と破棄
    void Create(ID3D12Device* device);
    void Shutdown();

    // ハンドルの確保
    DescriptorHandle Allocate(uint32_t count = 1);

private:
    D3D12_DESCRIPTOR_HEAP_TYPE m_Type;
    ID3D12Device* m_Device = nullptr;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_Heap;

    uint32_t m_DescriptorSize = 0;
    uint32_t m_NumDescriptorsPerHeap = 256;
    uint32_t m_RemainingFreeHandles = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE m_CurrentCpuHandle = {};
    D3D12_GPU_DESCRIPTOR_HANDLE m_CurrentGpuHandle = {};

    std::mutex m_AllocationMutex;
};

class DescriptorHeap {
public:
    // コンストラクタ
    DescriptorHeap() = default;
    ~DescriptorHeap() = default;

    // ヒープの作成
    // shaderVisible: テクスチャやImGui用なら true, RTV用なら false
    void Create(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT numDescriptors, bool shaderVisible);

    // ディスクリプタを1つ割り当てて、そのハンドルを返す
    // 戻り値: { CPUハンドル, GPUハンドル }
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> Allocate();

    // ヒープ自体の取得（SetDescriptorHeaps用）
    ID3D12DescriptorHeap* GetHeap() const { return heap_.Get(); }

    // 開始位置の取得（必要な場合用）
    D3D12_CPU_DESCRIPTOR_HANDLE GetStartCpuHandle() const { return startCpuHandle_; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetStartGpuHandle() const { return startGpuHandle_; }

private:
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap_;
    D3D12_CPU_DESCRIPTOR_HANDLE startCpuHandle_{};
    D3D12_GPU_DESCRIPTOR_HANDLE startGpuHandle_{};
    UINT descriptorSize_ = 0;
    UINT capacity_ = 0;
    UINT current_ = 0; // 現在どこまで使ったか
};