#pragma once
#include "PixelBuffer.h"
#include "DescriptorHeap.h"
#include <d3d12.h>
#include <dxgi1_6.h>  // ← これを追加

class ColorBuffer : public PixelBuffer
{
public:
    // デフォルトコンストラクタ
    ColorBuffer()
    {
        m_ClearColor[0] = 0.0f;
        m_ClearColor[1] = 0.0f;
        m_ClearColor[2] = 0.0f;
        m_ClearColor[3] = 0.0f;
    }

    // カラー指定コンストラクタ
    ColorBuffer(float r, float g, float b, float a = 1.0f)
    {
        m_ClearColor[0] = r;
        m_ClearColor[1] = g;
        m_ClearColor[2] = b;
        m_ClearColor[3] = a;
    }

    void CreateFromSwapChain(const std::wstring& name, ID3D12Resource* baseResource);

    const D3D12_CPU_DESCRIPTOR_HANDLE& GetRTV() const { return m_RTVHandle.CpuHandle; }
    const D3D12_CPU_DESCRIPTOR_HANDLE& GetSRV() const { return m_SRVHandle.CpuHandle; }

    const float* GetClearColor() const { return m_ClearColor; }
    void SetClearColor(float r, float g, float b, float a = 1.0f) {
        m_ClearColor[0] = r;
        m_ClearColor[1] = g;
        m_ClearColor[2] = b;
        m_ClearColor[3] = a;
    }

protected:
    float m_ClearColor[4];  // RGBA (0.0f～1.0f)
    DescriptorHandle m_RTVHandle;
    DescriptorHandle m_SRVHandle;
};