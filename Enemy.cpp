#include "Enemy.h"
#include "GraphicsCore.h"

Enemy::Enemy() {}
Enemy::~Enemy() {}

void Enemy::Initialize(Model* model, const Vector3& position) {
	worldTransform_.Initialize(GraphicsCore::GetInstance()->GetDevice());

	// 位置を設定
	worldTransform_.translation_ = position;
	worldTransform_.scale_ = { 1.0f, 1.0f, 1.0f };
	worldTransform_.rotation_ = { 0.0f, 0.0f, 0.0f };

	model_ = model;
}

void Enemy::Update(const Camera& camera) {
	worldTransform_.UpdateMatrix(camera);
}

void Enemy::Draw(ID3D12GraphicsCommandList* list) {
	model_->Draw(list, worldTransform_);
}