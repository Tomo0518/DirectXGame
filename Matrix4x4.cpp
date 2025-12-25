#include "Matrix4x4.h"
#include <cmath> // tanf

Matrix4x4 Matrix4x4::Add(const Matrix4x4& m1, const Matrix4x4& m2) {
	Matrix4x4 result = { 0 };
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			result.m[i][j] = m1.m[i][j] + m2.m[i][j];
		}
	}
	return result;
}

Matrix4x4 Matrix4x4::Subtract(const Matrix4x4& m1, const Matrix4x4& m2) {
	Matrix4x4 result = { 0 };
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			result.m[i][j] = m1.m[i][j] - m2.m[i][j];
		}
	}
	return result;
};

Matrix4x4 Matrix4x4::Multiply(const Matrix4x4& m1, const Matrix4x4& m2) {
	Matrix4x4 result = { 0 };
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			for (int k = 0; k < 4; ++k) {
				result.m[i][j] += m1.m[i][k] * m2.m[k][j];
			}
		}
	}
	return result;
};

#pragma region Inverse4x4

// 4x4行列の逆行列を求める
Matrix4x4 Matrix4x4::Inverse(const Matrix4x4& m) {

	float determinant = 0;
	Matrix4x4 result = { 0 };

	// 4x4行列の逆行列を求めるためのDeterminant計算
	determinant = Determinant4x4(m);

	//0行目の計算
	result.m[0][0] = {
		(m.m[1][1] * m.m[2][2] * m.m[3][3] + m.m[1][2] * m.m[2][3] * m.m[3][1]
		+ m.m[1][3] * m.m[2][1] * m.m[3][2] - m.m[1][3] * m.m[2][2] * m.m[3][1]
		- m.m[1][2] * m.m[2][1] * m.m[3][3] - m.m[1][1] * m.m[2][3] * m.m[3][2])
			/ determinant
	};

	result.m[0][1] = {
		(-m.m[0][1] * m.m[2][2] * m.m[3][3] - m.m[0][2] * m.m[2][3] * m.m[3][1]
		- m.m[0][3] * m.m[2][1] * m.m[3][2] + m.m[0][3] * m.m[2][2] * m.m[3][1]
		+ m.m[0][2] * m.m[2][1] * m.m[3][3] + m.m[0][1] * m.m[2][3] * m.m[3][2])
			/ determinant
	};

	result.m[0][2] = {
		(m.m[0][1] * m.m[1][2] * m.m[3][3] + m.m[0][2] * m.m[1][3] * m.m[3][1]
			+ m.m[0][3] * m.m[1][1] * m.m[3][2] - m.m[0][3] * m.m[1][2] * m.m[3][1]
			- m.m[0][2] * m.m[1][1] * m.m[3][3] - m.m[0][1] * m.m[1][3] * m.m[3][2])
		/ determinant
	};

	result.m[0][3] = {
		(-m.m[0][1] * m.m[1][2] * m.m[2][3] - m.m[0][2] * m.m[1][3] * m.m[2][1]
		- m.m[0][3] * m.m[1][1] * m.m[2][2] + m.m[0][3] * m.m[1][2] * m.m[2][1]
		+ m.m[0][2] * m.m[1][1] * m.m[2][3] + m.m[0][1] * m.m[1][3] * m.m[2][2])
			/ determinant
	};

	//1行目の計算
	result.m[1][0] = {
	(-m.m[1][0] * m.m[2][2] * m.m[3][3] - m.m[1][2] * m.m[2][3] * m.m[3][0]
	- m.m[1][3] * m.m[2][0] * m.m[3][2] + m.m[1][3] * m.m[2][2] * m.m[3][0]
	+ m.m[1][2] * m.m[2][0] * m.m[3][3] + m.m[1][0] * m.m[2][3] * m.m[3][2])
		/ determinant
	};

	result.m[1][1] = {
		(m.m[0][0] * m.m[2][2] * m.m[3][3] + m.m[0][2] * m.m[2][3] * m.m[3][0]
		+ m.m[0][3] * m.m[2][0] * m.m[3][2] - m.m[0][3] * m.m[2][2] * m.m[3][0]
		- m.m[0][2] * m.m[2][0] * m.m[3][3] - m.m[0][0] * m.m[2][3] * m.m[3][2])
			/ determinant
	};

	result.m[1][2] = {
		(-m.m[0][0] * m.m[1][2] * m.m[3][3] - m.m[0][2] * m.m[1][3] * m.m[3][0]
		- m.m[0][3] * m.m[1][0] * m.m[3][2] + m.m[0][3] * m.m[1][2] * m.m[3][0]
		+ m.m[0][2] * m.m[1][0] * m.m[3][3] + m.m[0][0] * m.m[1][3] * m.m[3][2])
			/ determinant
	};

	result.m[1][3] = {
		(m.m[0][0] * m.m[1][2] * m.m[2][3] + m.m[0][2] * m.m[1][3] * m.m[2][0]
		+ m.m[0][3] * m.m[1][0] * m.m[2][2] - m.m[0][3] * m.m[1][2] * m.m[2][0]
		- m.m[0][2] * m.m[1][0] * m.m[2][3] - m.m[0][0] * m.m[1][3] * m.m[2][2])
			/ determinant
	};

	//2行目の計算
	result.m[2][0] = {
		(m.m[1][0] * m.m[2][1] * m.m[3][3] + m.m[1][1] * m.m[2][3] * m.m[3][0]
		+ m.m[1][3] * m.m[2][0] * m.m[3][1] - m.m[1][3] * m.m[2][1] * m.m[3][0]
		- m.m[1][1] * m.m[2][0] * m.m[3][3] - m.m[1][0] * m.m[2][3] * m.m[3][1])
			/ determinant
	};

	result.m[2][1] = {
		(-m.m[0][0] * m.m[2][1] * m.m[3][3] - m.m[0][1] * m.m[2][3] * m.m[3][0]
		- m.m[0][3] * m.m[2][0] * m.m[3][1] + m.m[0][3] * m.m[2][1] * m.m[3][0]
		+ m.m[0][1] * m.m[2][0] * m.m[3][3] + m.m[0][0] * m.m[2][3] * m.m[3][1])
			/ determinant
	};

	result.m[2][2] = {
		(m.m[0][0] * m.m[1][1] * m.m[3][3] + m.m[0][1] * m.m[1][3] * m.m[3][0]
		+ m.m[0][3] * m.m[1][0] * m.m[3][1] - m.m[0][3] * m.m[1][1] * m.m[3][0]
		- m.m[0][1] * m.m[1][0] * m.m[3][3] - m.m[0][0] * m.m[1][3] * m.m[3][1])
			/ determinant
	};

	result.m[2][3] = {
		(-m.m[0][0] * m.m[1][1] * m.m[2][3] - m.m[0][1] * m.m[1][3] * m.m[2][0]
		- m.m[0][3] * m.m[1][0] * m.m[2][1] + m.m[0][3] * m.m[1][1] * m.m[2][0]
		+ m.m[0][1] * m.m[1][0] * m.m[2][3] + m.m[0][0] * m.m[1][3] * m.m[2][1])
			/ determinant
	};

	//3行目の計算
	result.m[3][0] = {
		(-m.m[1][0] * m.m[2][1] * m.m[3][2] - m.m[1][1] * m.m[2][2] * m.m[3][0]
		- m.m[1][2] * m.m[2][0] * m.m[3][1] + m.m[1][2] * m.m[2][1] * m.m[3][0]
		+ m.m[1][1] * m.m[2][0] * m.m[3][2] + m.m[1][0] * m.m[2][2] * m.m[3][1])
			/ determinant
	};

	result.m[3][1] = {
		(m.m[0][0] * m.m[2][1] * m.m[3][2] + m.m[0][1] * m.m[2][2] * m.m[3][0]
		+ m.m[0][2] * m.m[2][0] * m.m[3][1] - m.m[0][2] * m.m[2][1] * m.m[3][0]
		- m.m[0][1] * m.m[2][0] * m.m[3][2] - m.m[0][0] * m.m[2][2] * m.m[3][1])
			/ determinant
	};

	result.m[3][2] = {
		(-m.m[0][0] * m.m[1][1] * m.m[3][2] - m.m[0][1] * m.m[1][2] * m.m[3][0]
		- m.m[0][2] * m.m[1][0] * m.m[3][1] + m.m[0][2] * m.m[1][1] * m.m[3][0]
		+ m.m[0][1] * m.m[1][0] * m.m[3][2] + m.m[0][0] * m.m[1][2] * m.m[3][1])
			/ determinant
	};

	result.m[3][3] = {
		(m.m[0][0] * m.m[1][1] * m.m[2][2] + m.m[0][1] * m.m[1][2] * m.m[2][0]
		+ m.m[0][2] * m.m[1][0] * m.m[2][1] - m.m[0][2] * m.m[1][1] * m.m[2][0]
		- m.m[0][1] * m.m[1][0] * m.m[2][2] - m.m[0][0] * m.m[1][2] * m.m[2][1])
			/ determinant
	};
	return result;
}

