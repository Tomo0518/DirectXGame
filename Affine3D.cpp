#include "Affine3D.h"
#include <cmath>

Matrix4x4 MakeRotateXMatrix(float rotateX) {
	Matrix4x4 matrix = {};
	matrix.m[0][0] = 1.0f;
	matrix.m[0][1] = 0.0f;
	matrix.m[0][2] = 0.0f;
	matrix.m[0][3] = 0.0f;

	matrix.m[1][0] = 0.0f;
	matrix.m[1][1] = std::cosf(rotateX);
	matrix.m[1][2] = std::sinf(rotateX);
	matrix.m[1][3] = 0.0f;

	matrix.m[2][0] = 0.0f;
	matrix.m[2][1] = -std::sinf(rotateX);
	matrix.m[2][2] = std::cosf(rotateX);
	matrix.m[2][3] = 0.0f;

	matrix.m[3][0] = 0.0f;
	matrix.m[3][1] = 0.0f;
	matrix.m[3][2] = 0.0f;
	matrix.m[3][3] = 1.0f;
	return matrix;
}

Matrix4x4 MakeRotateYMatrix(float rotateY) {
	Matrix4x4 matrix = {};
	matrix.m[0][0] = std::cosf(rotateY);
	matrix.m[0][1] = 0.0f;
	matrix.m[0][2] = -std::sinf(rotateY);
	matrix.m[0][3] = 0.0f;

	matrix.m[1][0] = 0.0f;
	matrix.m[1][1] = 1.0f;
	matrix.m[1][2] = 0.0f;
	matrix.m[1][3] = 0.0f;

	matrix.m[2][0] = std::sinf(rotateY);
	matrix.m[2][1] = 0.0f;
	matrix.m[2][2] = std::cosf(rotateY);
	matrix.m[2][3] = 0.0f;

	matrix.m[3][0] = 0.0f;
	matrix.m[3][1] = 0.0f;
	matrix.m[3][2] = 0.0f;
	matrix.m[3][3] = 1.0f;
	return matrix;
}

Matrix4x4 MakeRotateZMatrix(float rotateZ) {
	Matrix4x4 matrix = {};
	matrix.m[0][0] = std::cosf(rotateZ);
	matrix.m[0][1] = std::sinf(rotateZ);
	matrix.m[0][2] = 0.0f;
	matrix.m[0][3] = 0.0f;

	matrix.m[1][0] = -std::sinf(rotateZ);
	matrix.m[1][1] = std::cosf(rotateZ);
	matrix.m[1][2] = 0.0f;
	matrix.m[1][3] = 0.0f;

	matrix.m[2][0] = 0.0f;
	matrix.m[2][1] = 0.0f;
	matrix.m[2][2] = 1.0f;
	matrix.m[2][3] = 0.0f;

	matrix.m[3][0] = 0.0f;
	matrix.m[3][1] = 0.0f;
	matrix.m[3][2] = 0.0f;
	matrix.m[3][3] = 1.0f;
	return matrix;
}

Matrix4x4 MakeRotateXYZMatrix(const Matrix4x4& mx, const Matrix4x4& my, const Matrix4x4& mz) {
	Matrix4x4 result = Matrix4x4::Multiply(mx, my);
	result = Matrix4x4::Multiply(result, mz);
	return result;
}

Matrix4x4 MakeTranslateMatrix(const Vector3& translate) {
	Matrix4x4 result = {};
	result.m[0][0] = 1.0f; result.m[0][1] = 0.0f; result.m[0][2] = 0.0f; result.m[0][3] = 0.0f;
	result.m[1][0] = 0.0f; result.m[1][1] = 1.0f; result.m[1][2] = 0.0f; result.m[1][3] = 0.0f;
	result.m[2][0] = 0.0f; result.m[2][1] = 0.0f; result.m[2][2] = 1.0f; result.m[2][3] = 0.0f;
	result.m[3][0] = translate.x; result.m[3][1] = translate.y; result.m[3][2] = translate.z; result.m[3][3] = 1.0f;
	return result;
}

Matrix4x4 MakeScaleMatrix(const Vector3& scale) {
	Matrix4x4 result = {};
	result.m[0][0] = scale.x; result.m[0][1] = 0.0f; result.m[0][2] = 0.0f; result.m[0][3] = 0.0f;
	result.m[1][0] = 0.0f; result.m[1][1] = scale.y; result.m[1][2] = 0.0f; result.m[1][3] = 0.0f;
	result.m[2][0] = 0.0f; result.m[2][1] = 0.0f; result.m[2][2] = scale.z; result.m[2][3] = 0.0f;
	result.m[3][0] = 0.0f; result.m[3][1] = 0.0f; result.m[3][2] = 0.0f; result.m[3][3] = 1.0f;
	return result;
}

Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate) {
	Matrix4x4 scaleMatrix = MakeScaleMatrix(scale);
	Matrix4x4 rotateX = MakeRotateXMatrix(rotate.x);
	Matrix4x4 rotateY = MakeRotateYMatrix(rotate.y);
	Matrix4x4 rotateZ = MakeRotateZMatrix(rotate.z);
	Matrix4x4 rotationMatrix = MakeRotateXYZMatrix(rotateX, rotateY, rotateZ);
	Matrix4x4 translateMatrix = MakeTranslateMatrix(translate);
	Matrix4x4 affineMatrix = Matrix4x4::Multiply(scaleMatrix, rotationMatrix);
	affineMatrix = Matrix4x4::Multiply(affineMatrix, translateMatrix);
	return affineMatrix;
}

