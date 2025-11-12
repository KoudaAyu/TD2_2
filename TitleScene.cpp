#include "TitleScene.h"
#include "Camera.h"
#include "Model.h"
#include "ModelManager.h"
#include "Object3dCom.h"
#include "SpriteCom.h"
#include "TextureManager.h"
#include "WinApp.h"

extern KeyInput keyInput;
extern SoundManager* soundManager;

TitleScene::~TitleScene()
{
	if (titleBGM_.pBuffer)
	{
		soundManager->SoundUnload(&titleBGM_);
	}
	if (decideSE_.pBuffer)
	{
		soundManager->SoundUnload(&decideSE_);
	}

	delete titleTextModel_;
	delete camera_;
	delete spaceOrA_;
        delete background_;
}

void TitleScene::Initialize(Object3dCom* object3dCom, SpriteCom* spriteCom)
{
	object3dCom_ = object3dCom;
	spriteCom_ = spriteCom;

	// カメラ
	camera_ = new Camera();
	camera_->SetRotate({ 0.0f, 0.0f, 0.0f });
	camera_->SetTranslate({ 0.0f, 5.0f, -30.0f });
	camera_->Update();
	object3dCom_->SetDefaultCamera(camera_);

	// タイトルテキスト
	Model* modelTitleText = nullptr;
	LoadAndGetModel(modelTitleText, "titleTextNo.obj");
	titleTextModel_ = makeObject(object3dCom, modelTitleText,
		startPos_,    // ← 開始位置
		startScale_); // ← 開始スケール(0)

	// スプライト
	spaceOrA_ = new Sprite();
	const std::string spaceOrAPath = "Resources/spaceOrA.png";

	// 使うぶんは先に登録
	TextureManager::GetInstance()->LoadTexture("Resources/spaceOrA.png");
	spaceOrA_->Initialize(spriteCom, spaceOrAPath);
	spaceOrA_->SetPosition({ 320.0f,450.0f });

	background_ = new Sprite();
        const std::string bgPath =
            "Resources/backgroundTitle.png"; // 画面解像度と同サイズ推奨
        TextureManager::GetInstance()->LoadTexture(bgPath);
        background_->Initialize(spriteCom, bgPath);
        background_->SetPosition({0.0f, 0.0f});

	// フェード
	fade_.Initialize(spriteCom_);
	fade_.Start(FadeState::In, 60.0f);

	// BGM / SE 読み込み
	titleBGM_ = soundManager->SoundLoadWave("Resources/Audio/BGM/Title.wav");
	decideSE_ = soundManager->SoundLoadWave("Resources/Audio/SE/decision.wav");
#ifdef _DEBUG
	assert(titleBGM_.pBuffer && titleBGM_.bufferSize > 0);
	assert(decideSE_.pBuffer && decideSE_.bufferSize > 0);
#endif
	// BGM 再生
	soundManager->SoundPlayWave(titleBGM_, true);
}

