#include "TitleScene.h"
#include <cmath>
#ifndef M_PI
#define M_PI 3.1415927f
#endif

TitleScene::~TitleScene()
{
	delete fade_;
}

void TitleScene::Initialize(Camera* camera, Object3dCom* object3dCom, SpriteCom* spriteCom)
{
	fade_ = new Fade();
	fade_->Initialize(spriteCom);

	fade_->Start(Fade::State::kFadeIn, 1.0f);

	camera_ = camera;

	model_ = Object3d::Create(object3dCom, "title/title.obj", { {1.0f,1.0f,1.0f},{-M_PI/2.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} }, camera);
	

	keyInput_ = KeyInput::GetInstance();
}

void TitleScene::Update()
{
	switch (phase_) {
	case Phase::kFadeIn:
		model_->Update();
		fade_->Update();

		// フェードが終わったらメインフェーズへ
		if (fade_->IsFinished()) {
			phase_ = Phase::kMain;
		}
		break;

	case Phase::kMain:
		model_->Update();
		// スペースキーが押されたらフェードアウトへ
		if (keyInput_->PushKey(DIK_SPACE)) {
			phase_ = Phase::kFadeOut;
			fade_->Start(Fade::State::kFadeOut, 1.0f);

		}
		break;

	case Phase::kFadeOut:
		model_->Update();
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

	model_->Draw();
}
