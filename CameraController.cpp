#include "CameraController.h"
#include "Player.h"

// Lerp関数用
#include <cmath>
#include <algorithm>

CameraController::CameraController() {}

CameraController::~CameraController() {}

void CameraController::Initialize() { camera_.Initialize(); }

void CameraController::Reset() {
	const WorldTransform& targetTransform = target_->GetWorldTransform();
	// 追従対象とオフセットからカメラの座標を計算
	camera_.translation_ = targetTransform.translation_ + targetOffset_;
}

void CameraController::Update() {
	// 追従対象のワールドトランスフォームを取得
	const WorldTransform& targetTransform = target_->GetWorldTransform();

	// ジャンプ中は縦成分を動かさない
	//if (target_->GetOnGround()) {
	//	targetPosition_ = targetTransform.translation_ + targetOffset_ + target_->GetVelocity() * kVelocityBias;
	//} else {
	//	targetPosition_.x = targetTransform.translation_.x + targetOffset_.x + target_->GetVelocity().x * kVelocityBias;
	//	targetPosition_.z = targetTransform.translation_.z + targetOffset_.z + target_->GetVelocity().z * kVelocityBias;
	//}

	targetPosition_.x = targetTransform.translation_.x + targetOffset_.x + target_->GetVelocity().x * kVelocityBias;
	targetPosition_.z = targetTransform.translation_.z + targetOffset_.z + target_->GetVelocity().z * kVelocityBias;

	// 追従対象とオフセットからカメラの座標を計算
	camera_.translation_.x = std::lerp(camera_.translation_.x, targetPosition_.x, kInterpolationRate);
	camera_.translation_.z = std::lerp(camera_.translation_.z, targetPosition_.z, kInterpolationRate);

	// 追従対象が画面内に収まるようにマージンを適応
	camera_.translation_.x = std::clamp(camera_.translation_.x, targetTransform.translation_.x - kMargin.left, targetTransform.translation_.x + kMargin.right);
	camera_.translation_.y = std::clamp(camera_.translation_.y, targetTransform.translation_.y - kMargin.top, targetTransform.translation_.y + kMargin.bottom);

	// 移動可能エリアで制限
	camera_.translation_.x = std::clamp(camera_.translation_.x, movableArea_.left, movableArea_.right);
	camera_.translation_.y = std::clamp(camera_.translation_.y, movableArea_.top, movableArea_.bottom);

	// 行列を更新する
	camera_.UpdateMatrix();
}