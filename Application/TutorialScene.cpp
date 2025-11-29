#include "TutorialScene.h"

TutorialScene::~TutorialScene()
{
    if (fade_) { delete fade_; fade_ = nullptr; }
}

void TutorialScene::Initialize(Camera* /*camera*/, Object3dCom* /*object3dCom*/, SpriteCom* spriteCom)
{
    // フェードの初期化（白黒スプライトを作成）
    fade_ = new Fade();
    fade_->Initialize(spriteCom);

    // 開始時はフェードインから始める
    fade_->Start(Fade::State::kFadeIn, 0.5f);

    phase_ = Phase::kFadeIn;
}

void TutorialScene::Update()
{
    switch (phase_)
    {
    case Phase::kFadeIn:
        if (fade_)
        {
            fade_->Update();
            if (fade_->IsFinished()) phase_ = Phase::kMain;
        }
        else
        {
            phase_ = Phase::kMain;
        }
        break;

    case Phase::kMain:
   
        break;

    case Phase::kFadeOut:
        if (fade_)
        {
            fade_->Update();
            if (fade_->IsFinished()) isFinish_ = true;
        }
        else
        {
            isFinish_ = true;
        }
        break;
    }
}

void TutorialScene::Draw()
{
    if (fade_) fade_->Draw();
}
