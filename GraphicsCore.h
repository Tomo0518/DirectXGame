#pragma once
#include "ResourceObject.h"
#include <wrl/client.h>
#include <string>
#include <dxgi1_6.h>
#pragma comment(lib, "dxgi.lib")

#include "DescriptorHeap.h"
#include "CommandListManager.h"

#include "ColorBuffer.h"
#include "DepthBuffer.h"

class GraphicsCore {
public:
    static GraphicsCore* GetInstance();

    void Initialize(void* windowHandle, int width, int height);
    void Initialize();
    void Shutdown();

    ID3D12Device* GetDevice() const { return device_.Get(); }
    IDXGIFactory7* GetFactory() const { return dxgiFactory_.Get(); }
    HRESULT GetHr() { return hr_; }

    // マネージャーへのアクセサ
    CommandListManager& GetCommandListManager() { return commandListManager_; }

    CommandQueue& GetGraphicsQueue() { return commandListManager_.GetGraphicsQueue(); }

    // アロケータへのアクセサ
    DescriptorAllocator& GetRTVAllocator() { return m_RTVAllocator; }
    DescriptorAllocator& GetDSVAllocator() { return m_DSVAllocator; }
    DescriptorAllocator& GetSRVAllocator() { return m_SRVAllocator; } // リソース生成用

    // 現在のバックバッファ（描画対象）を取得する便利関数
    ColorBuffer& GetBackBuffer() { return m_DisplayPlane[m_CurrentBackBufferIndex]; }
    DepthBuffer& GetDepthBuffer() { return m_DepthBuffer; }

private:
    GraphicsCore() = default;
    ~GraphicsCore() = default;
    GraphicsCore(const GraphicsCore&) = delete;
    GraphicsCore& operator=(const GraphicsCore&) = delete;

    HRESULT hr_;

    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_;

#ifdef _DEBUG
    Microsoft::WRL::ComPtr<ID3D12Debug1> debugController_;
    Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue_;
#endif

	// コマンドリストマネージャー
    CommandListManager commandListManager_;

	// ディスクリプタアロケータ
    DescriptorAllocator m_RTVAllocator{ D3D12_DESCRIPTOR_HEAP_TYPE_RTV };
    DescriptorAllocator m_DSVAllocator{ D3D12_DESCRIPTOR_HEAP_TYPE_DSV };
    DescriptorAllocator m_SRVAllocator{ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV };

    // スワップチェーンとバッファ
    static const uint32_t BufferCount = 3; // 3重バッファリング
    uint32_t m_CurrentBackBufferIndex = 0;

    Microsoft::WRL::ComPtr<IDXGISwapChain4> m_SwapChain;
    ColorBuffer m_DisplayPlane[BufferCount]; // バックバッファのラッパー
    DepthBuffer m_DepthBuffer;               // 深度バッファ
};