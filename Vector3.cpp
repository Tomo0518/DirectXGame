#include "Vector3.h"
#include "Matrix4x4.h"
#include <cmath>

Vector3 Vector3::Add(const Vector3& v1, const Vector3& v2) {
	Vector3 result;
	result.x = v1.x + v2.x;
	result.y = v1.y + v2.y;
	result.z = v1.z + v2.z;
	return result;
}

Vector3 Vector3::Subtract(const Vector3& v1, const Vector3& v2) {
	Vector3 result;
	result.x = v1.x - v2.x;
	result.y = v1.y - v2.y;
	result.z = v1.z - v2.z;
	return result;
}

Vector3 Vector3::Multiply(float scalar, const Vector3& v1) {
	Vector3 result;
	result.x = v1.x * scalar;
	result.y = v1.y * scalar;
	result.z = v1.z * scalar;
	return result;
}

float Vector3::Dot(const Vector3& v1, const Vector3& v2) {
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

float Vector3::Length(const Vector3& v) {
	return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

Vector3 Vector3::Normalize(Vector3 v) {
	float length = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
	return { v.x / length, v.y / length, v.z / length };
}

Vector3 Vector3::Cross(const Vector3& a, const Vector3& b) {
	Vector3 result;
	result.x = a.y * b.z - a.z * b.y;
	result.y = a.z * b.x - a.x * b.z;
	result.z = a.x * b.y - a.y * b.x;
	return result;
}

Vector3 Vector3::Transform(const Vector3& vector, const Matrix4x4& matrix) {
	Vector3 result;
	result.x = vector.x * matrix.m[0][0] + vector.y * matrix.m[1][0] + vector.z * matrix.m[2][0] + matrix.m[3][0];
	result.y = vector.x * matrix.m[0][1] + vector.y * matrix.m[1][1] + vector.z * matrix.m[2][1] + matrix.m[3][1];
	result.z = vector.x * matrix.m[0][2] + vector.y * matrix.m[1][2] + vector.z * matrix.m[2][2] + matrix.m[3][2];
	float w = vector.x * matrix.m[0][3] + vector.y * matrix.m[1][3] + vector.z * matrix.m[2][3] + matrix.m[3][3];
	if (w != 0.0f) {
		result.x /= w;
		result.y /= w;
		result.z /= w;
	}
	return result;
}

Vector3 Vector3::ViewportTransform(const Vector3& ndc, const Matrix4x4& viewportMatrix) {
	Vector3 result;
	result.x = ndc.x * viewportMatrix.m[0][0] + ndc.y * viewportMatrix.m[1][0] + ndc.z * viewportMatrix.m[2][0] + viewportMatrix.m[3][0];
	result.y = ndc.x * viewportMatrix.m[0][1] + ndc.y * viewportMatrix.m[1][1] + ndc.z * viewportMatrix.m[2][1] + viewportMatrix.m[3][1];
	result.z = ndc.x * viewportMatrix.m[0][2] + ndc.y * viewportMatrix.m[1][2] + ndc.z * viewportMatrix.m[2][2] + viewportMatrix.m[3][2];
	return result;
}

Vector3 operator+(const Vector3& v1, const Vector3& v2) { return Vector3::Add(v1, v2); }
Vector3 operator-(const Vector3& v1, const Vector3& v2) { return Vector3::Subtract(v1, v2); }
Vector3 operator*(float s, const Vector3& v) { return Vector3::Multiply(s, v); }
Vector3 operator*(const Vector3& v, float s) { return s * v; }
Vector3 operator/(const Vector3& v, float s) { return Vector3::Multiply(1.0f / s, v); }

Vector3 operator-(const Vector3& v) { return { -v.x, -v.y, -v.z }; }
Vector3 operator+(const Vector3& v) { return v; }