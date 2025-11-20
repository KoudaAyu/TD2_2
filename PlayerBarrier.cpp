#include "PlayerBarrier.h"
#include <cassert>

PlayerBarrier::~PlayerBarrier()
{
    if (barrierModel_)
    {
        delete barrierModel_;
        barrierModel_ = nullptr;
    }
}

void PlayerBarrier::Initialize(Object3d* model, const Vector3 pos, Object3dCom* object3dCom)
{
#ifdef _DEBUG
    assert(model != nullptr);
    assert(object3dCom != nullptr);
#endif //  _DEBUG

    model_ = model; 
    object3dCom_ = object3dCom;

    worldTransform.Initialize();
    worldTransform.SetTranslate(pos);

    
    barrierModel_ = new Object3d();
    barrierModel_->Initialize(object3dCom_);

    // If the source has a Model resource, create a copy so the barrier can have its own color/material
    Model* srcModel = model_->GetModel();
    if (srcModel)
    {
        // Copy the Model resource so player/model remain unaffected
        Model* barrierModelResource = new Model(*srcModel);

        // Set distinct color on the copied Model
        Vector4 barrierColor = { 0.0f, 0.7f, 1.0f, 1.0f }; // cyan-ish
        barrierModelResource->SetColor(barrierColor);

        // Assign the copied Model to the barrier's Object3d
        barrierModel_->SetModel(barrierModelResource);

        // Also set Object3d's color in case Object3d's own color overrides Model color
        barrierModel_->SetColor(barrierColor);
    }

}

void PlayerBarrier::Update()
{
    worldTransform.TransferMatrix();

    if (barrierModel_)
    {
        barrierModel_->ApplyState(worldTransform, nullptr, false);
        barrierModel_->Update();
    }
}

void PlayerBarrier::Draw(const Camera& camera)
{
    if (barrierModel_)
    {
        // Apply camera before drawing (immediateUpdate true to ensure matrices are set)
        barrierModel_->ApplyState(worldTransform, const_cast<Camera*>(&camera), true);
        barrierModel_->Draw();
    }
}
