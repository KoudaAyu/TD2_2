#pragma once

#include "Transform.h"
#include "Matrix4x4.h"

class Boss;
class Object3d;

// 部位の抽象クラス
class BossPart
{
public:
    BossPart() = default;
    virtual ~BossPart() = default;

    virtual void Initialize(Boss* owner, Object3d* model, const Vector3& localPos) = 0;
    virtual void Update() = 0;
    virtual void Draw() = 0;

    virtual void OnDamage(int dmg) = 0;
    virtual bool IsDestroyed() const = 0;

    // ワールド変換取得
    virtual const Matrix4x4& GetWorldMatrix() const = 0;
    virtual Vector3 GetWorldTranslate() const = 0;
};
