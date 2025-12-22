#pragma once

class Matrix4x4 {
public:
	float m[4][4];

	static Matrix4x4 Add(const Matrix4x4& m1, const Matrix4x4& m2);
	static Matrix4x4 Subtract(const Matrix4x4& m1, const Matrix4x4& m2);
	static Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2);
	static Matrix4x4 Inverse(const Matrix4x4& m);
	static Matrix4x4 Transpose(const Matrix4x4& m);
	static Matrix4x4 MakeIdentity4x4();


private:
	static float Determinant3x3(
		float a00, float a01, float a02,
		float a10, float a11, float a12,
		float a20, float a21, float a22);
	static float Determinant4x4(const Matrix4x4& m);
};

#pragma endregion