// 4x4行列の転置
Matrix4x4 Matrix4x4::Transpose(const Matrix4x4& m) {
	Matrix4x4 result = { 0 };
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			result.m[i][j] = m.m[j][i];
		}
	}
	return result;
}

Matrix4x4 Matrix4x4::MakeIdentity4x4() {
	Matrix4x4 result = { 0 };
	for (int i = 0; i < 4; ++i) {
		result.m[i][i] = 1.0f;
	}
	return result;
}

Matrix4x4 Matrix4x4::MakeParspectiveFovMatrix(float fovY, float aspect, float nearZ, float farZ) {
	Matrix4x4 result = { 0 };

	// cot(fovY/2) = 1 / tan(fovY/2)
	const float cotHalfFovY = 1.0f / std::tanf(fovY * 0.5f);

	// 1/a * cot(fovY/2)
	result.m[0][0] = (1.0f / aspect) * cotHalfFovY; // 行0,列0

	// cot(fovY/2)
	result.m[1][1] = cotHalfFovY;                   // 行1,列1

	// z 行
	//   zf / (zf - zn)
	result.m[2][2] = farZ / (farZ - nearZ);         // 行2,列2
	//   1
	result.m[2][3] = 1.0f;                          // 行2,列3

	// w 行
	//   -zn * zf / (zf - zn)
	result.m[3][2] = (-nearZ * farZ) / (farZ - nearZ); // 行3,列2
	//   0
	result.m[3][3] = 0.0f;                             // 行3,列3

	return result;
}

