#pragma once
#include"Vector.h"

struct AABB
{
	Vector3 min; // 最小点
	Vector3 max; // 最大点
};

bool IsCollisionAABBAABB(const AABB& aabb1, const AABB& aabb2);
