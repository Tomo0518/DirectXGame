#include "Camera.h"


void Camera::Initialize() {
	// 初期化処理
	matView = Matrix4x4::MakeIdentity4x4();
	matProjection = Matrix4x4::MakeIdentity4x4();
}

void Camera::UpdateMatrix() {
	UpdateViewMatrix();
	UpdateProjectionMatrix(60.0f, 16.0f / 9.0f, 0.1f, 100.0f);
}

void Camera::UpdateProjectionMatrix(float fovY, float aspect, float nearZ, float farZ) {
	matProjection = Matrix4x4::MakeParspectiveFovMatrix(fovY * (3.14159265f / 180.0f), aspect, nearZ, farZ);
}

void Camera::UpdateViewMatrix() {
	// カメラの位置と回転からビュー行列を計算する
	Matrix4x4 translationMatrix = Matrix4x4::MakeTranslateMatrix(translation);
	Matrix4x4 rotationXMatrix = Matrix4x4::MakeRotateXMatrix(rotation.x);
	Matrix4x4 rotationYMatrix = Matrix4x4::MakeRotateYMatrix(rotation.y);
	Matrix4x4 rotationZMatrix = Matrix4x4::MakeRotateZMatrix(rotation.z);
	// 回転行列を合成（Z * X * Yの順）
	Matrix4x4 rotationMatrix = Matrix4x4::Multiply(rotationZMatrix, Matrix4x4::Multiply(rotationXMatrix, rotationYMatrix));
	// ワールド行列を計算（回転→平行移動の順）
	Matrix4x4 worldMatrix = Matrix4x4::Multiply(translationMatrix, rotationMatrix);
	// ビュー行列はワールド行列の逆行列
	matView = Matrix4x4::Inverse(worldMatrix);
}