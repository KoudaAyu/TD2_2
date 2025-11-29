#include "HeadPart.h"
#include "Boss.h"

HeadPart::HeadPart() {}

HeadPart::~HeadPart()
{
    for (auto m : models_)
    {
        if (m) { delete m; }
    }
    models_.clear();
}

void HeadPart::Initialize(Boss* owner, Object3d* model, const Vector3& localPos)
{
    owner_ = owner;
    camera_ = owner ? owner->GetCamera() : nullptr;
    object3dCom_ = owner ? owner->GetObject3dCom() : nullptr;

    worldTransform_.Initialize();
    worldTransform_.SetTranslate(localPos);

    if (model && object3dCom_)
    {
        // create several stacked cubes/boxes to make a square/rectangular head
        const int parts = 3;
        models_.reserve(parts);
        localTransforms_.reserve(parts);

        Vector3 scales[3] = { {1.0f,1.0f,1.0f}, {0.8f,0.8f,0.8f}, {0.6f,0.6f,0.6f} };
        Vector3 offsets[3] = { {0.0f,0.4f,0.0f}, {0.0f,0.0f,0.0f}, {0.0f,-0.4f,0.0f} };

        for (int i = 0; i < parts; ++i)
        {
            Object3d* sub = new Object3d();
            sub->Initialize(object3dCom_);
            if (auto* src = model->GetModel())
            {
                sub->SetModel(new Model(*src));
            }
            Vector4 col = { 0.9f - 0.1f*i, 0.6f, 0.6f, 1.0f };
            sub->SetColor(col);

            Transform lt;
            lt.Initialize();
            lt.SetScale(scales[i]);
            lt.SetTranslate(offsets[i]);

            models_.push_back(sub);
            localTransforms_.push_back(lt);
        }
    }
}

void HeadPart::Update()
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

void HeadPart::Draw()
{
    if (hp_ <= 0) return;
    for (auto m : models_)
    {
        if (m) m->Draw();
    }
}

void HeadPart::OnDamage(int dmg)
{
    hp_ -= dmg;
    if (hp_ <= 0)
    {
        hp_ = 0;
        if (owner_)
        {
            owner_->OnCollision();
        }
    }
}

bool HeadPart::IsDestroyed() const
{
    return hp_ <= 0;
}

const Matrix4x4& HeadPart::GetWorldMatrix() const
{
    return worldTransform_.GetWorldMatrix();
}

Vector3 HeadPart::GetWorldTranslate() const
{
    Vector3 local = worldTransform_.GetTranslate();
    if (owner_)
    {
        Vector3 bossPos = owner_->GetWorldTranslate();
        return { local.x + bossPos.x, local.y + bossPos.y, local.z + bossPos.z };
    }
    return local;
}
