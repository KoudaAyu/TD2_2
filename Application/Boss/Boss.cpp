#include "Boss.h"
#include "HeadPart.h"
#include "TurretPart.h"
#include "BodyPart.h"
#include "Player.h"
#include "Random.h"

#include <algorithm>
#include "SpriteCom.h"
#include "Sprite.h"
#include <filesystem>
#include <cmath>
// Particle effects
#include "Baziru3_Engine/Particle/ParticleManager.h"
#include "Baziru3_Engine/Audio/SoundManager.h"
// AABB collision
#include "Baziru3_Engine/MathUtl/AABB.h"

Boss::Boss() {}

Boss::~Boss()
{
    for (EnemyBullet* b : bullets_)
    {
        delete b;
    }
    bullets_.clear();

    // drone visuals cleanup
    for (auto* o : droneObjs_)
    {
        if (o) { delete o; }
    }
    droneObjs_.clear();

    // phase4 turret cleanup
    for (auto &t : phase4Turrets_)
    {
        if (t.obj) { delete t.obj; t.obj = nullptr; }
    }
    phase4Turrets_.clear();

    // スプライト解放
    for (auto s : phaseSprites_)
    {
        if (s) delete s;
    }

    // レーザーモデル解放
    if (laserModel_)
    {
        delete laserModel_;
        laserModel_ = nullptr;
    }
}

Vector3 Boss::GetPlayerWorldTranslate() const
{
    if (player_) return player_->GetWorldTranslate();
    return { 0.0f, 0.0f, 0.0f };
}

static std::string FindNumberTexturePath(const std::string& fileName)
{
    const std::vector<std::string> candidates = {
        std::string("Resources/number/") + fileName,
        fileName
    };

    for (const auto& c : candidates)
    {
        if (std::filesystem::exists(std::filesystem::path(c)))
        {
            return c;
        }
    }

    return std::string();
}

// --- Pre-laser motion implementation (polymorphic) ---
namespace {
    struct PreLaserMotionImpl : public Boss::Motion {
        int timer = 0;
        int duration = 90; // frames (~1.5s at 60fps)
        Vector3 origBossScale = {1.0f,1.0f,1.0f};
        Vector3 origLaserScale = {1.0f,1.0f,1.0f};
        bool finished = false;

        void Start(Boss* owner) override
        {
            timer = 0;
            finished = false;
            if (owner->GetModel())
            {
                origBossScale = owner->GetModel()->GetScale();
            }
            if (owner->GetLaserModel())
            {
                origLaserScale = owner->GetLaserModel()->GetScale();
            }
        }

        void Update(Boss* owner, float /*dt*/) override
        {
            if (finished) return;
            ++timer;

            float t = static_cast<float>(timer) / static_cast<float>(duration);
            if (t > 1.0f) t = 1.0f;

            // Boss pulse: scale up then return
            float pulse = 1.0f + 0.25f * std::sin(t * 3.14159265f * 2.0f);
            if (owner->GetModel())
            {
                Object3d* m = owner->GetModel();
                Vector3 s = { origBossScale.x * pulse, origBossScale.y * pulse, origBossScale.z * (1.0f + 0.2f * t) };
                m->SetScale(s);

                // slight tint toward red as charge progresses (no getter for color, so set directly)
                Vector4 c = { 1.0f * (1.0f - 0.4f * t) + 1.0f * (0.4f * t), 1.0f * (1.0f - 0.2f * t), 1.0f * (1.0f - 0.2f * t), 1.0f };
                m->SetColor(c);

                m->ApplyState(Transform{ m->GetScale(), m->GetRotate(), m->GetTranslate() }, owner->GetCamera(), true);
            }

            // Laser visual pre-appearance: fade in short beam
            if (owner->GetLaserModel())
            {
                Object3d* lm = owner->GetLaserModel();
                float laserAlpha = t; // from 0 to 1
                Vector4 lc = { 1.0f, 0.3f, 0.3f, laserAlpha };
                lm->SetColor(lc);

                // small Z scale to hint beam growing
                Vector3 ls = { owner->GetLaserWidth(), owner->GetLaserWidth(), 1.0f + owner->GetLaserMaxLength() * 0.2f * t };
                lm->SetScale(ls);

                // position it in front of boss
                Vector3 bossPos = owner->GetWorldTranslatePublic();
                Vector3 laserPos = bossPos + Vector3{0.0f, 0.0f, -ls.z * 0.5f};
                lm->SetTranslate(laserPos);
                lm->ApplyState(Transform{ lm->GetScale(), lm->GetRotate(), lm->GetTranslate() }, owner->GetCamera(), true);
            }

            if (timer >= duration)
            {
                finished = true;
            }
        }

        bool IsFinished() const override { return finished; }
    };
}

void Boss::StartLaserPreMotion()
{
    currentMotion_ = std::make_unique<PreLaserMotionImpl>();
    if (currentMotion_) currentMotion_->Start(this);
}

void Boss::SetPlayer(Player* player)
{
    player_ = player;
    // if boss is currently spawning or in transition, ensure player cannot fire
    if (player_) {
        if (phase_ == Phase::Spawn || inPhaseTransition_) {
            player_->SetCanFire(false);
            player_->ClearBarriers();
        }
    }
}

