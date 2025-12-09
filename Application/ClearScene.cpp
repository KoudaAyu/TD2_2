#include "ClearScene.h"
#include "SpriteCom.h"
#include "Sprite.h"
#include "Camera.h"
#include "Object3dCom.h"
#include "KeyInput.h"
#include "Fade.h"

ClearScene::~ClearScene()
{
    if (clearSprite_) { delete clearSprite_; clearSprite_ = nullptr; }
    if (fade_) { delete fade_; fade_ = nullptr; }
}

void ClearScene::Initialize(Camera* camera, Object3dCom* object3dCom, SpriteCom* spriteCom)
{
    camera_ = camera;
    object3dCom_ = object3dCom;
    spriteCom_ = spriteCom;

    // setup fade
    fade_ = new Fade();
    fade_->Initialize(spriteCom_);
    fade_->Start(Fade::State::kFadeIn, 0.6f);

    if (spriteCom_)
    {
        int sw = object3dCom_ ? object3dCom_->GetDirectXCom()->GetClientWidth() : 1280;
        int sh = object3dCom_ ? object3dCom_->GetDirectXCom()->GetClientHeight() : 720;
        clearSprite_ = spriteCom_->CreateSprite("Resources/white.png", { 0.0f, 0.0f }, { static_cast<float>(sw), static_cast<float>(sh) }, 0.0f, { 0.0f, 0.0f }, false, false);
        if (clearSprite_)
        {
            Vector4 c = clearSprite_->GetColor();
            c.w = 0.0f; // start transparent until fade completes
            clearSprite_->SetColor(c);
            clearSprite_->Update();
        }
    }
}

void ClearScene::Update()
{
    if (fade_) fade_->Update();

    KeyInput* ki = KeyInput::GetInstance();

    switch (phase_)
    {
    case Phase::kFadeIn:
        if (fade_ && fade_->IsFinished())
        {
            phase_ = Phase::kMain;
            if (clearSprite_)
            {
                Vector4 c = clearSprite_->GetColor();
                c.w = 1.0f;
                clearSprite_->SetColor(c);
                clearSprite_->Update();
            }
        }
        break;

    case Phase::kMain:
        if (ki && (ki->TriggerKey(DIK_RETURN) || ki->TriggerKey(DIK_SPACE) || ki->IsPadButtonPressed(XINPUT_GAMEPAD_A)))
        {
            phase_ = Phase::kFadeOut;
            if (fade_) fade_->Start(Fade::State::kFadeOut, 0.6f);
        }
        break;

    case Phase::kFadeOut:
        if (fade_ && fade_->IsFinished())
        {
            isFinish_ = true;
        }
        break;
    }
}

void ClearScene::Draw()
{
    /*if (clearSprite_) clearSprite_->Draw();*/
    if (fade_) fade_->Draw();
}
