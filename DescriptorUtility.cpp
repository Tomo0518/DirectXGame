#include "DescriptorUtility.h"

D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(
	ID3D12DescriptorHeap* descriptorHeap,
	uint32_t descriptorSize,
	uint32_t index
) {
	// デスクリプタヒープの先頭ハンドルを取得
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	// index分だけオフセットを加算
	handleCPU.ptr += UINT64(descriptorSize) * index;
	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(
	ID3D12DescriptorHeap* descriptorHeap,
	uint32_t descriptorSize,
	uint32_t index
) {
	// デスクリプタヒープの先頭ハンドルを取得
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	// index分だけオフセットを加算
	handleGPU.ptr += UINT64(descriptorSize) * index;
	return handleGPU;
}