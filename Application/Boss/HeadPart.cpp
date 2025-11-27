#include "HeadPart.h"
#include "Boss.h"

HeadPart::HeadPart() {}

HeadPart::~HeadPart()
{
    if (model_)
    {
        delete model_;
        model_ = nullptr;
    }
}

void HeadPart::Initialize(Boss* owner, Object3d* model, const Vector3& localPos)
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

void HeadPart::Update()
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

void HeadPart::Draw()
{
    if (hp_ <= 0) return;
    if (model_)
    {
        model_->Draw();
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
