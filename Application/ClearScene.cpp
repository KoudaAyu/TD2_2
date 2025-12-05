#include "ClearScene.h"
#include "SpriteCom.h"
#include "Sprite.h"
#include "Camera.h"
#include "Object3dCom.h"

ClearScene::~ClearScene()
{
    if (clearSprite_) { delete clearSprite_; clearSprite_ = nullptr; }
}

void ClearScene::Initialize(Camera* camera, Object3dCom* object3dCom, SpriteCom* spriteCom)
{
    camera_ = camera;
    object3dCom_ = object3dCom;
    spriteCom_ = spriteCom;

    if (!spriteCom_) return;

    // NOTE: Resources/UI/CLEAR.png is not yet provided. Temporarily disable sprite creation.
    /*
    int sw = object3dCom_->GetDirectXCom()->GetClientWidth();
    int sh = object3dCom_->GetDirectXCom()->GetClientHeight();

    clearSprite_ = spriteCom_->CreateSprite("Resources/UI/CLEAR.png", { static_cast<float>(sw)*0.5f, static_cast<float>(sh)*0.5f }, { 800.0f, 200.0f }, 0.0f, {0.5f,0.5f});
    if (clearSprite_)
    {
        Vector4 c = clearSprite_->GetColor();
        c.w = 1.0f;
        clearSprite_->SetColor(c);
        clearSprite_->Update();
    }
    */
}

void ClearScene::Update()
{
    // Allow user to press a key to finish the clear scene
    // (for testing, pressing Enter will mark finish)
    // We don't include KeyInput dependency; simple polling via Win32 or other input systems
    // For now, just keep the scene active until external check.
}

void ClearScene::Draw()
{
    // Draw disabled until Resources/UI/CLEAR.png is added.
    // if (clearSprite_) clearSprite_->Draw();
}
