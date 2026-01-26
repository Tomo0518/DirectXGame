#pragma once
#include "Camera.h"
#include "WorldTransform.h"
#include "Model.h"

class Enemy{
public:
	Enemy();
	~Enemy();
	void Initialize(Model* model, const Vector3& position);
	void Update(const Camera& camera);
	void Draw(ID3D12GraphicsCommandList* list);

	WorldTransform& GetWorldTransform() { return worldTransform_; }
private:
	WorldTransform worldTransform_;
	Model* model_ = nullptr;
	Camera* camera_ = nullptr;
};