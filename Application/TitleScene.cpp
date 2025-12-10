#include "TitleScene.h"
#include <cmath>
#ifndef M_PI
#define M_PI 3.1415927f
#endif
#include "ParticleManager.h"
#include "Random.h"

TitleScene::~TitleScene()
{
	// stop and unload BGM if started
	if (soundManager_ && hasBgm_) {
		soundManager_->SoundStopBGM();
		soundManager_->SoundUnload(&bgmData_);
		hasBgm_ = false;
	}
	delete fade_;
}

void TitleScene::Initialize(Camera* camera, Object3dCom* object3dCom, SpriteCom* spriteCom)
{
	fade_ = new Fade();
	fade_->Initialize(spriteCom);

	fade_->Start(Fade::State::kFadeIn, 1.0f);

	camera_ = camera;

	// Ensure camera is reset to a known default when entering the Title scene.
	if (camera_) {
		camera_->SetTranslate({ 0.0f, 0.0f, -5.0f });
		camera_->SetRotate({ 0.0f, 0.0f, 0.0f });
		camera_->Initialize();
		camera_->Update();
	}

	model_ = Object3d::Create(object3dCom, "title/title.obj", { {1.0f,1.0f,1.0f},{-M_PI/2.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} }, camera);
	

	keyInput_ = KeyInput::GetInstance();

	motionTime_ = 0.0f; // モデル動作用タイマー初期化

	// 事前にモデルをロード
	ModelManager::GetInstance()->LoadModel("apple.obj");
	// OBJモデルのパーティクルグループ作成
	ParticleManager::GetInstance()->CreateParticleGroupFromModel("title_effect_obj", "apple.obj");

	// Start Title BGM (loop)
	soundManager_ = SoundManager::GetInstance();
	if (soundManager_) {
		bgmData_ = soundManager_->SoundLoadWave("Resources/Audio/BGM/Title.wav");
		soundManager_->SoundPlayBGM(bgmData_, true, 0.5f);
		hasBgm_ = true;
	}
}

void TitleScene::Update()
{
	motionTime_ += 0.016f; // 1フレーム分進める（60FPS想定）

	// 微細な揺れ（控えめ）
	float shakeX = std::sin(motionTime_ * 2.0f) * 0.02f;
	float shakeY = std::sin(motionTime_ * 2.5f) * 0.02f;
	model_->SetRotate({-M_PI/2.0f, 0.0f, 0.0f});
	model_->SetTranslate({shakeX, shakeY, 0.0f});

	// 画面全体にランダムな位置でOBJパーティクルを発生
	for (int i = 0; i < 2; ++i) {
		float x = Random::GeneratorFloat(-3.0f, 3.0f);
		float y = Random::GeneratorFloat(-2.0f, 2.0f);
		float z = Random::GeneratorFloat(-1.0f, 1.0f);
		Vector3 pos = { x, y, z };
		ParticleManager::GetInstance()->Emit("title_effect_obj", pos, 1);
	}

	ParticleManager::GetInstance()->Update(camera_->GetViewMatrix(), camera_->GetProjectionMatrix());

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
		// スペースキーまたはAボタンが押されたらフェードアウトへ
		if (keyInput_->PushKey(DIK_SPACE) || keyInput_->IsPadButtonPressed(XINPUT_GAMEPAD_A)) {
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
	ParticleManager::GetInstance()->Draw();

	model_->Draw();
	fade_->Draw();

}
