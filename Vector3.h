#pragma once

class Matrix4x4;

class Vector3 {
public:
	float x, y, z;

	// 加算代入
	Vector3& operator+=(const Vector3& v) {
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}

	// 減算代入
	Vector3& operator-=(const Vector3& v) {
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}

	// スカラー倍代入
	Vector3& operator*=(float s) {
		x *= s;
		y *= s;
		z *= s;
		return *this;
	}


	Vector3& operator/=(float s) {
		x /= s;
		y /= s;
		z /= s;
		return *this;
	}

	static Vector3 Add(const Vector3& v1, const Vector3& v2);
	static Vector3 Subtract(const Vector3& v1, const Vector3& v2);
	static Vector3 Multiply(float scalar, const Vector3& v1);
	static float Dot(const Vector3& v1, const Vector3& v2);
	static float Length(const Vector3& v);
	static Vector3 Normalize(Vector3 v);
	static Vector3 Cross(const Vector3& a, const Vector3& b);
	static Vector3 Transform(const Vector3& vector, const Matrix4x4& matrix);
	static Vector3 ViewportTransform(const Vector3& ndc, const Matrix4x4& viewportMatrix);
};

Vector3 operator+(const Vector3& v1, const Vector3& v2);
Vector3 operator-(const Vector3& v1, const Vector3& v2);
Vector3 operator*(float s, const Vector3& v);
Vector3 operator*(const Vector3& v, float s);
Vector3 operator/(const Vector3& v, float s);

Vector3 operator-(const Vector3& v);
Vector3 operator+(const Vector3& v);

