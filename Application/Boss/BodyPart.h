#pragma once

#include "BossPart.h"
#include "Camera.h"
#include "Object3d.h"
#include "Object3dCom.h"
#include "Transform.h"

#include <vector>

class Boss;

/// <summary>
/// Bossの胴体部位
/// </summary>
class BodyPart : public BossPart
{
public:
    BodyPart();
    ~BodyPart() override;

    void Initialize(Boss* owner, Object3d* model, const Vector3& localPos) override;
    void Update() override;
    void Draw() override;

    void OnDamage(int dmg) override;
    bool IsDestroyed() const override;

    const Matrix4x4& GetWorldMatrix() const override;
    Vector3 GetWorldTranslate() const override;

    void StartSpawn(const Vector3& startLocal, int duration) override;
    void UpdateSpawn(float progress) override;

private:
    Boss* owner_ = nullptr;
    Camera* camera_ = nullptr;
  
    std::vector<Object3d*> models_;
    Object3dCom* object3dCom_ = nullptr;
 
    std::vector<Transform> localTransforms_;

    Transform worldTransform_ = {};

    int hp_ = 100; // 胴体はさらに耐久力が高い

   
    bool isSpawning_ = false;
    Vector3 spawnStartLocal_ = {0.0f, 0.0f, 0.0f};
    Vector3 spawnTargetLocal_ = {0.0f, 0.0f, 0.0f};
    int spawnTimer_ = 0;
    int spawnDuration_ = 0;
};
