#include "SelectScene.h"
#include "SpriteCom.h"
#include "KeyInput.h"

SelectScene::~SelectScene()
{
    delete fade_;
}

void SelectScene::Initialize(SpriteCom* spriteCom)
{
    fade_ = new Fade();
    fade_->Initialize(spriteCom);
    fade_->Start(Fade::State::kFadeIn, 0.5f);

    keyInput_ = KeyInput::GetInstance();
}

void SelectScene::Update()
{
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
        }
        // 右矢印を押すとゲーム（右をGameに変更）
        if (keyInput_->TriggerKey(DIK_RIGHT))
        {
            choice_ = Choice::kGame;
        }

        // 決定
        if (keyInput_->TriggerKey(DIK_SPACE))
        {
            phase_ = Phase::kFadeOut;
            fade_->Start(Fade::State::kFadeOut, 0.5f);
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
  
    fade_->Draw();
}