void Boss::Initialize(Object3d* model, Camera* camera, const Vector3 pos, Object3dCom* object3dCom, SpriteCom* spriteCom)
{
    model_ = model;
    camera_ = camera;
    object3dCom_ = object3dCom;
    spriteCom_ = spriteCom;

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

        
        float idx = static_cast<float>(i);
        float rings = 3.0f; 
        float angleStep = 0.8f; 
        float radiusBase = 10.0f;
        float radiusVar = 6.0f;  
        float angle = idx * angleStep;
        float ring = std::fmod(idx, rings);
        float radius = radiusBase + radiusVar * ring;

        
        Vector3 startLocal = {
            std::cos(angle) * radius,
            std::sin(angle) * radius * 0.6f + 6.0f,
            localTarget.z + 18.0f + ring * 4.0f
        };

       
        int dur = spawnDuration_ + static_cast<int>((idx * 6.0f));
        p->StartSpawn(startLocal, dur);
    }

    isActive_ = true;
    phase_ = Phase::Spawn;
    spawnTimer_ = 0;
    hp_ = maxHP_;
    hitCount_ = 0;
    hitCooldownTimer_ = 0;

    // disable player firing during spawn motion
    if (player_) {
        player_->SetCanFire(false);
        player_->ClearBarriers();
    }

    // デバッグ用スプライト作成 
    if (spriteCom_)
    {
        for (int i = 0; i < 5; ++i)
        {
            std::string fileName = std::to_string(i + 1) + ".png";
            std::string path = FindNumberTexturePath(fileName);
            if (path.empty())
            {
                phaseSprites_[i] = nullptr;
                continue;
            }

            Sprite* s = spriteCom_->CreateSprite(path, phaseSpritePosition_, phaseSpriteScale_, 0.0f, { 0.0f,0.0f }, false, false);
            phaseSprites_[i] = s;
            Vector4 c = s->GetColor();
            c.w = 0.0f;
            s->SetColor(c);
            s->Update();
        }
    }

    // レーザーモデル生成（弾と同じモデルを複製）
    // Phase4 専用。初期は非表示。
    if (model_)
    {
        laserModel_ = new Object3d();
        laserModel_->Initialize(object3dCom_);
        if (auto* src = model_->GetModel())
        {
            laserModel_->SetModel(new Model(*src));
            // 色を変えてレーザーらしく
            laserModel_->SetColor(laserChargeColor_);
        }
        // 初期スケール（発射方向Zへ伸ばす想定、後で UpdatePhase4 で調整）
        laserModel_->SetScale({ laserWidth_, laserWidth_, 1.0f });
        // 画面外に退避
        laserModel_->SetTranslate({ 10000.0f, 10000.0f, 10000.0f });
        laserModel_->ApplyState(Transform{ laserModel_->GetScale(), {0,0,0}, laserModel_->GetTranslate() }, camera_, true);
    }

    drones_.clear();
    drones_.resize(phase3DroneCount_);
    droneObjs_.clear();

    const float twoPi = 2.0f * 3.14159265f;
    for (int i = 0; i < phase3DroneCount_; ++i)
    {
        float ang = twoPi * static_cast<float>(i) / static_cast<float>(phase3DroneCount_);
        drones_[i].angle = ang;
        drones_[i].radius = 2.4f;
        drones_[i].orbitSpeed = 1.2f; 
        drones_[i].shootTimer = 0;
        drones_[i].shootInterval = 45 + (i * 5); 
        drones_[i].active = true;

        Object3d* dObj = new Object3d();
        dObj->Initialize(object3dCom_);
        if (model_ && model_->GetModel())
        {
            dObj->SetModel(new Model(*model_->GetModel()));
            dObj->SetScale({ droneModelScale_, droneModelScale_, droneModelScale_ });
            dObj->SetColor({ 0.7f, 0.9f, 1.0f, 1.0f });
        }

        dObj->SetTranslate({ 10000.0f, 10000.0f, 10000.0f });
        dObj->ApplyState(Transform{ dObj->GetScale(), dObj->GetRotate(), dObj->GetTranslate() }, camera_, true);
        droneObjs_.push_back(dObj);
    }

    // Initialize Phase4 turrets separately
    phase4Turrets_.clear();
    phase4Turrets_.resize(phase4TurretCount_);
    for (int i = 0; i < phase4TurretCount_; ++i)
    {
        Object3d* tObj = new Object3d();
        tObj->Initialize(object3dCom_);
        if (model_ && model_->GetModel())
        {
            tObj->SetModel(new Model(*model_->GetModel()));
            tObj->SetScale({ phase4TurretScale_, phase4TurretScale_, phase4TurretScale_ });
            tObj->SetColor({ 1.0f, 0.3f, 0.3f, 1.0f });
        }
        tObj->SetTranslate({ 10000.0f, 10000.0f, 10000.0f });
        tObj->ApplyState(Transform{ tObj->GetScale(), tObj->GetRotate(), tObj->GetTranslate() }, camera_, true);
        phase4Turrets_[i].obj = tObj;
        // ensure some turrets will fire quickly for testing
        phase4Turrets_[i].fireInterval = 24; // shorter interval for Phase4 turrets to be noticeable
        phase4Turrets_[i].fireTimer = phase4Turrets_[i].fireInterval - 1;
        phase4Turrets_[i].active = true;
    }
}

