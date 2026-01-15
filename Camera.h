#pragma once
#include "TomoEngine.h"

class Camera {
public:
	Camera() = default;
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
	/// 投影行列を更新します。
	/// </summary>
	/// <param name="fovY">Y方向の視野角</param>
	/// <param name="aspect">アスペクト比</param>
	/// <param name="nearZ">ニアクリップ面までの距離</param>
	/// <param name="farZ">ファークリップ面までの距離</param>
	void UpdateProjectionMatrix(float fovY, float aspect, float nearZ, float farZ);

	// Getter
	const Matrix4x4& GetViewMatrix() const { return matView; }
	const Matrix4x4& GetProjectionMatrix() const { return matProjection; }

	Vector3& GetTranslation() { return translation; }
	void SetTranslation(const Vector3& pos) { translation = pos; }
	Vector3& GetRotation() { return rotation; }
	void SetRotation(const Vector3& rot) { rotation = rot; }

private:
    Vector3 translation = { 0.0f, 0.0f, -5.0f };
    Vector3 rotation = { 0.0f, 0.0f, 0.0f };

    Matrix4x4 matView;
    Matrix4x4 matProjection;
};