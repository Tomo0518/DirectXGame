#pragma once
#include <d3d12.h>
#include <wrl.h>

class ResourceObject {
public:
	ResourceObject(ID3D12Resource* resource)
		:resource_(resource)
	{
	}

	~ResourceObject() {
		if (resource_) {
			resource_->Release();
			resource_ = nullptr;
		}
	}

	ID3D12Resource* Get() const {
		return resource_.Get();
	}

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> resource_ = nullptr;
};