void Boss::OnHit()
{
    if (phase_ == Phase::Spawn || phase_ == Phase::Leave) return;
    if (hitCooldownTimer_ > 0) return;

    ++hitCount_;
    if (hitCount_ > hitsPerFull) hitCount_ = hitsPerFull;

    int clampedIndex = (std::min)(hitCount_, hitsPerFull - 1);
    Phase newPhase = static_cast<Phase>(static_cast<int>(Phase::Phase1) + clampedIndex);
    phase_ = newPhase;

    // start visual transition
    StartPhaseTransition();

    if (camera_ && cameraShakeCooldown_ <= 0.0f)
    {
        camera_->StartShake(0.2f, 0.2f);
        cameraShakeCooldown_ = kCameraShakeCooldownSeconds;
    }

    if (spriteCom_)
    {
        int currentPhaseIndex = (phase_ >= Phase::Phase1 && phase_ <= Phase::Phase5) ?
            static_cast<int>(phase_) - static_cast<int>(Phase::Phase1) : 0;
        for (int i = 0; i < 5; ++i)
        {
            Sprite* s = phaseSprites_[i];
            if (!s) continue;
            Vector4 col = s->GetColor();
            col.w = (currentPhaseIndex == i) ? 1.0f : 0.0f;
            s->SetColor(col);
            s->Update();
        }
    }

    hitCooldownTimer_ = kHitCooldownFrames;

    if (hitCount_ >= hitsPerFull)
    {
        OnCollision();
    }
}

void Boss::Shoot()
{
    // don't shoot while in phase transition
    if (inPhaseTransition_) return;

    ++ShootTimer_;
    if (ShootTimer_ < ShootInterval_) return;
    ShootTimer_ = 0;

    std::vector<int> aliveIndices;
    aliveIndices.reserve(parts_.size());
    for (size_t i = 0; i < parts_.size(); ++i)
    {
        if (parts_[i] && !parts_[i]->IsDestroyed()) aliveIndices.push_back(static_cast<int>(i));
    }

    for (int i = 0; i < phase1BulletsPerShot_; ++i)
    {
        Vector3 spawnPos;
        if (!aliveIndices.empty())
        {
            float r = Random::GeneratorFloat(0.0f, static_cast<float>(aliveIndices.size() - 1));
            int pick = static_cast<int>(std::floor(r + 0.5f));
            if (pick < 0) pick = 0;
            if (pick >= static_cast<int>(aliveIndices.size())) pick = static_cast<int>(aliveIndices.size() - 1);
            spawnPos = parts_[aliveIndices[pick]]->GetWorldTranslate();
        }
        else
        {
            spawnPos = worldTransform_.GetTranslate();
        }

        Vector3 dir = { 0.0f, 0.0f, -1.0f };
        Vector3 vel = { dir.x * phase1BulletSpeed_, dir.y * phase1BulletSpeed_, dir.z * phase1BulletSpeed_ };

        EnemyBullet* b = new EnemyBullet();
        b->Initialize(model_, spawnPos, object3dCom_, vel);
        bullets_.push_back(b);
    }
}

void Boss::Move()
{
    const float dt = 1.0f / 60.0f;
    phase1Time_ += dt;

    Vector3 baseTarget = spawnTarget_;
    if (player_)
    {
        Vector3 playerPos = player_->GetWorldTranslate();
        baseTarget.x = playerPos.x;
        baseTarget.z = playerPos.z + 5.0f;
    }

    const float twoPi = 2.0f * 3.14159265f;
    float angleX = twoPi * phase1OrbitSpeedX_ * phase1Time_;
    float angleY = twoPi * phase1OrbitSpeedY_ * phase1Time_;

    float offsetX = std::cos(angleX) * phase1OrbitRadiusX_;
    float offsetY = std::sin(angleY) * phase1OrbitRadiusY_;

    phase1TargetOffset_.x = offsetX;
    phase1TargetOffset_.y = offsetY;
    phase1TargetOffset_.z = 0.0f;

    Vector3 desiredTarget = { baseTarget.x + phase1TargetOffset_.x,
                         baseTarget.y + phase1TargetOffset_.y,
                         baseTarget.z };

    Vector3 cur = worldTransform_.GetTranslate();

    float dirX = desiredTarget.x - cur.x;
    float distX = std::fabs(dirX);
    if (distX > 0.0001f)
    {
        float moveDist = phase1MoveSpeed_ * 60.0f * dt;
        if (moveDist > distX) moveDist = distX;
        cur.x += (dirX > 0.0f ? 1.0f : -1.0f) * moveDist;
    }

    float bob = phase1BobAmplitude_ * sinf(twoPi * phase1BobFrequency_ * phase1Time_);
    cur.y = baseTarget.y + phase1TargetOffset_.y + bob;

    worldTransform_.SetTranslate(cur);
}

