#include "BodyPart.h"
#include "Boss.h"

BodyPart::BodyPart() {}

BodyPart::~BodyPart()
{
    if (model_)
    {
        delete model_;
        model_ = nullptr;
    }
}

void BodyPart::Initialize(Boss* owner, Object3d* model, const Vector3& localPos)
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
}

void BodyPart::Update()
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
}

void BodyPart::Draw()
{
    if (hp_ <= 0) return;
    if (model_)
    {
        model_->Draw();
    }
}

void BodyPart::OnDamage(int dmg)
{
    hp_ -= dmg;
    if (hp_ < 0) hp_ = 0;
    // 胴体が壊れても即座にボス全体を消すのはしない（ゲームバランス用）
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
