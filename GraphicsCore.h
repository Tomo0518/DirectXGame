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

    // 初期化：ウィンドウ生成後に呼び出す
    void Initialize(HWND windowHandle, int width, int height);

    // 仮実装(のちに排除します)
    void Initialize();

    // 終了処理
    void Shutdown();

    // 画面フリップと同期
    void Present();

    // ゲッター
    ID3D12Device* GetDevice() const { return device_.Get(); }
    IDXGIFactory7* GetFactory() const { return dxgiFactory_.Get(); }
    // HRESULTは内部処理用なので公開しなくてよいが、デバッグ用に残しても可
    // HRESULT GetHr() { return hr_; } 

    // マネージャーへのアクセサ
    CommandListManager& GetCommandListManager() { return commandListManager_; }
    CommandQueue& GetGraphicsQueue() { return commandListManager_.GetGraphicsQueue(); }

    // アロケータへのアクセサ（GraphicsContextなどで使用）
    DescriptorAllocator& GetRTVAllocator() { return m_RTVAllocator_; }
    DescriptorAllocator& GetDSVAllocator() { return m_DSVAllocator_; }
    DescriptorAllocator& GetSRVAllocator() { return m_SRVAllocator_; }

    // 現在のバックバッファ（描画対象）を取得
    ColorBuffer& GetBackBuffer() { return m_DisplayPlane_[m_CurrentBackBufferIndex_]; }
    // 深度バッファを取得
    DepthBuffer& GetDepthBuffer() { return m_DepthBuffer_; }

	HRESULT& GetHr() { return hr_; }

private:
    GraphicsCore() = default;
    ~GraphicsCore() = default;
    GraphicsCore(const GraphicsCore&) = delete;
    GraphicsCore& operator=(const GraphicsCore&) = delete;

    HRESULT hr_ = S_OK;

    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_;

#ifdef _DEBUG
    Microsoft::WRL::ComPtr<ID3D12Debug1> debugController_;
    Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue_;
#endif

    // コマンドリストマネージャー
    CommandListManager commandListManager_;

    // ディスクリプタアロケータ
    DescriptorAllocator m_RTVAllocator_{ D3D12_DESCRIPTOR_HEAP_TYPE_RTV };
    DescriptorAllocator m_DSVAllocator_{ D3D12_DESCRIPTOR_HEAP_TYPE_DSV };
    DescriptorAllocator m_SRVAllocator_{ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV };

    // スワップチェーンとバッファ
    static const uint32_t BufferCount = 3; // 3重バッファリング推奨（最低2）
    uint32_t m_CurrentBackBufferIndex_ = 0;

    Microsoft::WRL::ComPtr<IDXGISwapChain4> m_SwapChain_;
    ColorBuffer m_DisplayPlane_[BufferCount];
    DepthBuffer m_DepthBuffer_;

    // フレーム同期用
    uint64_t m_CurrentFenceValue_ = 0;
};