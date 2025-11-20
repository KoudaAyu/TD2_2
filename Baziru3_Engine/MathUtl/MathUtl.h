#pragma once

#include "Vector.h"
#include "Matrix4x4.h"	

Vector3 TransformNormal(const Vector3& v,const Matrix4x4& m);

// 追加: ベクトル長・正規化ユーティリティ
float Length(const Vector3& v);
Vector3 Normalize(const Vector3& v);