#pragma once
#include "math.h"

struct Material {
	Vector4 color; // RGBA
	int enableLighting; // ライティングを有効にするか
	float padding[3]; // 12 bytes (16-byte alignment)
	Matrix4x4 uvTransform; // UV変換行列
};