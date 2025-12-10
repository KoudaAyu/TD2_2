#include "ClearScene.h"
#include "SpriteCom.h"
#include "Sprite.h"
#include "Camera.h"
#include "Object3dCom.h"
#include "KeyInput.h"
#include "Fade.h"
#include "Object3d.h"

ClearScene::~ClearScene()
{
    if (background_) { delete background_; background_ = nullptr; }
    if (clearSprite_) { delete clearSprite_; clearSprite_ = nullptr; }
    if (fade_) { delete fade_; fade_ = nullptr; }
    if (clearModel_) { delete clearModel_; clearModel_ = nullptr; }
    if (spaceModel_) { delete spaceModel_; spaceModel_ = nullptr; }
}

void ClearScene::Initialize(Camera* camera, Object3dCom* object3dCom, SpriteCom* spriteCom)
{
    camera_ = camera;
    object3dCom_ = object3dCom;
    spriteCom_ = spriteCom;

    // Ensure a consistent camera setup for ClearScene (same as GameOverScene)
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
    fade_->Start(Fade::State::kFadeIn, 0.6f);

    // background
    if (spriteCom_)
    {
        int sw = object3dCom_ ? object3dCom_->GetDirectXCom()->GetClientWidth() : 1280;
        int sh = object3dCom_ ? object3dCom_->GetDirectXCom()->GetClientHeight() : 720;
        background_ = spriteCom_->CreateSprite("Resources/BackScreen.png", { 0.0f, 0.0f }, { static_cast<float>(sw), static_cast<float>(sh) }, 0.0f, { 0.0f, 0.0f }, false, false);
        if (background_) {
            background_->SetColor({1.0f,1.0f,1.0f,1.0f});
            background_->Update();
        }
    }

    // create Clear/SPACE models
    if (object3dCom_ && camera_)
    {
        const float kPi = 3.1415927f; // define PI once for both models

        clearModel_ = Object3d::Create(object3dCom_, "Clear/Clear.obj", { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} }, camera_);
        if (clearModel_) {
            // face toward camera: rotate -90 degrees around X
            clearModel_->SetRotate({-kPi * 0.5f, 0.0f, 0.0f});
            // place slightly forward to be visible
            clearModel_->SetTranslate({0.0f, 0.0f, 0.0f});
        }

        // create SPACE model (Clear/SPACE.obj) below Clear.obj
        spaceModel_ = Object3d::Create(object3dCom_, "SPACE.obj", { {1.0f,1.0f,1.0f},{-kPi * 0.5f, 0.0f, 0.0f},{0.0f, -1.0f, 0.0f} }, camera_);
        if (spaceModel_) {
            spaceModel_->SetScale({0.5f, 0.5f, 0.5f});
            spaceModel_->SetTranslate({0.0f, -1.0f, 0.0f});
        }
    }

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

    animTime_ = 0.0f;
}

void ClearScene::Update()
{
    if (fade_) fade_->Update();
    if (clearModel_) {
        // simple idle motion: gentle bobbing only (no rotation)
        animTime_ += 1.0f / 60.0f;
        Vector3 t = clearModel_->GetTranslate();
        t.y = 0.2f * std::sin(animTime_ * 2.0f); // up/down
        clearModel_->SetTranslate(t);
        clearModel_->Update();
    }

    // Do not animate SPACE.obj; keep its transform constant
    if (spaceModel_) {
        spaceModel_->Update();
    }

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
    if (background_) background_->Draw();
    // draw Clear.obj and SPACE.obj (SPACE below Clear)
    if (clearModel_) clearModel_->Draw();
    if (spaceModel_) spaceModel_->Draw();

    /*if (clearSprite_) clearSprite_->Draw();*/
    if (fade_) fade_->Draw();
}
