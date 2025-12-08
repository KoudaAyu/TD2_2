#include "SelectScene.h"
#include "SpriteCom.h"
#include "KeyInput.h"
#include "Object3d.h"
#include "Object3dCom.h"
#include "Camera.h"
#include <cmath>
#include "ModelManager.h"
#include "ParticleManager.h"
#include "Sprite.h"

SelectScene::~SelectScene()
{
    if (fade_) { delete fade_; fade_ = nullptr; }
    if (objectModel_) { delete objectModel_; objectModel_ = nullptr; }
    if (bombModel_) { delete bombModel_; bombModel_ = nullptr; }
    if (selectSprite_) { delete selectSprite_; selectSprite_ = nullptr; }
}


void SelectScene::Initialize(SpriteCom* spriteCom, Object3dCom* object3dCom, Camera* camera)
{
    object3dCom_ = object3dCom;
    camera_ = camera;
    spriteCom_ = spriteCom;

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
    // always go to game after loading
    choice_ = Choice::kGame;

    // Create a single fullscreen white sprite if spriteCom available
    if (spriteCom_)
    {
        int sw = 1280;
        int sh = 720;
        if (object3dCom_ && object3dCom_->GetDirectXCom()) {
            sw = object3dCom_->GetDirectXCom()->GetClientWidth();
            sh = object3dCom_->GetDirectXCom()->GetClientHeight();
        }

        // create a sprite that covers the whole screen
        selectSprite_ = spriteCom_->CreateSprite("Resources/white.png", { 0.0f, 0.0f }, { static_cast<float>(sw), static_cast<float>(sh) }, 0.0f, { 0.0f, 0.0f }, false, false);
        if (selectSprite_) { selectSprite_->SetColor({1.0f,1.0f,1.0f,1.0f}); selectSprite_->Update(); }
    }
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
        // remove input selection; auto preload and start fade out
        if (!preloaded_) {
            preloaded_ = true;
            ModelManager::GetInstance()->LoadModel("player/player.obj");
            ModelManager::GetInstance()->LoadModel("wall.obj");
            ModelManager::GetInstance()->LoadModel("bomb.obj");
            auto* pm = ParticleManager::GetInstance();
            if (pm) {
                pm->CreateParticleGroupFromModel("default", "apple.obj");
                pm->CreateParticleGroupFromModel("defaultMesh", "apple.obj");
                if (!pm->HasGroup("enemyMesh")) pm->CreateParticleGroupFromModel("enemyMesh", "wall.obj");
            }
        }
        phase_ = Phase::kFadeOut;
        fade_->Start(Fade::State::kFadeOut, 0.5f);
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
    // Only draw the single fullscreen sprite when available
    if (selectSprite_) selectSprite_->Draw();

    // Draw fade overlay on top
    if (fade_) fade_->Draw();
}