void Boss::UpdatePhase1()
{
    Move();
    Shoot();
}

void Boss::UpdatePhase2()
{
    // don't shoot while in phase transition
    if (inPhaseTransition_) return;

    ++ShootTimer_;
    if (ShootTimer_ < phase2ShootInterval_) return;
    ShootTimer_ = 0;

    std::vector<int> aliveIndices;
    aliveIndices.reserve(parts_.size());
    for (size_t i = 0; i < parts_.size(); ++i)
    {
        if (parts_[i] && !parts_[i]->IsDestroyed()) aliveIndices.push_back(static_cast<int>(i));
    }

    for (int i = 0; i < phase2BulletsPerShot_; ++i)
    {
        Vector3 spawnPos;
        if (!aliveIndices.empty())
        {
            float r = Random::GeneratorFloat(0.0f, static_cast<float>(aliveIndices.size() - 1));
            int pick = static_cast<int>(std::floor(r + 0.5f));
            if (pick < 0) pick = 0;
            if (pick >= static_cast<int>(aliveIndices.size())) pick = static_cast<int>(aliveIndices.size() - 1);
            spawnPos = parts_[aliveIndices[pick]]->GetWorldTranslate();
        }
        else
        {
            spawnPos = worldTransform_.GetTranslate();
        }

        Vector3 dir = { 0.0f, 0.0f, -1.0f };
        Vector3 vel = { dir.x * phase2BulletSpeed_, dir.y * phase2BulletSpeed_, dir.z * phase2BulletSpeed_ };

        EnemyBullet* b = new EnemyBullet();
        b->Initialize(model_, spawnPos, object3dCom_, vel);
        b->EnableHoming(player_, phase2BulletSpeed_, phase2TurnRate_);
        b->SetLifeDuration(phase2BulletLifeFrames_);
        bullets_.push_back(b);
    }
}

