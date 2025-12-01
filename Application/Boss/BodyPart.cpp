#include "BodyPart.h"
#include "Boss.h"

BodyPart::BodyPart() {}

BodyPart::~BodyPart()
{
    for (auto m : models_)
    {
        if (m) { delete m; }
    }
    models_.clear();
}

void BodyPart::Initialize(Boss* owner, Object3d* model, const Vector3& localPos)
{
    owner_ = owner;
    camera_ = owner ? owner->GetCamera() : nullptr;
    object3dCom_ = owner ? owner->GetObject3dCom() : nullptr;

    worldTransform_.Initialize();
    worldTransform_.SetTranslate(localPos);
    spawnTargetLocal_ = localPos;

    if (model && object3dCom_)
    {
      
        const int layers = 4;
        models_.reserve(layers);
        localTransforms_.reserve(layers);

        
        Vector3 baseScale = { 0.6f, 0.6f, 0.6f };
        Vector3 offsets[4] = {
            { -0.3f, -0.3f, 0.0f },
            { -0.3f,  0.3f, 0.0f },
            {  0.3f, -0.3f, 0.0f },
            {  0.3f,  0.3f, 0.0f }
        };

        for (int i = 0; i < layers; ++i)
        {
            Object3d* sub = new Object3d();
            sub->Initialize(object3dCom_);
            if (auto* src = model->GetModel())
            {
                sub->SetModel(new Model(*src));
            }

            
            Vector4 col = { 0.6f + 0.05f * i, 0.6f, 0.8f - 0.02f * i, 1.0f };
            sub->SetColor(col);

            Transform lt;
            lt.Initialize();
            lt.SetScale(baseScale);
            lt.SetTranslate(offsets[i]);

            models_.push_back(sub);
            localTransforms_.push_back(lt);
        }
    }
}

void BodyPart::StartSpawn(const Vector3& startLocal, int duration)
{
    isSpawning_ = true;
    spawnStartLocal_ = startLocal;
    spawnTimer_ = 0;
    spawnDuration_ = duration;

    worldTransform_.SetTranslate(spawnStartLocal_);
}

void BodyPart::UpdateSpawn(float progress)
{
    if (!isSpawning_) return;
    if (progress < 0.0f) progress = 0.0f;
    if (progress > 1.0f) progress = 1.0f;

    // ease-out
    float ease = 1.0f - (1.0f - progress) * (1.0f - progress);

    Vector3 cur = {
        spawnStartLocal_.x + (spawnTargetLocal_.x - spawnStartLocal_.x) * ease,
        spawnStartLocal_.y + (spawnTargetLocal_.y - spawnStartLocal_.y) * ease,
        spawnStartLocal_.z + (spawnTargetLocal_.z - spawnStartLocal_.z) * ease
    };
    worldTransform_.SetTranslate(cur);

    if (progress >= 1.0f)
    {
        isSpawning_ = false;
        // ensure final
        worldTransform_.SetTranslate(spawnTargetLocal_);
    }
}

void BodyPart::Update()
{
    if (hp_ <= 0) return;

    if (owner_)
    {
        Vector3 bossPos = owner_->GetWorldTranslate();

        for (size_t i = 0; i < models_.size(); ++i)
        {
            if (!models_[i]) continue;

            Transform t = localTransforms_[i];

            Vector3 lt = t.GetTranslate();
            t.SetTranslate({ lt.x + bossPos.x + worldTransform_.GetTranslate().x,
                             lt.y + bossPos.y + worldTransform_.GetTranslate().y,
                             lt.z + bossPos.z + worldTransform_.GetTranslate().z });
            t.TransferMatrix();
            if (camera_) models_[i]->ApplyState(t, camera_, true);
        }
    }
}

void BodyPart::Draw()
{
    if (hp_ <= 0) return;
    for (auto m : models_)
    {
        if (m) m->Draw();
    }
}

void BodyPart::OnDamage(int dmg)
{
    hp_ -= dmg;
    if (hp_ < 0) hp_ = 0;

    // ボディがダメージを受けたらボスにヒット通知（フェーズ進行用）
    if (owner_)
    {
        owner_->OnHit();
    }
}

bool BodyPart::IsDestroyed() const
{
    return hp_ <= 0;
}

const Matrix4x4& BodyPart::GetWorldMatrix() const
{
    return worldTransform_.GetWorldMatrix();
}

Vector3 BodyPart::GetWorldTranslate() const
{
    Vector3 local = worldTransform_.GetTranslate();
    if (owner_)
    {
        Vector3 bossPos = owner_->GetWorldTranslate();
        return { local.x + bossPos.x, local.y + bossPos.y, local.z + bossPos.z };
    }
    return local;
}
