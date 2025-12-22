#pragma once
#include "Matrix4x4.h"
#include "Vector3.h"

// X軸回転行列
Matrix4x4 MakeRotateXMatrix(float rotateX);

// Y軸回転行列
Matrix4x4 MakeRotateYMatrix(float rotateY);

// Z軸回転行列
Matrix4x4 MakeRotateZMatrix(float rotateZ);

// XYZ回転行列（X→Y→Zの順で合成）
Matrix4x4 MakeRotateXYZMatrix(const Matrix4x4& mx, const Matrix4x4& my, const Matrix4x4& mz);

// 平行移動行列
Matrix4x4 MakeTranslateMatrix(const Vector3& translate);

// スケール行列
Matrix4x4 MakeScaleMatrix(const Vector3& scale);

// アフィン変換行列
Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate);