#include "TitleScene.h"

#include<algorithm>

TitleScene::~TitleScene()
{
	if (titleTextModel_) {
		delete titleTextModel_;
		titleTextModel_ = nullptr;
	}
	object3dCom_ = nullptr;
	spriteCom_ = nullptr;
	delete fade_;
}

void TitleScene::Initialize(Camera* camera, Object3dCom* object3dCom, SpriteCom* spriteCom)
{
	camera_ = camera;
	// カメラの初期化（アスペクト比設定）
	camera_->Initialize();

	fade_ = new Fade();
	fade_->Initialize(spriteCom);

	fade_->Start(Fade::State::kFadeIn, 1.0f);

	keyInput_ = KeyInput::GetInstance();

	// object3dCom を注入して保持
	object3dCom_ = object3dCom;
	assert(object3dCom && "object3dCom is null. Call Fade::Initialize with a valid object3dCom*.");

	titleTextModel_ = Object3d::Create(object3dCom_, "titleTextNo.obj", { {1,1,1},{0,0,0},{0,0,0} }, camera_);
	titleTextModel_->SetTranslate({ 0.0f, 0.0f, 10.0f });

	// SpriteCom を注入して保持
	spriteCom_ = spriteCom;
	assert(spriteCom_ && "SpriteCom is null. Call Fade::Initialize with a valid SpriteCom*.");

	backGroundSprite_ = spriteCom_->CreateSprite("Resources/backgroundTitle.png",
		{ 0.0f,0.0f },
		{ 1280.0f,720.0f },
		0.0f,
		{ 0.0f,0.0f },
		false,
		false);

	spaceOrASprite_ = spriteCom_->CreateSprite("Resources/SpaceOrA.png",
		{ 640.0f,550.0f },
		{ 640.0f,128.0f },
		0.0f,
		{ 0.5f,0.5f },
		false,
		false);
}

void TitleScene::Update()
{
	// 時間経過
	elapsedTime_ += kDeltaTime;
	// 終了判定
	if (elapsedTime_ >= 60.0f) {
		elapsedTime_ = 0.0f;
	}

	titleTextModel_->Update();

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
		if (keyInput_->PushKey(DIK_SPACE)) {
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

	if (phase_ != Phase::kFadeOut) {
		SpaceOrAPingPong();
	} else {
		PushKeyReaction();
	}
}

void TitleScene::Draw()
{
	if (backGroundSprite_) {
		backGroundSprite_->Draw();
	}

	if (spaceOrASprite_) {
		spaceOrASprite_->Draw();
	}

	if (titleTextModel_) {
		titleTextModel_->Draw();
	}

	fade_->Draw();
}

float TitleScene::CalculatePingPongTime(float elapsedTime, float duration) {
	if (duration <= 0.0f) {
		return 0.0f;
	}
	// 正規化して往復サイクルでの位置を得る
	float normalizedTime = elapsedTime / duration;
	// 整数部分を取り出し、現在が何往復目のサイクルかを判断する
	float cycle = std::floor(normalizedTime);
	// 現在のサイクル内での位置を取り出す(0.0~1.0)
	float cyclePosition = normalizedTime - cycle;

	// cycleが偶数か奇数かで、増減の方向を反転
	if (static_cast<long long>(cycle) % 2 == 0) {
		// 偶数
		return cyclePosition;
	} else {
		// 奇数
		return 1.0f - cyclePosition;
	}
}

void TitleScene::SpaceOrAPingPong() {
	float t = CalculatePingPongTime(elapsedTime_, duration_);
	spaceOrASprite_->SetColor(Vector4(0, 0, 0, 1.0f - t));
}

void TitleScene::PushKeyReaction() {
	elapsedReactionTime_ += kDeltaTime;

	const float duration = 0.2f;
	float pingPongTime = CalculatePingPongTime(elapsedReactionTime_, duration);

	const float minAlpha = 0.2f;
	const float maxAlpha = 1.0f;

	float alpha = minAlpha + (maxAlpha - minAlpha) * static_cast<float>(pingPongTime);

	spaceOrASprite_->SetColor(Vector4(0, 0, 0, alpha));
}
