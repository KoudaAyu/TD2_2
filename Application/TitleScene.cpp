#include "TitleScene.h"

TitleScene::~TitleScene()
{
	delete fade_;
}

void TitleScene::Initialize(SpriteCom* spriteCom)
{
	fade_ = new Fade();
	fade_->Initialize(spriteCom);

	fade_->Start(Fade::State::kFadeIn, 1.0f);

	keyInput_ = KeyInput::GetInstance();
}

void TitleScene::Update()
{
	switch (phase_) {
	case Phase::kFadeIn:
		fade_->Update();

		// フェードが終わったらメインフェーズへ
		if (fade_->IsFinished()) {
			phase_ = Phase::kMain;
		}
		break;

	case Phase::kMain:
		// スペースキーが押されたらフェードアウトへ
		if (keyInput_->IsKeyPressed(DIK_SPACE)) {
			phase_ = Phase::kFadeOut;
			fade_->Start(Fade::State::kFadeOut, 1.0f);

		}
		break;

	case Phase::kFadeOut:
		fade_->Update();

		// フェードアウト完了したらシーン終了
		if (fade_->IsFinished()) {
			isFinish_ = true;
		}
		break;
	}
}

void TitleScene::Draw()
{
	fade_->Draw();
}