void Boss::UpdatePhase3()
{
    if (!isActive_ || inPhaseTransition_) return;

    const float dt = 1.0f / 60.0f;
    const float twoPi = 2.0f * 3.14159265f;
    Vector3 bossPos = worldTransform_.GetTranslate();

    for (size_t i = 0; i < drones_.size(); ++i)
    {
        Drone& d = drones_[i];
        if (!d.active) continue;

        // advance orbit
        d.angle += d.orbitSpeed * dt;
        if (d.angle > twoPi) d.angle -= twoPi;

        Vector3 dronePos = bossPos;
        dronePos.x += std::cos(d.angle) * d.radius;
        dronePos.y += std::sin(d.angle) * d.radius;
        dronePos.z = bossPos.z; 

        if (i < droneObjs_.size() && droneObjs_[i])
        {
            Object3d* dobj = droneObjs_[i];
            dobj->SetTranslate(dronePos);
            dobj->SetScale({ droneModelScale_, droneModelScale_, droneModelScale_ });
            dobj->ApplyState(Transform{ dobj->GetScale(), dobj->GetRotate(), dobj->GetTranslate() }, camera_, true);
        }

        ++d.shootTimer;
        if (d.shootTimer >= d.shootInterval)
        {
            d.shootTimer = 0;

            Vector3 target = GetPlayerWorldTranslate();
            Vector3 baseDir = { target.x - dronePos.x, target.y - dronePos.y, target.z - dronePos.z };

            float len = std::sqrt(baseDir.x*baseDir.x + baseDir.y*baseDir.y + baseDir.z*baseDir.z);
            if (len < 1e-6f) baseDir = { 0.0f, 0.0f, -1.0f };
            else baseDir = { baseDir.x / len, baseDir.y / len, baseDir.z / len };

            const int coneCount = 3;
            const float coneAngle = 0.18f; 
            for (int ci = 0; ci < coneCount; ++ci)
            {
                float t = 0.0f;
                if (coneCount > 1) t = (static_cast<float>(ci) / (coneCount - 1)) - 0.5f; // -0.5..0.5
                float angOff = t * coneAngle;

                float dx = baseDir.x;
                float dy = baseDir.y;
                float dz = baseDir.z;
                float ca = std::cos(angOff);
                float sa = std::sin(angOff);
                Vector3 dir = { ca * dx - sa * dy, sa * dx + ca * dy, dz };

                float l2 = std::sqrt(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
                if (l2 > 1e-6f) { dir.x /= l2; dir.y /= l2; dir.z /= l2; }

                Vector3 vel = { dir.x * phase3BulletSpeed_, dir.y * phase3BulletSpeed_, dir.z * phase3BulletSpeed_ };

                EnemyBullet* b = new EnemyBullet();
                b->Initialize(model_, dronePos, object3dCom_, vel);
                bullets_.push_back(b);
            }
        }
    }

    for (EnemyBullet* b : bullets_)
    {
        if (b) b->Update();
    }

    ++phase3Timer_;
    if (phase3Timer_ > 60)
    {
        phase3Timer_ = 0;
        if (camera_ && cameraShakeCooldown_ <= 0.0f)
        {
            camera_->StartShake(0.15f, 0.12f);
            cameraShakeCooldown_ = kCameraShakeCooldownSeconds;
        }
    }
}

void Boss::UpdatePhase4()
{
    if (inPhaseTransition_) return;
    if (!laserModel_) return;

    const float twoPi = 2.0f * 3.14159265f;
    Vector3 bossPos = worldTransform_.GetTranslate();

    // initialize on first entry
    if (!laserActive_)
    {
        laserActive_ = true;
        laserTimer_ = 0;
        laserBulletTimer_ = 0;
        laserSweepAngle_ = 0.0f;
        laserAimX_ = 0.0f;

        // position turrets around boss
        for (int i = 0; i < phase4TurretCount_; ++i)
        {
            float ang = twoPi * static_cast<float>(i) / static_cast<float>(phase4TurretCount_);
            Vector3 pos = bossPos + Vector3{ std::cos(ang) * phase4TurretRadius_, std::sin(ang) * phase4TurretRadius_, 0.0f };
            if (i < static_cast<int>(phase4Turrets_.size()) && phase4Turrets_[i].obj)
            {
                auto* tobj = phase4Turrets_[i].obj;
                tobj->SetTranslate(pos);
                tobj->SetScale({ phase4TurretScale_, phase4TurretScale_, phase4TurretScale_ });
                tobj->SetRotate({ 0.0f, 0.0f, 0.0f });
                tobj->ApplyState(Transform{ tobj->GetScale(), tobj->GetRotate(), tobj->GetTranslate() }, camera_, true);
                // make them ready to fire immediately for visibility
                phase4Turrets_[i].fireTimer = phase4Turrets_[i].fireInterval - 1;
                phase4Turrets_[i].active = true;
            }
        }

        // short visual on boss
        Vector3 laserPos = bossPos + Vector3{ 0.0f, 0.0f, -2.0f };
        laserModel_->SetTranslate(laserPos);
        laserModel_->SetRotate({ 0.0f,0.0f,0.0f });
        laserModel_->SetScale({ laserWidth_, laserWidth_, 1.0f });
        laserModel_->SetColor(laserChargeColor_);
        laserModel_->ApplyState(Transform{ laserModel_->GetScale(), laserModel_->GetRotate(), laserModel_->GetTranslate() }, camera_, true);

        auto* pm = ParticleManager::GetInstance();
        if (pm) pm->EmitBurst8("default", bossPos, 0.12f, 1.6f, 0.6f);
    }

    ++laserTimer_;
    int total = laserChargeFrames_ + laserFireFrames_ + laserCooldownFrames_;

    if (laserTimer_ <= laserChargeFrames_)
    {
        // charging: short visible core, turrets idle (but visible)
        float t = static_cast<float>(laserTimer_) / static_cast<float>(laserChargeFrames_);
        float alpha = 0.5f + 0.5f * std::sin(t * 3.14159265f);
        Vector4 col = laserChargeColor_;
        col.w = alpha;
        laserModel_->SetScale({ laserWidth_, laserWidth_, 1.0f });
        laserModel_->SetTranslate(bossPos + Vector3{ 0.0f, 0.0f, -0.5f });
        laserModel_->SetColor(col);

        // update turret visuals
        for (auto &tur : phase4Turrets_)
        {
            if (tur.obj)
            {
                tur.obj->ApplyState(Transform{ tur.obj->GetScale(), tur.obj->GetRotate(), tur.obj->GetTranslate() }, camera_, true);
            }
        }
    }
    else if (laserTimer_ <= laserChargeFrames_ + laserFireFrames_)
    {
        // firing period: turrets shoot bullets toward player periodically; central beam is a visual core
        float length = laserMaxLength_;
        laserModel_->SetScale({ laserWidth_ * 0.4f, laserWidth_ * 0.4f, length * 0.6f });
        laserModel_->SetTranslate(bossPos + Vector3{ 0.0f, 0.0f, -length * 0.5f * 0.6f });
        laserModel_->SetColor(laserFireColor_);

        // turrets fire
        for (auto &tur : phase4Turrets_)
        {
            if (!tur.active || !tur.obj) continue;
            ++tur.fireTimer;
            if (tur.fireTimer >= tur.fireInterval)
            {
                tur.fireTimer = 0;
                Vector3 tp = tur.obj->GetTranslate();
                Vector3 dir = { 0.0f, 0.0f, -1.0f };
                if (player_)
                {
                    Vector3 p = player_->GetWorldTranslate();
                    dir = { p.x - tp.x, p.y - tp.y, p.z - tp.z };
                    float l = std::sqrt(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
                    if (l > 1e-6f) dir = { dir.x / l, dir.y / l, dir.z / l };
                    else dir = { 0.0f, 0.0f, -1.0f };
                }

                Vector3 vel = { dir.x * phase4BulletSpeed_, dir.y * phase4BulletSpeed_, dir.z * phase4BulletSpeed_ };
                EnemyBullet* b = new EnemyBullet();
                b->Initialize(model_, tp, object3dCom_, vel);
                bullets_.push_back(b);

                auto* pm = ParticleManager::GetInstance();
                if (pm) pm->EmitBurst8("default", tp, 0.08f, 0.6f, 0.45f);

                if (camera_ && cameraShakeCooldown_ <= 0.0f)
                {
                    camera_->StartShake(0.08f, 0.08f);
                    cameraShakeCooldown_ = kCameraShakeCooldownSeconds;
                }
            }

            // update turret visual
            tur.obj->ApplyState(Transform{ tur.obj->GetScale(), tur.obj->GetRotate(), tur.obj->GetTranslate() }, camera_, true);
        }

        // central laser collision check (short core)
        if (player_ && player_->IsAlive())
        {
            Vector3 p = player_->GetWorldTranslate();
            Vector3 lp = laserModel_->GetTranslate();
            Vector3 laserHalf = { (laserWidth_ * 0.5f) + laserPlayerHitPaddingXY_, (laserWidth_ * 0.5f) + laserPlayerHitPaddingXY_, (length * 0.5f) + laserPlayerHitPaddingZ_ };
            Vector3 playerHalf = { playerHitHalfSizeXY_, playerHitHalfSizeXY_, playerHitHalfSizeZ_ };

            AABB laserBox{ { lp.x - laserHalf.x, lp.y - laserHalf.y, lp.z - laserHalf.z }, { lp.x + laserHalf.x, lp.y + laserHalf.y, lp.z + laserHalf.z } };
            AABB playerBox{ { p.x - playerHalf.x, p.y - playerHalf.y, p.z - playerHalf.z }, { p.x + playerHalf.x, p.y + playerHalf.y, p.z + playerHalf.z } };

            if (IsCollisionAABBAABB(laserBox, playerBox))
            {
                player_->OnCollision();
                if (camera_) camera_->StartShake(0.6f, 0.45f);
            }
        }
    }
    else if (laserTimer_ <= total)
    {
        // cooldown: shrink and fade
        int coolT = laserTimer_ - (laserChargeFrames_ + laserFireFrames_);
        float tf = static_cast<float>(coolT) / static_cast<float>(laserCooldownFrames_);
        float length = laserMaxLength_ * (1.0f - tf);
        if (length < 1.0f) length = 1.0f;
        // keep the beam's X/Y thickness consistent with firing visual to avoid sudden jump
        laserModel_->SetScale({ laserWidth_ * 0.4f, laserWidth_ * 0.4f, length });
        laserModel_->SetTranslate(bossPos + Vector3{ 0.0f, 0.0f, -length * 0.5f });
        Vector4 col = laserFireColor_;
        col.w = 1.0f - tf;
        laserModel_->SetColor(col);

        // keep turret visuals while cooling
        for (auto &tur : phase4Turrets_)
        {
            if (tur.obj) tur.obj->ApplyState(Transform{ tur.obj->GetScale(), tur.obj->GetRotate(), tur.obj->GetTranslate() }, camera_, true);
        }

        // collision still active during cooldown
        if (player_ && player_->IsAlive())
        {
            Vector3 p = player_->GetWorldTranslate();
            Vector3 lp = laserModel_->GetTranslate();
            Vector3 laserHalf = { (laserWidth_ * 0.5f) + laserPlayerHitPaddingXY_, (laserWidth_ * 0.5f) + laserPlayerHitPaddingXY_, (length * 0.5f) + laserPlayerHitPaddingZ_ };
            Vector3 playerHalf = { playerHitHalfSizeXY_, playerHitHalfSizeXY_, playerHitHalfSizeZ_ };

            AABB laserBox{ { lp.x - laserHalf.x, lp.y - laserHalf.y, lp.z - laserHalf.z }, { lp.x + laserHalf.x, lp.y + laserHalf.y, lp.z + laserHalf.z } };
            AABB playerBox{ { p.x - playerHalf.x, p.y - playerHalf.y, p.z - playerHalf.z }, { p.x + playerHalf.x, p.y + playerHalf.y, p.z + playerHalf.z } };

            if (IsCollisionAABBAABB(laserBox, playerBox))
            {
                player_->OnCollision();
                if (camera_) camera_->StartShake(0.45f, 0.35f);
            }
        }
    }
    else
    {
        // end cycle: hide laser and turrets until next activation
        laserActive_ = false;
        laserTimer_ = 0;
        laserModel_->SetTranslate({ 10000.0f, 10000.0f, 10000.0f });
        for (auto &tur : phase4Turrets_)
        {
            if (tur.obj) tur.obj->SetTranslate({ 10000.0f, 10000.0f, 10000.0f });
            tur.active = true;
            tur.fireTimer = static_cast<int>(Random::GeneratorFloat(0.0f, static_cast<float>(tur.fireInterval)));
        }
    }

    // apply matrix for laser
    laserModel_->ApplyState(Transform{ laserModel_->GetScale(), laserModel_->GetRotate(), laserModel_->GetTranslate() }, camera_, true);
}

// --- Phase5: スパイラル / ラジアル攻撃の実装 ---
void Boss::UpdatePhase5()
{
    if (!isActive_ || inPhaseTransition_) return;

    ++phase5BurstTimer_;

    // 毎 interval フレームごとにバースト（レール）を生成
    if (phase5BurstTimer_ >= phase5BurstInterval_)
    {
        phase5BurstTimer_ = 0;
        ++phase5BurstCount_;

        Vector3 bossPos = worldTransform_.GetTranslate();

        int lanes = phase5BulletsPerBurst_;
        float half = (static_cast<float>(lanes - 1) * 0.5f);

        // スイープ方向の変化量（振幅 -1..1）
        float sweepOsc = std::sin(phase5Angle_);

        for (int i = 0; i < lanes; ++i)
        {
            float laneOffset = (static_cast<float>(i) - half) * phase5LaneSpacing_;
            Vector3 spawnPos = bossPos + Vector3{ laneOffset, 0.0f, -1.0f };

            // 基本はまっすぐ手前へ進む
            Vector3 vel = { 0.0f, 0.0f, -phase5BulletSpeed_ };

            if (phase5SweepMode_)
            {
                // スイープモード: 弾全体が横に流れる（レールゲームでの横スクロールのような見た目）
                vel.x = sweepOsc * phase5SweepSpeed_;
            }
            else
            {
                // 非スイープ: 各レーンの弾をプレイヤー方向に少し寄せることで狙い感を出す
                if (player_)
                {
                    Vector3 p = player_->GetWorldTranslate();
                    float dx = p.x - spawnPos.x;
                    // 横成分を弱めに反映（プレイヤーの軸にゆっくり寄る）
                    vel.x = dx * 0.08f;
                    // 制限をかける
                    if (vel.x > phase5SweepSpeed_) vel.x = phase5SweepSpeed_;
                    if (vel.x < -phase5SweepSpeed_) vel.x = -phase5SweepSpeed_;
                }
            }

            EnemyBullet* b = new EnemyBullet();
            b->Initialize(model_, spawnPos, object3dCom_, vel);
            bullets_.push_back(b);
        }

        // 次のスイープ位相を進める
        phase5Angle_ += phase5SpinRate_;
        const float twoPiF = 2.0f * 3.14159265f;
        if (phase5Angle_ > twoPiF) phase5Angle_ -= twoPiF;

        // 発射時に軽いカメラ振動を入れて演出
        if (camera_ && cameraShakeCooldown_ <= 0.0f)
        {
            camera_->StartShake(0.12f, 0.12f);
            cameraShakeCooldown_ = kCameraShakeCooldownSeconds;
        }
    }

    // ここで弾の更新（Update でも二重更新されるが他フェーズに合わせて維持）
    for (EnemyBullet* b : bullets_)
    {
        if (b) b->Update();
    }
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

    if (hitCooldownTimer_ > 0) --hitCooldownTimer_;

    const float dt = 1.0f / 60.0f;
    if (cameraShakeCooldown_ > 0.0f)
    {
        cameraShakeCooldown_ -= dt;
        if (cameraShakeCooldown_ < 0.0f) cameraShakeCooldown_ = 0.0f;
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
            phase_ = Phase::Phase1;
            if (camera_ && cameraShakeCooldown_ <= 0.0f)
            {
                camera_->StartShake(0.6f, 0.6f);
                cameraShakeCooldown_ = kCameraShakeCooldownSeconds;
            }
            // re-enable player firing after spawn completed
            if (player_) {
                player_->SetCanFire(true);
            }
        }
    }

    UpdatePhaseByHP();

    // if entering Phase4 start pre-motion
    if (phase_ == Phase::Phase4 && lastPhase_ != Phase::Phase4 && !currentMotion_)
    {
        StartLaserPreMotion();
    }

    if (spriteCom_)
    {
        for (int i = 0; i < 5; ++i)
        {
            Sprite* s = phaseSprites_[i];
            if (!s) continue;
            Vector4 c = s->GetColor();
            int currentPhaseIndex = 0;
            if (phase_ >= Phase::Phase1 && phase_ <= Phase::Phase5)
            {
                currentPhaseIndex = static_cast<int>(phase_) - static_cast<int>(Phase::Phase1);
            }
            c.w = (currentPhaseIndex == i) ? 1.0f : 0.0f;
            s->SetColor(c);
            s->Update();
        }
    }

    if (currentMotion_)
    {
        currentMotion_->Update(this, dt);
        if (currentMotion_->IsFinished())
        {
            currentMotion_.reset();
        }
    }

    // phase transition timer handling: keep player firing disabled during transition
    if (inPhaseTransition_)
    {
        ++phaseTransitionTimer_;
        // only end transition after both the timer expires AND any motion (visual pre-motion) finished
        bool motionFinished = (currentMotion_ == nullptr);
        if (phaseTransitionTimer_ >= phaseTransitionDuration_ && motionFinished)
        {
            inPhaseTransition_ = false;
            phaseTransitionTimer_ = 0;
            if (player_) player_->SetCanFire(true);
        }
    }

    if (phase_ == Phase::Phase1)
    {
        UpdatePhase1();
    }
    else if (phase_ == Phase::Phase2)
    {
        UpdatePhase2();
    }
    else if (phase_ == Phase::Phase3)
    {
        UpdatePhase3();
    }
    else if (phase_ == Phase::Phase4)
    {
        if (!currentMotion_)
        {
            UpdatePhase4();
        }
    }
    else if (phase_ == Phase::Phase5)
    {
        UpdatePhase5();
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

    lastPhase_ = phase_;
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

   
    if (phase_ == Phase::Phase3)
    {
        for (auto* o : droneObjs_)
        {
            if (o) o->Draw();
        }
    }

    if (phase_ == Phase::Phase4 && laserModel_ && laserActive_)
    {
        // draw turrets first
        for (auto &t : phase4Turrets_)
        {
            if (t.obj) t.obj->Draw();
        }
        laserModel_->Draw();
    }

    if (spriteCom_)
    {
        for (int i = 0; i < 5; ++i)
        {
            Sprite* s = phaseSprites_[i];
            if (!s) continue;
            s->Draw();
        }
    }
}

void Boss::OnCollision()
{
    worldTransform_.SetTranslate({ 10000.0f, 10000.0f, 10000.0f });
    for (EnemyBullet* b : bullets_)
    {
        if (b) b->OnCollision();
    }
    // レーザーも退避
    if (laserModel_)
    {
        laserModel_->SetTranslate({ 10000.0f, 10000.0f, 10000.0f });
    }
    isActive_ = false;
}

void Boss::StartPhaseTransition()
{
    auto* pm = ParticleManager::GetInstance();
    Vector3 center = worldTransform_.GetTranslate();

    // mark transition state and set timer
    inPhaseTransition_ = true;
    phaseTransitionTimer_ = 0; // will be incremented in Update

    // disable player firing during transition if player exists
    if (player_) {
        player_->SetCanFire(false);
        // also clear any active barriers to avoid instant collision during transition
        player_->ClearBarriers();
    }

    if (pm) {
        // If mesh-group exists, favor mesh particles (OBJ) for richer visuals
        if (pm->HasGroup("defaultMesh")) {
            // multiple rotating inward bursts (mesh)
            pm->EmitBurst8RotatingInward("defaultMesh", center, 8.0f, 1.3f, 3.0f, 0.9f, 2.6f, 0.8f);
            pm->EmitBurst8RotatingInward("defaultMesh", center, 5.0f, 1.1f, -4.2f, 0.6f, 2.2f, 0.6f);
            // add an outward burst using mesh for chunk pieces
            pm->EmitBurst8("defaultMesh", center, 0.12f, 0.7f, 1.0f);
        }

        // sprite/textured particles for glow and spark
        if (pm->HasGroup("default")) {
            pm->EmitBurst8RotatingInward("default", center, 6.0f, 0.9f, 4.0f, 0.9f, 1.6f, 0.4f);
            pm->EmitBurst8("default", center, 0.08f, 1.2f, 0.9f);
        }

        // a final, tight rotating ring of small sprites for impact
        if (pm->HasGroup("default")) {
            pm->EmitBurst8Rotating("default", center, 2.4f, 0.6f, 8.0f, 0.45f, false, -1.6f, -0.6f);
        }
    }

    // camera: strong shake
    if (camera_ && cameraShakeCooldown_ <= 0.0f) {
        camera_->StartShake(0.6f, 0.6f);
        cameraShakeCooldown_ = kCameraShakeCooldownSeconds;
    }

    // model: quick flash tint
    if (model_) {
        // set a bright tint briefly
        model_->SetColor({1.0f, 0.6f, 0.2f, 1.0f});
    }
}

// UpdatePhaseByHP: call StartPhaseTransition when phase changes
void Boss::UpdatePhaseByHP()
{
    if (phase_ == Phase::Spawn || phase_ == Phase::Leave) return;

    if (hitCount_ > 0)
    {
        int clampedIndex = (std::min)(hitCount_, hitsPerFull - 1);
        Phase newPhase = static_cast<Phase>(static_cast<int>(Phase::Phase1) + clampedIndex);
        if (newPhase != phase_)
        {
            phase_ = newPhase;
            StartPhaseTransition();
            if (camera_ && cameraShakeCooldown_ <= 0.0f)
            {
                camera_->StartShake(0.3f, 0.3f);
                cameraShakeCooldown_ = kCameraShakeCooldownSeconds;
            }
        }
        return; // ヒットベース優先なので HP による更新は行わない
    }

    if (maxHP_ <= 0) return;

    float hpRatio = static_cast<float>(hp_) / static_cast<float>(maxHP_);

    Phase newPhase = Phase::Phase1;

    for (size_t i = 0; i < hpPhaseThresholds_.size(); ++i)
    {
        float threshold = hpPhaseThresholds_[i];
        if (hpRatio <= threshold)
        {
            newPhase = static_cast<Phase>(static_cast<int>(Phase::Phase1) + static_cast<int>(i));
        }
    }

    if (newPhase != phase_)
    {
        phase_ = newPhase;
        // fancy transition
        StartPhaseTransition();
        if (camera_)
        {
            if (cameraShakeCooldown_ <= 0.0f)
            {
                camera_->StartShake(0.3f, 0.3f);
                cameraShakeCooldown_ = kCameraShakeCooldownSeconds;
            }
        }
    }
}
