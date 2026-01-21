#pragma once
#include "Vector3.h"
#include "Vector4.h"

struct DirectionalLight {
	Vector4 color;    // ライトの色
	Vector3 direction; // ライトの向き（単位ベクトル）
	float intensity;  // ライトの強度
	float padding[3];  // 12 bytes (16-byte境界合わせ)
};