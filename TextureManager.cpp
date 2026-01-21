#include "TextureManager.h"
#include "GraphicsCore.h"
#include "ResourcesUtility.h"
#include <cassert>

TextureManager* TextureManager::GetInstance() {
    static TextureManager instance;
    return &instance;
}

void TextureManager::Initialize(DescriptorHeap* srvHeap) {
    assert(srvHeap != nullptr);
    m_srvHeap = srvHeap;
}

const Texture* TextureManager::Load(const std::string& filePath, ID3D12GraphicsCommandList* commandList) {
    // 既に読み込み済みならそれを返す
    auto it = m_textures.find(filePath);
    if (it != m_textures.end()) {
        return &it->second;
    }

    // 新規読み込み
    Texture newTexture;

    // テクスチャファイルの読み込み
    DirectX::ScratchImage mipImages = LoadTexture(filePath);
    const DirectX::TexMetadata& metadata = mipImages.GetMetadata();

    // テクスチャリソースの作成
    ID3D12Device* device = GraphicsCore::GetInstance()->GetDevice();
    newTexture.resource = CreateTextureResource(device, metadata);

    // データ転送の戻り値（中間リソース）を受け取って保持
    ResourceObject intermediateResource = UploadTextureData(newTexture.resource, mipImages, device, commandList);
    m_intermediateResources.push_back(intermediateResource);

    // SRVの作成（DescriptorHeapから確保）
    auto [cpuHandle, gpuHandle] = m_srvHeap->Allocate();
    newTexture.cpuHandle = cpuHandle;
    newTexture.gpuHandle = gpuHandle;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = metadata.format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = static_cast<UINT>(metadata.mipLevels);

    device->CreateShaderResourceView(newTexture.resource.Get(), &srvDesc, cpuHandle);

    // マップに登録
    m_textures[filePath] = newTexture;

    return &m_textures[filePath];
}

const Texture* TextureManager::GetTexture(const std::string& filePath) const {
    auto it = m_textures.find(filePath);
    return (it != m_textures.end()) ? &it->second : nullptr;
}