#include "Camera.h"
#include "InputManager.h"

Camera::Camera() {
    Initialize();
}

void Camera::Initialize() {
    translation_ = { 0.0f, 0.0f, -5.0f };
    rotation_ = { 0.0f, 0.0f, 0.0f };

    fovY_ = 60.0f;
    aspect_ = 16.0f / 9.0f;
    nearZ_ = 0.1f;
    farZ_ = 100.0f;

    matView = Matrix4x4::MakeIdentity4x4();
    matProjection = Matrix4x4::MakeIdentity4x4();

    UpdateViewMatrix();
    UpdateProjectionMatrix();
}

void Camera::UpdateMatrix() {
    UpdateViewMatrix();
    UpdateProjectionMatrix();
}

void Camera::UpdateProjectionMatrix(float fovY, float aspect, float nearZ, float farZ) {
    matProjection = Matrix4x4::MakeParspectiveFovMatrix(fovY * (3.14159265f / 180.0f), aspect, nearZ, farZ);
}

void Camera::UpdateProjectionMatrix() {
    matProjection = Matrix4x4::MakeParspectiveFovMatrix(fovY_ * (3.14159265f / 180.0f), aspect_, nearZ_, farZ_);
}

void Camera::UpdateViewMatrix() {

    // 1. 回転行列を計算
    Matrix4x4 rotateX = Matrix4x4::MakeRotateXMatrix(rotation_.x);
    Matrix4x4 rotateY = Matrix4x4::MakeRotateYMatrix(rotation_.y);
    Matrix4x4 rotateZ = Matrix4x4::MakeRotateZMatrix(rotation_.z);

    // 回転行列の合成 (X * Y * Z)
    Matrix4x4 rotateMatrix = Matrix4x4::Multiply(rotateX, rotateY);
    rotateMatrix = Matrix4x4::Multiply(rotateMatrix, rotateZ);

    // 2. カメラのワールド行列 (S * R * T)
    Vector3 scale = { 1.0f, 1.0f, 1.0f };
    Matrix4x4 worldMatrix = MakeAffineMatrix(scale, rotation_, translation_);

    // 3. ビュー行列はワールド行列の逆行列
    matView = Matrix4x4::Inverse(worldMatrix);
}

void Camera::UpdateDebugCameraMove(float dt) {
    const auto& input = InputManager::GetInstance();

    if (input->PushKey('W')) {
        translation_.y += 0.3f * dt;
    }
    else if (input->PushKey('S')) {
        translation_.y -= 0.3f * dt;
    }

    if (input->PushKey('A')) {
        translation_.x -= 0.3f * dt;
    }
    else if (input->PushKey('D')) {
        translation_.x += 0.3f * dt;
    }

    if (input->PushKey('Q')) {
        translation_.z += 0.3f * dt;
    }
    else if (input->PushKey('E')) {
        translation_.z -= 0.3f * dt;
    }
}