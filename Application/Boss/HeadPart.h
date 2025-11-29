#pragma once

#include "BossPart.h"
#include "Camera.h"
#include "Object3d.h"
#include "Object3dCom.h"
#include "Transform.h"

class Player;
class Boss;

class HeadPart : public BossPart
{
public:
    HeadPart();
    ~HeadPart() override;

    void Initialize(Boss* owner, Object3d* model, const Vector3& localPos) override;
    void Update() override;
    void Draw() override;

    void OnDamage(int dmg) override;
    bool IsDestroyed() const override;

    const Matrix4x4& GetWorldMatrix() const override;
    Vector3 GetWorldTranslate() const override;

private:
    Boss* owner_ = nullptr;
    Camera* camera_ = nullptr;
    Object3d* model_ = nullptr;
    Object3dCom* object3dCom_ = nullptr;
    Transform worldTransform_ = {};

    int hp_ = 50;
};
