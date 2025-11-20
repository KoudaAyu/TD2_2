#pragma once
#include"Camera.h"
#include"Object3d.h"
#include"Transform.h"

class PlayerBarrier
{
public:
    PlayerBarrier() = default;
    ~PlayerBarrier();
    // Initialize now accepts an Object3dCom to create its own Object3d instance
    void Initialize(Object3d* model, const Vector3 pos, Object3dCom* object3dCom);
    void Update();
    void Draw(const Camera& camera);

private:
    Object3d* model_ = nullptr; // source model (not the instance used for drawing)
    Object3dCom* object3dCom_ = nullptr;
    Object3d* barrierModel_ = nullptr; // separate Object3d instance for the barrier
    Transform worldTransform = {};

};
