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

    // If a source model is provided, create several copies with different local transforms
    // to compose a rectangular, blocky body (like an EVA-like square-shaped angel)
    if (model && object3dCom_)
    {
        // We'll make a central block and several layered blocks around it
        const int layers = 4;
        models_.reserve(layers);
        localTransforms_.reserve(layers);

        // base sizes and offsets
        Vector3 baseScale = { 1.2f, 1.2f, 1.2f };
        Vector3 offsets[4] = {
            { 0.0f, 0.0f, 0.0f },
            { 0.0f, 0.6f, 0.0f },
            { 0.6f, 0.0f, 0.0f },
            { -0.6f, 0.0f, 0.0f }
        };

        for (int i = 0; i < layers; ++i)
        {
            Object3d* sub = new Object3d();
            sub->Initialize(object3dCom_);
            if (auto* src = model->GetModel())
            {
                sub->SetModel(new Model(*src));
            }

            // set color slightly varied
            Vector4 col = { 0.6f + 0.1f * i, 0.6f, 0.8f - 0.05f * i, 1.0f };
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

void BodyPart::Update()
{
    if (hp_ <= 0) return;

    if (owner_)
    {
        // compute boss world position
        Vector3 bossPos = owner_->GetWorldTranslate();

        // Update each local transform combined with boss world transform
        for (size_t i = 0; i < models_.size(); ++i)
        {
            if (!models_[i]) continue;

            Transform t = localTransforms_[i];
            // Add boss translation
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
