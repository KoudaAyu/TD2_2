#include "GameOverScene.h"
#include "SpriteCom.h"
#include "Sprite.h"
#include "KeyInput.h"
#include "Object3dCom.h"

#include "Logger.h"

GameOverScene::~GameOverScene()
{
    if (overlay_) { delete overlay_; overlay_ = nullptr; }
    if (fade_) { delete fade_; fade_ = nullptr; }
}

void GameOverScene::Initialize(Camera* camera, Object3dCom* object3dCom, SpriteCom* spriteCom)
{
    camera_ = camera;
    object3dCom_ = object3dCom;
    spriteCom_ = spriteCom;

    fade_ = new Fade();
    fade_->Initialize(spriteCom_);
    fade_->Start(Fade::State::kFadeIn, 0.6f);

    // create a simple overlay sprite saying "Game Over" if texture exists, else use blank black overlay
    if (spriteCom_)
    {
        int sw = object3dCom_ ? object3dCom_->GetDirectXCom()->GetClientWidth() : 1280;
        int sh = object3dCom_ ? object3dCom_->GetDirectXCom()->GetClientHeight() : 720;
        overlay_ = spriteCom_->CreateSprite("Resources/white.png", { 0.0f, 0.0f }, { static_cast<float>(sw), static_cast<float>(sh) }, 0.0f, { 0.0f, 0.0f }, false, false);
        if (overlay_)
        {
            Vector4 c = overlay_->GetColor();
            c.w = 0.0f;
            overlay_->SetColor(c);
            overlay_->Update();
        }
    }

}

void GameOverScene::Update()
{
    if (fade_) fade_->Update();

    KeyInput* ki = KeyInput::GetInstance();

    switch (phase_)
    {
    case Phase::kFadeIn:
        if (fade_ && fade_->IsFinished())
        {
            phase_ = Phase::kMain;
            // show overlay alpha
            if (overlay_)
            {
                Vector4 c = overlay_->GetColor();
                c.w = 1.0f;
                overlay_->SetColor(c);
                overlay_->Update();
            }
        }
        break;

    case Phase::kMain:
        // Press space or A button to go back to title (mark finish)
        if (ki && (ki->TriggerKey(DIK_SPACE) || ki->IsPadButtonPressed(XINPUT_GAMEPAD_A)))
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

void GameOverScene::Draw()
{
    if (overlay_) overlay_->Draw();
    if (fade_) fade_->Draw();
}
