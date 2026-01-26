#pragma once
#include <d3d12.h>
#include <string>
#include <unordered_map>
#include "DescriptorHeap.h"
#include "ResourceObject.h"

// テクスチャ情報を保持する構造体
struct Texture {
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
};

class TextureManager {
public:
    static TextureManager* GetInstance();

    // DescriptorHeapを使って初期化
    void Initialize(DescriptorHeap* srvHeap);

    // テクスチャの読み込み（戻り値を[[nodiscard]]にする）
    [[nodiscard]] const Texture* Load(const std::string& filePath, ID3D12GraphicsCommandList* commandList);

    // テクスチャの取得
    const Texture* GetTexture(const std::string& filePath) const;

private:
    TextureManager() = default;
    ~TextureManager() = default;
    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;

    DescriptorHeap* m_srvHeap = nullptr;
    std::unordered_map<std::string, Texture> m_textures;

    // テクスチャ転送用の中間リソースを保持
    std::vector<ResourceObject> m_intermediateResources;
};