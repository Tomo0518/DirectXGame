#pragma once
#include <d3d12.h>

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
		return resource_;
	}

private:
	ID3D12Resource* resource_ = nullptr;
};

