#pragma once
#include "Vector3.h"

// 球体を表す構造体
struct Sphere {
	Vector3 center; //!< 中心点
	float radius;   //!< 半径
	Vector3 rotation; //!< 回転角
};