#include "DescriptorHeap.h"
#include <cassert>
#include <stdexcept>

DescriptorAllocator::DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type)
    : m_Type(type)
    , m_Device(nullptr)
    , m_RemainingFreeHandles(0)
{
}

DescriptorAllocator::~DescriptorAllocator()
{
    Shutdown();
}

void DescriptorAllocator::Create(ID3D12Device* device)
{
    m_Device = device;
    m_DescriptorSize = device->GetDescriptorHandleIncrementSize(m_Type);
    m_NumDescriptorsPerHeap = 1024; // デフォルトサイズ

    // ヒープの作成
    D3D12_DESCRIPTOR_HEAP_DESC Desc = {};
    Desc.Type = m_Type;
    Desc.NumDescriptors = m_NumDescriptorsPerHeap;
    Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // CPU可視のみ（動的バインド用は別途実装）
    Desc.NodeMask = 1;

    // シェーダー可視が必要なタイプ（CBV/SRV/UAV, Sampler）の場合のフラグ設定
    // ※今回はリソース生成用（RTV/DSV）を主眼に置くため NONE で進めますが、
    //   SRV用には SHADER_VISIBLE が必要になる場合があります。
    //   MiniEngineでは "Staging" と "ShaderVisible" を明確に分けます。

    HRESULT hr = m_Device->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&m_Heap));
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create descriptor heap");
    }
    m_Heap->SetName(L"DescriptorAllocator Heap");

    m_CurrentCpuHandle = m_Heap->GetCPUDescriptorHandleForHeapStart();

    // ShaderVisibleでない場合、GPUハンドルは取得できない（ptr=0になる）
    if (Desc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) {
        m_CurrentGpuHandle = m_Heap->GetGPUDescriptorHandleForHeapStart();
    }
    else {
        m_CurrentGpuHandle.ptr = 0;
    }

    m_RemainingFreeHandles = m_NumDescriptorsPerHeap;
}

void DescriptorAllocator::Shutdown()
{
    m_Heap.Reset();
}

DescriptorHandle DescriptorAllocator::Allocate(uint32_t count)
{
    std::lock_guard<std::mutex> lock(m_AllocationMutex);

    if (m_RemainingFreeHandles < count) {
        // ここで新しいヒープを確保するロジックが必要ですが、
        // まずはアサートで停止させます。実用段階ではプール化が必要です。
        assert(false && "Descriptor Heap out of memory!");
        return DescriptorHandle();
    }

    DescriptorHandle ret;
    ret.CpuHandle = m_CurrentCpuHandle;
    ret.GpuHandle = m_CurrentGpuHandle;

    // ポインタを進める
    m_CurrentCpuHandle.ptr += (uint64_t)count * m_DescriptorSize;
    if (ret.IsShaderVisible()) {
        m_CurrentGpuHandle.ptr += (uint64_t)count * m_DescriptorSize;
    }
    m_RemainingFreeHandles -= count;

    return ret;
}