#pragma endregion
float Matrix4x4::Determinant3x3(
	float a00, float a01, float a02,
	float a10, float a11, float a12,
	float a20, float a21, float a22) {
	return
		a00 * (a11 * a22 - a12 * a21)
		- a01 * (a10 * a22 - a12 * a20)
		+ a02 * (a10 * a21 - a11 * a20);
}

float Matrix4x4::Determinant4x4(const Matrix4x4& m) {
	return
		m.m[0][0] * Matrix4x4::Determinant3x3(
			m.m[1][1], m.m[1][2], m.m[1][3],
			m.m[2][1], m.m[2][2], m.m[2][3],
			m.m[3][1], m.m[3][2], m.m[3][3])
		- m.m[0][1] * Matrix4x4::Determinant3x3(
			m.m[1][0], m.m[1][2], m.m[1][3],
			m.m[2][0], m.m[2][2], m.m[2][3],
			m.m[3][0], m.m[3][2], m.m[3][3])
		+ m.m[0][2] * Matrix4x4::Determinant3x3(
			m.m[1][0], m.m[1][1], m.m[1][3],
			m.m[2][0], m.m[2][1], m.m[2][3],
			m.m[3][0], m.m[3][1], m.m[3][3])
		- m.m[0][3] * Matrix4x4::Determinant3x3(
			m.m[1][0], m.m[1][1], m.m[1][2],
			m.m[2][0], m.m[2][1], m.m[2][2],
			m.m[3][0], m.m[3][1], m.m[3][2]);
}

#pragma endregion