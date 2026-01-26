#include "Skydome.h"
#include "Math.h"
#include "GraphicsCore.h"

Skydome::Skydome() {}

Skydome::~Skydome() { delete model_; }

void Skydome::Initialize(Model* model, Camera* camera) {
	model_ = model;
	camera_ = camera;

	worldTransform_.Initialize(GraphicsCore::GetInstance()->GetDevice());
	worldTransform_.scale_ = { 20.0f, 20.0f, 20.0f };
	worldTransform_.translation_ = { 0.0f, 0.0f, 0.0f };
}

void Skydome::Update() { 	// カメラの位置に追従させる
	worldTransform_.translation_ = camera_->GetTranslation();
	worldTransform_.UpdateMatrix(*camera_);
}

void Skydome::Draw(ID3D12GraphicsCommandList* list) { model_->Draw(list,worldTransform_); }