void TitleScene::Update()
{
  background_->Update();

	if (!isGameStart_)
	{
		if (keyInput.TriggerKey(DIK_SPACE) ||
			keyInput.TriggerPadButton(XINPUT_GAMEPAD_A))
		{
			isGameStart_ = true; // （必要なら）カメラドリーはお好みで
			t = camera_->GetTranslate();

			// 決定音再生
			soundManager->SoundPlayWave(decideSE_);
		}
	}

	if (isGameStart_)
	{
		// Z を前進させる（-30 → 10 まで）
		const float dollyPerFrame = 1.0f; // 速度はお好みで
		t.z += dollyPerFrame;

		if (t.z >= 10.0f)
		{

			t.z = 10.0f;
			if (!fadeStarted_)
			{
				fade_.Start(FadeState::Out, 60.0f); // 開始時フェードイン（1秒）
				fadeStarted_ = true;
			}
		}
		camera_->SetTranslate(t); // 前進を反映
	}
	else
	{
		// 非遷移中は念のため同期
		t = camera_->GetTranslate();
	}

	// 最初のイージング
	if (timer_ < easeDuration_)
	{

		timer_++;
		float t = timer_ / easeDuration_;
		float e = EaseOutBack(t);

		// 位置をLerp
		Vector3 pos{ startPos_.x + (endPos_.x - startPos_.x) * e,
					startPos_.y + (endPos_.y - startPos_.y) * e,
					startPos_.z + (endPos_.z - startPos_.z) * e };
		// スケールをLerp（拡大演出）
		Vector3 sca{ startScale_.x + (endScale_.x - startScale_.x) * e,
					startScale_.y + (endScale_.y - startScale_.y) * e,
					startScale_.z + (endScale_.z - startScale_.z) * e };

		if (titleTextModel_)
		{
			titleTextModel_->SetTranslate(pos);
			titleTextModel_->SetScale(sca);
		}

	}
	else
	{
		idle_ = true;
	}

	// ゆらゆらするイージング
	if (idle_ && titleTextModel_)
	{
		// 時間を進める
		totalTime_ += 1.0f / 110.0f;

		// 回転：軽く首振り（Z軸）
		auto rot = titleTextModel_->GetRotate();
		rot.z = swayAmp_ * sinf(totalTime_ * 2.0f * 3.14159265f * swayHz_);

		// スケール：呼吸（等方）
		float breathe =
			1.0f + breatheAmp_ * sinf(totalTime_ * 2.0f * 3.14159265f * breatheHz_);
		Vector3 sca = { endScale_.x * breathe, endScale_.y * breathe,
					   endScale_.z * breathe };

		titleTextModel_->SetRotate(rot);
		titleTextModel_->SetScale(sca);
	}

	float camOrbitAmp_ = 0.8f; // カメラの左右オービット幅
	float camOrbitHz_ = 0.1f;  // 周期
	float camDollyAmp_ = 0.6f; // Z(前後)ドリー幅
	float camDollyHz_ = 0.07f; // 周期

	// Update の idle_ 内で
	if (idle_ && camera_ && !isGameStart_)
	{
		Vector3 camT = { 0.0f, 5.0f, -30.0f };
		camT.x +=
			camOrbitAmp_ * sinf(totalTime_ * 2.0f * 3.14159265f * camOrbitHz_);
		camT.z +=
			camDollyAmp_ * sinf(totalTime_ * 2.0f * 3.14159265f * camDollyHz_);
		camera_->SetTranslate(camT);
	}

	if (camera_)
	{
		camera_->Update();
	}

	// タイトルテキストの更新
	titleTextModel_->Update();

	spaceOrA_->Update();

	// フェードアウト
	fade_.Update();
	if (isGameStart_ && !finished_ && fade_.IsEnd() && fadeStarted_)
	{
		finished_ = true;
	}
}


void TitleScene::Draw()
{
  background_->Draw();

	if (titleTextModel_)
	{
		titleTextModel_->Draw();
	}

	if (!isGameStart_)
	{
		spaceOrA_->Draw();
	}

	fade_.Draw();
}

void TitleScene::LoadAndGetModel(Model*& outModel,
	const std::string& filePath)
{
	auto* mm = ModelManager::GetInstance();
	mm->LoadModel(filePath);
	outModel = mm->FindModel(filePath);
}

Object3d* TitleScene::makeObject(Object3dCom* object3dCom, Model* model,
	const Vector3& pos, const Vector3& scale)
{
	auto* obj = new Object3d();
	obj->Initialize(object3dCom);
	obj->SetModel(model);
	obj->SetTranslate(pos);
	obj->SetScale(scale);
	return obj;
}

float TitleScene::EaseOutBack(float t)
{
	const float c1 = 1.70158f;
	const float c3 = c1 + 1.0f;
	return 1.0f + c3 * powf(t - 1.0f, 3.0f) + c1 * powf(t - 1.0f, 2.0f);
}