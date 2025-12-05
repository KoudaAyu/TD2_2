#include "SelectScene.h"
#include "SpriteCom.h"
#include "KeyInput.h"
#include "Object3d.h"
#include "Object3dCom.h"
#include "Camera.h"
#include <cmath>
#include "ModelManager.h"
#include "ParticleManager.h"

SelectScene::~SelectScene()
{
    if (fade_) { delete fade_; fade_ = nullptr; }
    if (objectModel_) { delete objectModel_; objectModel_ = nullptr; }
    if (bombModel_) { delete bombModel_; bombModel_ = nullptr; }
}


void SelectScene::Initialize(SpriteCom* spriteCom, Object3dCom* object3dCom, Camera* camera)
{
    object3dCom_ = object3dCom;
    camera_ = camera;

    fade_ = new Fade();
    fade_->Initialize(spriteCom);
    fade_->Start(Fade::State::kFadeIn, 0.5f);

    keyInput_ = KeyInput::GetInstance();

    // Light preloads to reduce hitch on first create (textures/models used in UI)
    ModelManager::GetInstance()->LoadModel("apple.obj");
    ModelManager::GetInstance()->LoadModel("bomb.obj");
   
    objectModel_ = Object3d::Create(object3dCom_, "apple.obj",
        { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} }, camera_);

    bombModel_ = Object3d::Create(object3dCom_, "bomb.obj",
        { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} }, camera_);

  
    if (objectModel_) objectModel_->SetTranslate({ 0.0f, 0.0f, 0.0f });
    if (bombModel_) bombModel_->SetTranslate({ 3.0f, 0.0f, 0.0f });
    choice_ = Choice::kTutorial;
}


static void ApplySelectionPosition(Object3d* selected, Object3d* other)
{
    if (selected) selected->SetTranslate({ 0.0f, 0.0f, 0.0f });
    if (other) other->SetTranslate({ 3.0f, 0.0f, 0.0f });
}

void SelectScene::Update()
{
    if (objectModel_) objectModel_->Update();
    if (bombModel_) bombModel_->Update();

    switch (phase_)
    {
    case Phase::kFadeIn:
        fade_->Update();
        if (fade_->IsFinished()) phase_ = Phase::kMain;
        break;

    case Phase::kMain:
        // 左右で選択、スペースで決定
        // 左矢印を押すとチュートリアル（左をTutorialに変更）
        if (keyInput_->TriggerKey(DIK_LEFT))
        {
            choice_ = Choice::kTutorial;
            ApplySelectionPosition(objectModel_, bombModel_);
        }
        // 右矢印を押すとゲーム（右をGameに変更）
        if (keyInput_->TriggerKey(DIK_RIGHT))
        {
            choice_ = Choice::kGame;

            if (bombModel_) bombModel_->SetTranslate({ 0.0f, 0.0f, 0.0f });
            if (objectModel_) objectModel_->SetTranslate({ -3.0f, 0.0f, 0.0f });

            // Start preloading heavy GameScene assets now so actual transition (SPACE) isn't blocked
            ModelManager::GetInstance()->LoadModel("player/player.obj");
            ModelManager::GetInstance()->LoadModel("wall.obj");
            ModelManager::GetInstance()->LoadModel("bomb.obj");

            auto* pm = ParticleManager::GetInstance();
            if (pm) {
                // create particle groups early (idempotent)
                pm->CreateParticleGroupFromModel("default", "apple.obj");
                pm->CreateParticleGroupFromModel("defaultMesh", "apple.obj");
                if (!pm->HasGroup("enemyMesh")) pm->CreateParticleGroupFromModel("enemyMesh", "wall.obj");
            }
        }

        // 決定
        if (keyInput_->TriggerKey(DIK_SPACE))
        {
            phase_ = Phase::kFadeOut;
            fade_->Start(Fade::State::kFadeOut, 0.5f);
            // NOTE: avoid heavy synchronous loads here to prevent blocking the frame; preloads occur on selection.
        }
        break;

    case Phase::kFadeOut:
        fade_->Update();
        if (fade_->IsFinished())
        {
            isFinish_ = true;
        }
        break;
    }
}

void SelectScene::Draw()
{
    if (objectModel_) objectModel_->Draw();
    if (bombModel_) bombModel_->Draw();

    fade_->Draw();
}
