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

    worldTransform.Initialize();
    worldTransform.SetTranslate(pos);

    
    barrierModel_ = new Object3d();
    barrierModel_->Initialize(object3dCom_);

    
    if (Model* apple = ModelManager::GetInstance()->LoadAndGetModel("apple.obj")) {
        const Vector4 barrierColor{0.0f, 0.7f, 1.0f, 1.0f};
        barrierModel_->SetModel(new Model(*apple));          // コピーを作成して設定
        barrierModel_->GetModel()->SetColor(barrierColor); // モデル内部の色
        barrierModel_->SetColor(barrierColor);             // Object3d の色
    }

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
