#pragma once
#include <wrl/client.h>
#include <d3d12.h>

class ResourceObject
{
public:
    ResourceObject() = default;
    ResourceObject(Microsoft::WRL::ComPtr<ID3D12Resource> resource)
        : m_resource(resource) {}

    // 必要に応じて他のメンバ関数やアクセサを追加
    Microsoft::WRL::ComPtr<ID3D12Resource> Get() const { return m_resource; }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;
};