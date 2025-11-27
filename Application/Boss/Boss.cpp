#include "Boss.h"
#include "HeadPart.h"
#include "TurretPart.h"
#include "BodyPart.h"
#include "Player.h"

#include <algorithm>

Boss::Boss() {}

Boss::~Boss()
{
    for (EnemyBullet* b : bullets_)
    {
        delete b;
    }
    bullets_.clear();
}

Vector3 Boss::GetPlayerWorldTranslate() const
{
    if (player_) return player_->GetWorldTranslate();
    return {0.0f, 0.0f, 0.0f};
}

void Boss::Initialize(Object3d* model, Camera* camera, const Vector3 pos, Object3dCom* object3dCom)
{
    model_ = model;
    camera_ = camera;
    object3dCom_ = object3dCom;

    worldTransform_.Initialize();
    worldTransform_.SetTranslate(pos);
    if (model_)
    {
        model_->ApplyState(worldTransform_, camera_, true);
    }

    // 部位の初期化（頭・胴体・左右タレット）
    Vector3 headPos = { 0.0f, 2.0f, 0.0f };
    Vector3 bodyPos = { 0.0f, 0.5f, 0.0f };
    Vector3 leftPos = { -1.5f, 1.0f, 0.0f };
    Vector3 rightPos = { 1.5f, 1.0f, 0.0f };

    parts_.push_back(std::make_unique<HeadPart>());
    parts_.back()->Initialize(this, model_, headPos);

    parts_.push_back(std::make_unique<BodyPart>());
    parts_.back()->Initialize(this, model_, bodyPos);

    parts_.push_back(std::make_unique<TurretPart>());
    parts_.back()->Initialize(this, model_, leftPos);

    parts_.push_back(std::make_unique<TurretPart>());
    parts_.back()->Initialize(this, model_, rightPos);

    // 追加: 腕パーツを胴体の下側に2つ追加（BodyPartを流用）
    Vector3 leftArmPos = { -0.7f, -1.0f, 0.0f };
    Vector3 rightArmPos = { 0.7f, -1.0f, 0.0f };

    parts_.push_back(std::make_unique<BodyPart>());
    parts_.back()->Initialize(this, model_, leftArmPos);

    parts_.push_back(std::make_unique<BodyPart>());
    parts_.back()->Initialize(this, model_, rightArmPos);

    isActive_ = true;
}

void Boss::Update()
{
    if (!isActive_)
    {
        for (EnemyBullet* b : bullets_)
        {
            if (b) b->Update();
        }
        return;
    }

    for (auto& p : parts_)
    {
        if (p) p->Update();
    }

    for (EnemyBullet* b : bullets_)
    {
        if (b) b->Update();
    }

    worldTransform_.TransferMatrix();
    if (model_) model_->ApplyState(worldTransform_, camera_, true);

    bool allDestroyed = true;
    for (auto& p : parts_)
    {
        if (p && !p->IsDestroyed())
        {
            allDestroyed = false;
            break;
        }
    }
    if (allDestroyed)
    {
        isActive_ = false;
    }
}

void Boss::Draw()
{
    for (EnemyBullet* b : bullets_)
    {
        if (b) b->Draw();
    }

    // Do not draw the parent/full boss model here — parts are drawn individually.

    for (auto& p : parts_)
    {
        if (p) p->Draw();
    }
}

void Boss::OnCollision()
{
    worldTransform_.SetTranslate({ 10000.0f, 10000.0f, 10000.0f });
    for (EnemyBullet* b : bullets_)
    {
        if (b) b->OnCollision();
    }
    isActive_ = false;
}
