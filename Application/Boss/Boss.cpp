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
    spawnTarget_ = pos;
    spawnStart_ = { pos.x, pos.y + 8.0f, pos.z + 40.0f };
    worldTransform_.SetTranslate(spawnTarget_);

    if (model_)
    {
        model_->ApplyState(worldTransform_, camera_, true);
    }

    const int gridSizeX = 3;
    const int gridSizeY = 3;
    const float spacing = 1.2f;

    float offsetX = -(gridSizeX - 1) * 0.5f * spacing;
    float offsetY = -(gridSizeY - 1) * 0.5f * spacing;

    for (int y = 0; y < gridSizeY; ++y)
    {
        for (int x = 0; x < gridSizeX; ++x)
        {
            Vector3 partPos = { offsetX + x * spacing, offsetY + y * spacing, 0.0f };
            parts_.push_back(std::make_unique<BodyPart>());
            parts_.back()->Initialize(this, model_, partPos);
        }
    }

    const int layers = 2;
    for (int l = 1; l < layers; ++l)
    {
        float layerHeight = 0.9f * l;
        for (int y = 0; y < gridSizeY; ++y)
        {
            for (int x = 0; x < gridSizeX; ++x)
            {
                Vector3 partPos = { offsetX + x * spacing, offsetY + y * spacing, layerHeight };
                parts_.push_back(std::make_unique<BodyPart>());
                parts_.back()->Initialize(this, model_, partPos);
            }
        }
    }

    for (size_t i = 0; i < parts_.size(); ++i)
    {
        BossPart* p = parts_[i].get();
        if (!p) continue;
        Vector3 targetLocal = p->GetWorldTranslate();
        Vector3 bossPos = worldTransform_.GetTranslate();
        Vector3 localTarget = { targetLocal.x - bossPos.x, targetLocal.y - bossPos.y, targetLocal.z - bossPos.z };

        float scatterScale = 4.0f;
        Vector3 startLocal = { localTarget.x * scatterScale, localTarget.y * scatterScale + 6.0f, localTarget.z + 12.0f };

        int dur = spawnDuration_;
        p->StartSpawn(startLocal, dur);
    }

    isActive_ = true;

   
    phase_ = Phase::Spawn;
    spawnTimer_ = 0;
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

    if (phase_ == Phase::Spawn)
    {
        ++spawnTimer_;
        float t = (spawnDuration_ <= 0) ? 1.0f : (float)spawnTimer_ / (float)spawnDuration_;
        if (t > 1.0f) t = 1.0f;

       
        for (auto& p : parts_)
        {
            if (p) p->UpdateSpawn(t);
        }

        if (spawnTimer_ >= spawnDuration_)
        {
            phase_ = Phase::Active;
          
            if (camera_)
            {
                camera_->StartShake(0.6f, 0.6f);
            }
        }
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
