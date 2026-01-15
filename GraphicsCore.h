#pragma once
// 既存のインクルード
#include "ResourceObject.h"
#include <wrl/client.h>
#include <string>
#include <dxgi1_6.h>
#pragma comment(lib, "dxgi.lib")

// 【変更点】CommandQueue/Context単体ではなくマネージャーをインクルード
#include "CommandListManager.h"

class GraphicsCore {
public:
    static GraphicsCore* GetInstance();

    void Initialize();
    void Shutdown();

    ID3D12Device* GetDevice() const { return device_.Get(); }
    IDXGIFactory7* GetFactory() const { return dxgiFactory_.Get(); }
    HRESULT GetHr() { return hr_; }

    // マネージャーへのアクセサ
    CommandListManager& GetCommandListManager() { return commandListManager_; }

    CommandQueue& GetGraphicsQueue() { return commandListManager_.GetGraphicsQueue(); }

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

    CommandListManager commandListManager_;
};