#include "ColorBuffer.h"
#include "GraphicsCore.h"

void ColorBuffer::CreateFromSwapChain(const std::wstring& name, ID3D12Resource* baseResource)
{
    // 既存リソース(SwapChain Buffer)に関連付け
    m_pResource = baseResource;

    // スワップチェーンは初期状態が PRESENT (Common)
    m_UsageState = D3D12_RESOURCE_STATE_PRESENT;

    // リソース情報の取得
    D3D12_RESOURCE_DESC desc = baseResource->GetDesc();
    m_Width = (uint32_t)desc.Width;
    m_Height = (uint32_t)desc.Height;
    m_ArraySize = (uint32_t)desc.DepthOrArraySize;
    m_Format = desc.Format;

    SetName(name);

    // RTVハンドルの確保（GraphicsCoreのアロケータを使用）
    m_RTVHandle = GraphicsCore::GetInstance()->GetRTVAllocator().Allocate();

    // RTVの作成
    GraphicsCore::GetInstance()->GetDevice()->CreateRenderTargetView(
        m_pResource.Get(), nullptr, m_RTVHandle.CpuHandle);
}