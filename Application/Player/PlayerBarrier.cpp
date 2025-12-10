#include "PlayerBarrier.h"
#include <cassert>
#include "ModelManager.h"

PlayerBarrier::~PlayerBarrier()
{
    if (barrierModel_)
    {
        delete barrierModel_;
        barrierModel_ = nullptr;
    }
}

void PlayerBarrier::Initialize(Object3d* model, const Vector3 pos, Object3dCom* object3dCom, const Vector3& velocity)
{
#ifdef _DEBUG
    assert(model != nullptr);
    assert(object3dCom != nullptr);
#endif //  _DEBUG

    model_ = model; 
    object3dCom_ = object3dCom;
	velocity_ = velocity;

    // 縦方向ショットなら弾速アップ（DPSメリットの一部：速度のみ）
    {
        const float kVerticalSpeedBoost = 1.35f; // 調整可
        if (std::abs(velocity_.y) > std::abs(velocity_.x) * 0.75f) {
            velocity_.x *= kVerticalSpeedBoost;
            velocity_.y *= kVerticalSpeedBoost;
            velocity_.z *= kVerticalSpeedBoost;
        }
    }

    worldTransform.Initialize();
    worldTransform.SetTranslate(pos);

    barrierModel_ = new Object3d();
    barrierModel_->Initialize(object3dCom_);

    // Barrier を apple.obj ベースにして、見やすいパープルを適用（少し暗め）
    if (Model* apple = ModelManager::GetInstance()->LoadAndGetModel("apple.obj")) {
        const Vector4 barrierColor{0.72f, 0.40f, 0.85f, 1.0f};
        barrierModel_->SetModel(new Model(*apple));
        if (barrierModel_->GetModel()) barrierModel_->GetModel()->SetColor(barrierColor);
        barrierModel_->SetColor(barrierColor);
        barrierModel_->SetScale({0.26f, 0.26f, 0.26f});
    }

	birthWave_ = 0; // initialize member
}

void PlayerBarrier::Update()
{

    if (!isActive_)
    {
        return;
    }

    if (--deathTimer_ <= 0)
    {
        isActive_ = false;
		return;
    }

	worldTransform.SetTranslate(worldTransform.GetTranslate() + velocity_);
    worldTransform.TransferMatrix();

    if (barrierModel_)
    {
        barrierModel_->ApplyState(worldTransform, nullptr, false);
        barrierModel_->Update();
    }
}

void PlayerBarrier::Draw(const Camera& camera)
{
    if (!isActive_)
    {
        return;
    }

    if (barrierModel_)
    {
        barrierModel_->ApplyState(worldTransform, const_cast<Camera*>(&camera), true);
        barrierModel_->Draw();
    }
}

void PlayerBarrier::OnCollision()
{
    isActive_ = false;
}
