#pragma once
#include <cmath>
#include "Vector3.h"

class Matrix4x4 {
public:
	float m[4][4];

	static Matrix4x4 Add(const Matrix4x4& m1, const Matrix4x4& m2);
	static Matrix4x4 Subtract(const Matrix4x4& m1, const Matrix4x4& m2);
	static Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2);
	static Matrix4x4 Inverse(const Matrix4x4& m);
	static Matrix4x4 Transpose(const Matrix4x4& m);
	static Matrix4x4 MakeIdentity4x4();

	/// <summary>
	/// 縦方向の視野角、アスペクト比、近接/遠方クリップ平面から透視投影行列を作成します。
	/// </summary>
	/// <param name="fovY">縦方向の視野角。通常はラジアンで指定します。</param>
	/// <param name="aspect">ビューのアスペクト比（幅 ÷ 高さ）。</param>
	/// <param name="nearZ">近接クリップ平面までの距離（正の値）。</param>
	/// <param name="farZ">遠方クリップ平面までの距離（nearZ より大きい正の値）。</param>
	/// <returns>透視投影を表す 4x4 行列（Matrix4x4）。</returns>
	static Matrix4x4 MakeParspectiveFovMatrix(float fovY, float aspect, float nearZ, float farZ);

	//（left/top/right/bottom 指定）
	static Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearZ, float farZ);

	/// <summary>
	/// スケーリング、回転、平行移動を組み合わせたアフィン変換行列を作成します。
	/// </summary>
	static Matrix4x4 MakeTranslateMatrix(const Vector3& translation) {
		Matrix4x4 result = MakeIdentity4x4();
		result.m[0][3] = translation.x;
		result.m[1][3] = translation.y;
		result.m[2][3] = translation.z;
		return result;
	}

	static Matrix4x4 MakeScaleMatrix(const Vector3& scale) {
		Matrix4x4 result = MakeIdentity4x4();
		result.m[0][0] = scale.x;
		result.m[1][1] = scale.y;
		result.m[2][2] = scale.z;
		return result;
	}

	static Matrix4x4 MakeRotateXMatrix(float angle) {
		Matrix4x4 result = MakeIdentity4x4();
		float c = std::cos(angle);
		float s = std::sin(angle);
		result.m[1][1] = c;
		result.m[1][2] = -s;
		result.m[2][1] = s;
		result.m[2][2] = c;
		return result;
	}

	static Matrix4x4 MakeRotateYMatrix(float angle) {
		Matrix4x4 result = MakeIdentity4x4();
		float c = std::cos(angle);
		float s = std::sin(angle);
		result.m[0][0] = c;
		result.m[0][2] = s;
		result.m[2][0] = -s;
		result.m[2][2] = c;
		return result;
	}

	static Matrix4x4 MakeRotateZMatrix(float angle) {
		Matrix4x4 result = MakeIdentity4x4();
		float c = std::cos(angle);
		float s = std::sin(angle);
		result.m[0][0] = c;
		result.m[0][1] = -s;
		result.m[1][0] = s;
		result.m[1][1] = c;
		return result;
	}

private:
	static float Determinant3x3(
		float a00, float a01, float a02,
		float a10, float a11, float a12,
		float a20, float a21, float a22);
	static float Determinant4x4(const Matrix4x4& m);
};

#pragma endregion

