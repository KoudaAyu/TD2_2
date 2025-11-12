#pragma once

#include <cmath>

struct Vector2
{
	float x;
	float y;
};

struct Vector3
{
	float x;
	float y;
	float z;

	Vector3& operator+=(const Vector3& other)
	{
		this->x += other.x;
		this->y += other.y;
		this->z += other.z;
		return *this; // 自分自身への参照を返す
	}
};

// 二項演算（最小限）
inline Vector3 operator+(const Vector3& a, const Vector3& b)
{
	return { a.x + b.x, a.y + b.y, a.z + b.z };
}

inline Vector3 operator*(const Vector3& v, float s)
{
	return { v.x * s, v.y * s, v.z * s };
}

inline Vector3 operator*(float s, const Vector3& v)
{
	return v * s;
}

// 等価比較（小さなイプシロンで浮動小数点誤差を吸収）
inline bool operator==(const Vector3& a, const Vector3& b)
{
	constexpr float EPS = 1e-5f;
	return (std::fabs(a.x - b.x) <= EPS) &&
	       (std::fabs(a.y - b.y) <= EPS) &&
	       (std::fabs(a.z - b.z) <= EPS);
}

inline bool operator!=(const Vector3& a, const Vector3& b)
{
	return !(a == b);
}

struct Vector4
{
	float x;
	float y;
	float z;
	float w;
};