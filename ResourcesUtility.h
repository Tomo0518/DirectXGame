#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <string>
#include <vector>

// DirectXTexのインクルード
#include "externals/DirectXTex/DirectXTex.h"

	// 256バイトアライメント計算
	// 定数バッファのサイズ調整などで頻繁に使用します
inline size_t Align256(size_t size) {
	return (size + 255) & ~size_t(255);
}

// バッファリソース（Upload Heap）の生成
Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(
	const Microsoft::WRL::ComPtr<ID3D12Device>& device,
	size_t sizeInBytes
);

// テクスチャリソース（Default Heap）の生成
Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(
	const Microsoft::WRL::ComPtr<ID3D12Device>& device,
	const DirectX::TexMetadata& metadata
);

// 深度ステンシルテクスチャの生成
Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(
	const Microsoft::WRL::ComPtr<ID3D12Device>& device,
	int32_t width,
	int32_t height
);

// テクスチャファイルの読み込み（DirectXTex使用）
// ミップマップ生成まで行います
DirectX::ScratchImage LoadTexture(const std::string& filePath);

// テクスチャデータのアップロード
// 中間バッファを生成し、Copyコマンドを積みます。
// 戻り値の Resource(中間バッファ) は、コマンド実行完了まで保持する必要があります。
[[nodiscard]]
Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(
	const Microsoft::WRL::ComPtr<ID3D12Resource>& texture,
	const DirectX::ScratchImage& mipImages,
	const Microsoft::WRL::ComPtr<ID3D12Device>& device,
	const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& commandList
);