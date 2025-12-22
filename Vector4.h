#pragma once
#include "Vector3.h"
#include "Matrix4x4.h"

struct Vector4 {
	float x, y, z, w;
};

Vector4 TransformWithW(const Vector3& vector, const Matrix4x4& matrix);

Matrix4x4 operator+(const Matrix4x4& m1, const Matrix4x4& m2);
Matrix4x4 operator-(const Matrix4x4& m1, const Matrix4x4& m2);
Matrix4x4 operator*(const Matrix4x4& m1, const Matrix4x4& m2);