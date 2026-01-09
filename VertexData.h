#pragma once
#include "math.h"

struct VertexData {
	Vector4 position; // xyz:座標、w:1.0f
	Vector2 texcoord; // uv座標
	Vector3 normal;   // 法線ベクトル
};