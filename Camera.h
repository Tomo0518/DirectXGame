#pragma once
#include "TomoEngine.h"

class CameraController;

class Camera {
	friend class CameraController;
public:
	Camera();
	~Camera() = default;

    /// <summary>
	/// カメラの初期化
    /// </summary>
    void Initialize();

    /// <summary>
    /// 行列の更新
    /// </summary>
    void UpdateMatrix();

	/// <summary>
	/// ビューマトリックスの更新
	/// </summary>
	void UpdateViewMatrix();


	/// <summary>
	/// デバッグカメラの挙動更新(キー入力)
	/// 
	void UpdateDebugCameraMove(float deltaTime);

	Matrix4x4 GetViewProjectionMatrix() const {
		return Matrix4x4::Multiply(matView, matProjection);
	}

	/// <summary>
	/// 投影行列を更新します。
	/// </summary>
	/// <param name="fovY">Y方向の視野角</param>
	/// <param name="aspect">アスペクト比</param>
	/// <param name="nearZ">ニアクリップ面までの距離</param>
	/// <param name="farZ">ファークリップ面までの距離</param>
	void UpdateProjectionMatrix(float fovY, float aspect, float nearZ, float farZ);

	void UpdateProjectionMatrix();

	// Getter
	const Matrix4x4& GetViewMatrix() const { return matView; }
	const Matrix4x4& GetProjectionMatrix() const { return matProjection; }

	Vector3& GetTranslation() { return translation_; }
	void SetTranslation(const Vector3& pos) { translation_ = pos; }
	Vector3& GetRotation() { return rotation_; }
	void SetRotation(const Vector3& rot) { rotation_ = rot; }

private:
    Vector3 translation_ = { 0.0f, 0.0f, -5.0f };
    Vector3 rotation_ = { 0.0f, 0.0f, 0.0f };

	float fovY_ = 60.0f;
	float aspect_ = 16.0f / 9.0f;
	float nearZ_ = 0.1f;
	float farZ_ = 100.0f;

    Matrix4x4 matView;
    Matrix4x4 matProjection;
};