#include "TurretPart.h"
#include "Boss.h"
#include <cmath>

TurretPart::~TurretPart()
{
    if (model_)
    {
        delete model_;
        model_ = nullptr;
    }
}

void TurretPart::Initialize(Boss* owner, Object3d* model, const Vector3& localPos)
{
    owner_ = owner;
    camera_ = owner ? owner->GetCamera() : nullptr;
    object3dCom_ = owner ? owner->GetObject3dCom() : nullptr;

    if (model)
    {
        model_ = new Object3d();
        model_->Initialize(object3dCom_);
        if (auto* src = model->GetModel())
        {
            model_->SetModel(new Model(*src));
        }
    }

    worldTransform_.Initialize();
    worldTransform_.SetTranslate(localPos);

    fireTimer_ = fireInterval_;
}

static inline float LocalLength(const Vector3& v)
{
    return std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
}

static inline Vector3 LocalNormalize(const Vector3& v)
{
    float len = LocalLength(v);
    if (len <= 1e-6f) return {0.0f, 0.0f, 0.0f};
    return { v.x / len, v.y / len, v.z / len };
}

void TurretPart::Update()
{
    if (hp_ <= 0) return;

    if (owner_)
    {
        Transform t = worldTransform_;
        Vector3 bossPos = owner_->GetWorldTranslate();
        t.SetTranslate({ t.GetTranslate().x + bossPos.x, t.GetTranslate().y + bossPos.y, t.GetTranslate().z + bossPos.z });
        t.TransferMatrix();
        if (model_ && camera_) model_->ApplyState(t, camera_, true);
    }

    if (--fireTimer_ <= 0)
    {
        // Use boss wrapper to get player position to avoid depending on Player definition here
        if (owner_ && owner_->GetPlayer())
        {
            Object3d* bulletModel = new Object3d();
            bulletModel->Initialize(object3dCom_);
            if (model_)
            {
                if (auto* src = model_->GetModel())
                {
                    bulletModel->SetModel(new Model(*src));
                    const Vector4 bulletColor{ 1.0f, 0.2f, 0.2f, 1.0f };
                    bulletModel->GetModel()->SetColor(bulletColor);
                    bulletModel->SetColor(bulletColor);
                }
            }

            EnemyBullet* b = new EnemyBullet();
            Vector3 spawnPos = GetWorldTranslate();
            spawnPos.z -= 1.0f;

            Vector3 enemyPos = GetWorldTranslate();
            Vector3 playerPos = owner_->GetPlayerWorldTranslate();
            Vector3 dir{ playerPos.x - enemyPos.x, playerPos.y - enemyPos.y, playerPos.z - enemyPos.z };
            Vector3 normDir = LocalNormalize(dir);
            const float kBulletSpeed = 1.0f;
            Vector3 bulletVelocity{ 0.0f, 0.0f, -kBulletSpeed };
            if (LocalLength(dir) > 1e-6f)
            {
                bulletVelocity = normDir * kBulletSpeed;
            }

            b->Initialize(bulletModel, spawnPos, object3dCom_, bulletVelocity);
            owner_->RegisterBullet(b);
        }

        fireTimer_ = fireInterval_;
    }
}

void TurretPart::Draw()
{
    if (hp_ <= 0) return;
    if (model_)
    {
        model_->Draw();
    }
}

void TurretPart::OnDamage(int dmg)
{
    hp_ -= dmg;
    if (hp_ < 0) hp_ = 0;
}

bool TurretPart::IsDestroyed() const
{
    return hp_ <= 0;
}

const Matrix4x4& TurretPart::GetWorldMatrix() const
{
    return worldTransform_.GetWorldMatrix();
}

Vector3 TurretPart::GetWorldTranslate() const
{
    Vector3 local = worldTransform_.GetTranslate();
    if (owner_)
    {
        Vector3 bossPos = owner_->GetWorldTranslate();
        return { local.x + bossPos.x, local.y + bossPos.y, local.z + bossPos.z };
    }
    return local;
}
