#pragma once
#include "PixelBuffer.h"
#include "DescriptorHeap.h"

class DepthBuffer : public PixelBuffer
{
public:
    DepthBuffer(float clearDepth = 1.0f, uint8_t clearStencil = 0)
        : m_ClearDepth(clearDepth), m_ClearStencil(clearStencil)
    {
    }

    // 深度バッファの生成
    void Create(const std::wstring& name, uint32_t width, uint32_t height, DXGI_FORMAT format);

    // ビューハンドルの取得
    const D3D12_CPU_DESCRIPTOR_HANDLE& GetDSV() const { return m_DSVHandle.CpuHandle; }

    float GetClearDepth() const { return m_ClearDepth; }
    uint8_t GetClearStencil() const { return m_ClearStencil; }

protected:
    float m_ClearDepth;
    uint8_t m_ClearStencil;
    DescriptorHandle m_DSVHandle;
};