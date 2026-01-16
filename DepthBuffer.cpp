#include "DepthBuffer.h"
#include "GraphicsCore.h"
#include <cassert>

void DepthBuffer::Create(const std::wstring& name, uint32_t width, uint32_t height, DXGI_FORMAT format)
{
    D3D12_RESOURCE_DESC ResourceDesc = DescribeTex2D(width, height, 1, 1, format, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    // クリア値の設定
    D3D12_CLEAR_VALUE ClearValue = {};
    ClearValue.Format = format;
    ClearValue.DepthStencil.Depth = m_ClearDepth;
    ClearValue.DepthStencil.Stencil = m_ClearStencil;

    // ヒーププロパティ
    D3D12_HEAP_PROPERTIES HeapProps = {};
    HeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProps.CreationNodeMask = 1;
    HeapProps.VisibleNodeMask = 1;

    // リソース生成
    HRESULT hr = GraphicsCore::GetInstance()->GetDevice()->CreateCommittedResource(
        &HeapProps,
        D3D12_HEAP_FLAG_NONE,
        &ResourceDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE, // 初期状態
        &ClearValue,
        IID_PPV_ARGS(&m_pResource)
    );
    assert(SUCCEEDED(hr));

    m_UsageState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    SetName(name);

    // DSVハンドルの確保
    m_DSVHandle = GraphicsCore::GetInstance()->GetDSVAllocator().Allocate();

    // DSV作成
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = format;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;

    GraphicsCore::GetInstance()->GetDevice()->CreateDepthStencilView(
        m_pResource.Get(), &dsvDesc, m_DSVHandle.CpuHandle);
}