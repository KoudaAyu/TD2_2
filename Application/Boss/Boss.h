#pragma once

#include <vector>
#include <list>
#include <memory>
#include <cstdint>
#include <cassert>

#include "Camera.h"
#include "EnemyBullet.h"
#include "Object3d.h"
#include "Object3dCom.h"
#include "Transform.h"
#include "BossPart.h"

class Player;

class Boss
{
public:
    Boss();
    ~Boss();

    void Initialize(Object3d* model, Camera* camera, const Vector3 pos, Object3dCom* object3dCom);
    void Update();
    void Draw();

    void OnCollision();

    void SetPlayer(Player* player) { player_ = player; }

    const std::list<EnemyBullet*>& GetBullets() const { return bullets_; }
    bool IsActive() const { return isActive_; }

    void RegisterBullet(EnemyBullet* b) { bullets_.push_back(b); }

    Camera* GetCamera() const { return camera_; }
    Object3dCom* GetObject3dCom() const { return object3dCom_; }
    Player* GetPlayer() const { return player_; }

    // ワールド変換の取得（部位がボス位置に追従するため）
    const Matrix4x4& GetWorldMatrix() const { return worldTransform_.GetWorldMatrix(); }
    Vector3 GetWorldTranslate() const { return worldTransform_.GetTranslate(); }

    // プレイヤーのワールド座標を返すラッパー（parts が Player を直接参照しなくて済む）
    Vector3 GetPlayerWorldTranslate() const;

private:
    std::vector<std::unique_ptr<BossPart>> parts_;
    std::list<EnemyBullet*> bullets_;

    Transform worldTransform_ = {};
    Object3d* model_ = nullptr;
    Object3dCom* object3dCom_ = nullptr;
    Camera* camera_ = nullptr;
    Player* player_ = nullptr;

    bool isActive_ = true;
};
