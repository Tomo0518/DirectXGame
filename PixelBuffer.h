#pragma once
#include "GpuResource.h"
#include <cstdint>
#include <dxgiformat.h>

class PixelBuffer : public GpuResource
{
public:
    PixelBuffer() : m_Width(0), m_Height(0), m_ArraySize(0), m_Format(DXGI_FORMAT_UNKNOWN) {}

    uint32_t GetWidth() const { return m_Width; }
    uint32_t GetHeight() const { return m_Height; }
    uint32_t GetDepth() const { return m_ArraySize; }
    const DXGI_FORMAT& GetFormat() const { return m_Format; }

protected:
    uint32_t m_Width;
    uint32_t m_Height;
    uint32_t m_ArraySize;
    DXGI_FORMAT m_Format;

    // リソース記述子の生成ヘルパー
    D3D12_RESOURCE_DESC DescribeTex2D(uint32_t width, uint32_t height, uint32_t depthOrArraySize, uint32_t numMips, DXGI_FORMAT format, UINT flags)
    {
        m_Width = width;
        m_Height = height;
        m_ArraySize = depthOrArraySize;
        m_Format = format;

        D3D12_RESOURCE_DESC Desc = {};
        Desc.Alignment = 0;
        Desc.DepthOrArraySize = (UINT16)depthOrArraySize;
        Desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        Desc.Flags = (D3D12_RESOURCE_FLAGS)flags;
        Desc.Format = GetBaseFormat(format);
        Desc.Height = (UINT)height;
        Desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        Desc.MipLevels = (UINT16)numMips;
        Desc.SampleDesc.Count = 1;
        Desc.SampleDesc.Quality = 0;
        Desc.Width = (UINT64)width;
        return Desc;
    }

    static DXGI_FORMAT GetBaseFormat(DXGI_FORMAT defaultFormat) { return defaultFormat; }
};