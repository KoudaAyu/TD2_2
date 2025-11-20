#include "MathUtl.h"
#include <cmath>

Vector3 TransformNormal(const Vector3& v, const Matrix4x4& m)
{
	Vector3 result{
		v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0],
		v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1],
		v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2]
	};

	return result;
}

// ベクトル長
float Length(const Vector3& v)
{
	return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

// 正規化（ゼロベクトル対策）
Vector3 Normalize(const Vector3& v)
{
	float len = Length(v);
	if (len <= 1e-6f) return {0.0f, 0.0f, 0.0f};
	return { v.x / len, v.y / len, v.z / len };
}

// 2点間の距離
float Distance(const Vector3& a, const Vector3& b)
{
	Vector3 diff{ a.x - b.x, a.y - b.y, a.z - b.z };
	return Length(diff);
}
