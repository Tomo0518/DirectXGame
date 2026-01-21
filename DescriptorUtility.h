#pragma once

#include <d3d12.h>
#include <cstdint>

D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(
	ID3D12DescriptorHeap* descriptorHeap,
	uint32_t descriptorSize,
	uint32_t index
);

D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(
	ID3D12DescriptorHeap* descriptorHeap,
	uint32_t descriptorSize,
	uint32_t index
);