#include "TutorialScene.h"

TutorialScene::~TutorialScene()
{
    if (gameScene_) delete gameScene_;
    if (fade_) delete fade_;
}

void TutorialScene::Initialize(Camera* camera, Object3dCom* object3dCom, SpriteCom* spriteCom)
{
    gameScene_ = new GameScene();
    gameScene_->Initialize(camera, object3dCom, spriteCom);

    fade_ = new Fade();
    fade_->Initialize(spriteCom);
    fade_->Start(Fade::State::kNone, 0.0f);
}

void TutorialScene::Update()
{
    if (gameScene_) gameScene_->Update();

     if (gameScene_ && gameScene_->IsFinish())
    {
        isFinish_ = true;
    }
}

void TutorialScene::Draw()
{
    if (gameScene_) gameScene_->Draw();

    if (fade_) fade_->Draw();
}
