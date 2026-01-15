#pragma once
#include <d3d12.h>
#include <wrl/client.h>

// GPU仮想アドレスが未定義の場合の対応
#ifndef D3D12_GPU_VIRTUAL_ADDRESS_NULL
#define D3D12_GPU_VIRTUAL_ADDRESS_NULL ((D3D12_GPU_VIRTUAL_ADDRESS)0)
#endif

// 前方宣言
class CommandContext;

// ==================================================================================
// GPUリソース基底クラス
// リソースの状態(State)を追跡し、バリアの自動化をサポートする
// ==================================================================================
class GpuResource
{
    // CommandContextが直接 m_UsageState を書き換えられるようにする
    friend class CommandContext;

public:
    GpuResource()
        : m_UsageState(D3D12_RESOURCE_STATE_COMMON)
        , m_GpuVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS_NULL)
    {
    }

    virtual ~GpuResource() { Destroy(); }

    virtual void Destroy() {
        m_pResource.Reset();
        m_UsageState = D3D12_RESOURCE_STATE_COMMON;
        m_GpuVirtualAddress = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
    }

    // 生のリソースポインタ取得（描画コマンド設定用）
    ID3D12Resource* GetResource() { return m_pResource.Get(); }
    const ID3D12Resource* GetResource() const { return m_pResource.Get(); }

    // 現在の状態を取得
    D3D12_RESOURCE_STATES GetUsageState() const { return m_UsageState; }

    // GPU仮想アドレス（Constant Buffer Viewなどで使用）
    D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const { return m_GpuVirtualAddress; }

protected:
    // リソースの実体（ComPtrで管理し、メモリリークを防ぐ）
    Microsoft::WRL::ComPtr<ID3D12Resource> m_pResource;

    // 現在のリソースステート（バリア判定の肝）
    D3D12_RESOURCE_STATES m_UsageState;

    D3D12_GPU_VIRTUAL_ADDRESS m_GpuVirtualAddress;
};