#include "GameOverScene.h"
#include "SpriteCom.h"
#include "Sprite.h"
#include "KeyInput.h"
#include "Camera.h"
#include "Object3dCom.h"
#include "Fade.h"
#include "Object3d.h"

#include "Logger.h"

GameOverScene::~GameOverScene()
{
    if (background_) { delete background_; background_ = nullptr; }
    if (overlay_) { delete overlay_; overlay_ = nullptr; }
    if (fade_) { delete fade_; fade_ = nullptr; }
    if (gameOverModel_) { delete gameOverModel_; gameOverModel_ = nullptr; }
    if (spaceModel_) { delete spaceModel_; spaceModel_ = nullptr; }
}

void GameOverScene::Initialize(Camera* camera, Object3dCom* object3dCom, SpriteCom* spriteCom)
{
    camera_ = camera;
    object3dCom_ = object3dCom;
    spriteCom_ = spriteCom;

    // Ensure a consistent camera setup for GameOver (same as other UI scenes)
    if (camera_) {
        camera_->Initialize();
        camera_->SetAspectRatio(static_cast<float>(object3dCom_->GetDirectXCom()->GetClientWidth()) /
                                 static_cast<float>(object3dCom_->GetDirectXCom()->GetClientHeight()));
        camera_->SetFovY(0.8f);
        camera_->SetTranslate({0.0f, 0.0f, -5.0f});
        camera_->SetRotate({0.0f, 0.0f, 0.0f});
        camera_->Update();
    }

    // setup fade
    fade_ = new Fade();
    fade_->Initialize(spriteCom_);
    fade_->Start(Fade::State::kFadeIn, 0.8f);

    // background: BackScreen.png fullscreen
    if (spriteCom_)
    {
        int sw = object3dCom_ ? object3dCom_->GetDirectXCom()->GetClientWidth() : 1280;
        int sh = object3dCom_ ? object3dCom_->GetDirectXCom()->GetClientHeight() : 720;
        background_ = spriteCom_->CreateSprite("Resources/BackScreen.png", { 0.0f, 0.0f }, { static_cast<float>(sw), static_cast<float>(sh) }, 0.0f, { 0.0f, 0.0f }, false, false);
        if (background_) {
            background_->SetColor({1.0f,1.0f,1.0f,1.0f});
            background_->Update();
        }

        // overlay starts transparent (can be used for tint)
        overlay_ = spriteCom_->CreateSprite("Resources/white.png", { 0.0f, 0.0f }, { static_cast<float>(sw), static_cast<float>(sh) }, 0.0f, { 0.0f, 0.0f }, false, false);
        if (overlay_) {
            Vector4 c = overlay_->GetColor();
            c.w = 0.0f;
            overlay_->SetColor(c);
            overlay_->Update();
        }
    }

    // add GameOver model and SPACE.obj, static
    if (object3dCom_ && camera_)
    {
        const float kPi = 3.1415927f;
        // GameOver/GameOver.obj centered
        gameOverModel_ = Object3d::Create(object3dCom_, "GameOver/GameOver.obj", { {1.0f,1.0f,1.0f},{-kPi * 0.5f, 0.0f, 0.0f},{0.0f, 0.0f, 0.0f} }, camera_);
        if (gameOverModel_) {
            gameOverModel_->SetTranslate({0.0f, 0.0f, 0.0f});
        }
        // SPACE below
        spaceModel_ = Object3d::Create(object3dCom_, "SPACE.obj", { {1.0f,1.0f,1.0f},{-kPi * 0.5f, 0.0f, 0.0f},{0.0f, -1.0f, 0.0f} }, camera_);
        if (spaceModel_) {
            spaceModel_->SetScale({0.5f, 0.5f, 0.5f});
            spaceModel_->SetTranslate({0.0f, -1.0f, 0.0f});
        }
    }

    keyInput_ = KeyInput::GetInstance();
    animTime_ = 0.0f;
}

void GameOverScene::Update()
{
    if (fade_) fade_->Update();

    // subtle game-over style motion: slow sink and slight squash/stretch pulse
    animTime_ += 1.0f / 60.0f;
    if (gameOverModel_) {
        Vector3 t = gameOverModel_->GetTranslate();
        // slow downward sink (caps
        t.y = -0.1f + 0.05f * std::sin(animTime_ * 1.6f);
        gameOverModel_->SetTranslate(t);
        Vector3 s = gameOverModel_->GetScale();
        float pulse = 0.02f * std::sin(animTime_ * 3.2f);
        s.x = 1.0f + pulse;
        s.y = 1.0f - pulse;
        s.z = 1.0f;
        gameOverModel_->SetScale(s);
        gameOverModel_->Update();
    }

    if (spaceModel_) spaceModel_->Update();

    switch (phase_)
    {
    case Phase::kFadeIn:
        if (fade_ && fade_->IsFinished())
        {
            phase_ = Phase::kMain;
            // optional tint via overlay
            if (overlay_)
            {
                Vector4 c = overlay_->GetColor();
                c.w = 0.3f; // slight darkening
                overlay_->SetColor(c);
                overlay_->Update();
            }
        }
        break;

    case Phase::kMain:
        if (keyInput_ && (keyInput_->TriggerKey(DIK_RETURN) || keyInput_->TriggerKey(DIK_SPACE) || keyInput_->IsPadButtonPressed(XINPUT_GAMEPAD_A)))
        {
            phase_ = Phase::kFadeOut;
            if (fade_) fade_->Start(Fade::State::kFadeOut, 0.7f);
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
    if (background_) background_->Draw();
    if (overlay_) overlay_->Draw();
    if (gameOverModel_) gameOverModel_->Draw();
    if (spaceModel_) spaceModel_->Draw();
    if (fade_) fade_->Draw();
}
