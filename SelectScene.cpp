#define NOMINMAX

#include "SelectScene.h"
#include "Camera.h"
#include "Model.h"
#include "ModelManager.h"
#include "Object3dCom.h"
#include "SpriteCom.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include "TextureManager.h"

extern KeyInput keyInput;
extern SoundManager* soundManager;

SelectScene::~SelectScene()
{
	delete TitleBackModel_;
	delete GameSceneModel_;
	delete TutorialModel_;
	delete camera_;
	delete spaceOrA_;
    delete background_;

	if (decideSE_.pBuffer)
	{
		soundManager->SoundUnload(&decideSE_);
	}
	soundManager->SoundUnload(&selectBGM_);
}

void SelectScene::Initialize(Object3dCom* object3dCom, SpriteCom* spriteCom)
{
	object3dCom_ = object3dCom;
	spriteCom_ = spriteCom;

	// カメラ（中央を見せる固定ビュー）
	camera_ = new Camera();
	camera_->SetTranslate(cameraPosition_);
	camera_->SetRotate({ 0.0f, 0.0f, 0.0f });
	camera_->Update();
	object3dCom_->SetDefaultCamera(camera_);

	// モデル生成（初期は Tutorial を中央 / Game を右 / Title を左）
	Model* mTut = nullptr;
	LoadAndGetModel(mTut, "stage1.obj");
	Model* mGame = nullptr;
	LoadAndGetModel(mGame, "stage2.obj");
	Model* mBack = nullptr;
	LoadAndGetModel(mBack, "backTitle.obj"); // 仮

	TutorialModel_ = makeObject(object3dCom_, mTut, tutPosCur_, { 1, 1, 1 });
	GameSceneModel_ = makeObject(object3dCom_, mGame, gamePosCur_, { 1, 1, 1 });
	TitleBackModel_ = makeObject(object3dCom_, mBack, backPosCur_, { 1, 1, 1 });

	// 初期スケール反映（選択中のみ大きく）
	if (TutorialModel_)
		TutorialModel_->SetScale(tutScaleCur_);
	if (GameSceneModel_)
		GameSceneModel_->SetScale(gameScaleCur_);
	if (TitleBackModel_)
		TitleBackModel_->SetScale(backScaleCur_);

	// スプライト
	spaceOrA_ = new Sprite();
	const std::string spaceOrAPath = "Resources/spaceOrA.png";

	// 使うぶんは先に登録
	TextureManager::GetInstance()->LoadTexture("Resources/spaceOrA.png");
	spaceOrA_->Initialize(spriteCom, spaceOrAPath);
	spaceOrA_->SetPosition({ 320.0f, 470.0f });
        
	background_ = new Sprite();
    const std::string bgPath = "Resources/backgroundTitle.png"; // 画面解像度と同サイズ推奨
    TextureManager::GetInstance()->LoadTexture(bgPath);
    background_->Initialize(spriteCom, bgPath);
    background_->SetPosition({0.0f, 0.0f});


	// フェードIN
	fade_.Initialize(spriteCom_);
	fade_.Start(FadeState::In, 60.0f);

	// 決定SE読み込み
	decideSE_ = soundManager->SoundLoadWave("Resources/Audio/SE/decision.wav");
	swipeSE_ = soundManager->SoundLoadWave("Resources/Audio/SE/swipe.wav");
	selectBGM_ = soundManager->SoundLoadWave("Resources/Audio/BGM/SelectScene.wav");

#ifdef _DEBUG
	assert(&decideSE_);
	assert(&swipeSE_);
#endif // _DEBUG


}

void SelectScene::Update()
{

	background_->Update();

	if (!bgmStarted_ && fade_.IsEnd())
	{
		soundManager->SoundPlayWave(selectBGM_, true);
		bgmStarted_ = true;
	}

	// --- 左スティック取得 ---
	auto s = keyInput.GetLeftStick(0);
	lStick.x = s.x;
	lStick.y = s.y;

	// スティックを Trigger 相当に扱う（ヒステリシス付き）
	// 倒し始め閾値と、ニュートラルに戻ったとみなす閾値を分ける
	constexpr float stickEnter = 0.6f; // これ以上で「押し始め」
	constexpr float stickExit = 0.3f; // これ未満で「離した」
	static bool stickRightHeld = false;
	static bool stickLeftHeld = false;

	bool stickRightTrigger = false;
	bool stickLeftTrigger = false;

	// 右方向
	if (lStick.x > stickEnter)
	{
		if (!stickRightHeld) { stickRightTrigger = true; }
		stickRightHeld = true;
	}
	else if (std::fabs(lStick.x) < stickExit)
	{
		stickRightHeld = false;
	}

	// 左方向
	if (lStick.x < -stickEnter)
	{
		if (!stickLeftHeld) { stickLeftTrigger = true; }
		stickLeftHeld = true;
	}
	else if (std::fabs(lStick.x) < stickExit)
	{
		stickLeftHeld = false;
	}

	// --- 入力：A/D・矢印・パッド（十字/スティック）で選択を循環 ---
	bool changed = false;
	if (keyInput.TriggerKey(DIK_D) || keyInput.TriggerKey(DIK_RIGHT) ||
		keyInput.TriggerPadButton(XINPUT_GAMEPAD_DPAD_RIGHT) ||
		stickRightTrigger)
	{
		soundManager->SoundPlayWave(swipeSE_);
		focusedIndex_ = (focusedIndex_ + 1) % 3; // 0→1→2→0
		changed = true;
	}
	else if (keyInput.TriggerKey(DIK_A) || keyInput.TriggerKey(DIK_LEFT) ||
		keyInput.TriggerPadButton(XINPUT_GAMEPAD_DPAD_LEFT) ||
		stickLeftTrigger)
	{
		soundManager->SoundPlayWave(swipeSE_);
		focusedIndex_ = (focusedIndex_ + 2) % 3; // 0←1←2←0
		changed = true;
	}

	// --- 割り当て更新（選択中を中央へ、残りを左右へ）---
	if (changed)
	{
		switch (focusedIndex_)
		{
		case 0: // Tutorial 中央 / Game 右 / Title 左
			tutPosDst_ = P_center;
			gamePosDst_ = P_right;
			backPosDst_ = P_left;
			break;
		case 1: // Game 中央 / Title 右 / Tutorial 左
			gamePosDst_ = P_center;
			backPosDst_ = P_right;
			tutPosDst_ = P_left;
			break;
		case 2: // Title 中央 / Tutorial 右 / Game 左
			backPosDst_ = P_center;
			tutPosDst_ = P_right;
			gamePosDst_ = P_left;
			break;
		}
	}

	// --- 深度の扱い：非選択は常に奥、選択は中央に近づいたら前に出る ---
	const float backZ = P_left.z;     // = 10.0f
	const float centerX = P_center.x; // = 5.0f
	const float centerPullWidth =
		3.0f; // 中央に近づいたら z を 10→0 に引き寄せる幅

	auto assignBackZ = [&](Vector3& dst)
		{
			dst.z = backZ; // 常に奥の平面上で移動
		};
	auto assignCenterZ = [&](Vector3& dst, const Vector3& cur)
		{
			// x が中央に近いほど z を 0 に近づける（遠い間は z=10 のまま）
			float dx = std::fabs(cur.x - centerX);
			float t =
				std::clamp(dx / centerPullWidth, 0.0f, 1.0f); // 1=遠い→z=10, 0=近い→z=0
			dst.z = backZ * t;
		};

	// 目的地Zを決め直す（中心に来るやつだけ段階的に 10→0 へ）
	switch (focusedIndex_)
	{
	case 0:
		assignCenterZ(tutPosDst_, tutPosCur_); // 中央へ来るやつ
		assignBackZ(gamePosDst_);              // 非選択は奥で移動
		assignBackZ(backPosDst_);
		break;
	case 1:
		assignBackZ(tutPosDst_);
		assignCenterZ(gamePosDst_, gamePosCur_);
		assignBackZ(backPosDst_);
		break;
	case 2:
		assignBackZ(tutPosDst_);
		assignBackZ(gamePosDst_);
		assignCenterZ(backPosDst_, backPosCur_);
		break;
	}

	// --- 位置をなめらかに補間 ---
	auto stepLerp = [&](Vector3& cur, const Vector3& dst)
		{
			cur = LerpVec(cur, dst, posLerp_);
			// 目標に十分近いならスナップしてピタッと止める
			if (std::fabs(cur.x - dst.x) < snapEps_ &&
				std::fabs(cur.y - dst.y) < snapEps_ &&
				std::fabs(cur.z - dst.z) < snapEps_)
			{
				cur = dst;
			}
		};
	stepLerp(tutPosCur_, tutPosDst_);
	stepLerp(gamePosCur_, gamePosDst_);
	stepLerp(backPosCur_, backPosDst_);

	if (TutorialModel_)
		TutorialModel_->SetTranslate(tutPosCur_);
	if (GameSceneModel_)
		GameSceneModel_->SetTranslate(gamePosCur_);
	if (TitleBackModel_)
		TitleBackModel_->SetTranslate(backPosCur_);

	// --- スケール：前面に来てから拡大（奥を走ってる間は拡大しない）---
	auto decideScale = [&](const Vector3& cur, bool isFocused) -> Vector3
		{
			if (!isFocused)
				return baseScale_; // 非選択は常にベース
			if (cur.z > 1.0f)
				return baseScale_; // まだ奥にいる間はベース
			if (std::fabs(cur.x - centerX) > 0.5f)
				return baseScale_; // ほぼ中央に来たら拡大
			return focusScale_;
		};

	const Vector3 tutTargetScale = decideScale(tutPosCur_, focusedIndex_ == 0);
	const Vector3 gameTargetScale = decideScale(gamePosCur_, focusedIndex_ == 1);
	const Vector3 backTargetScale = decideScale(backPosCur_, focusedIndex_ == 2);

	tutScaleCur_ = LerpVec(tutScaleCur_, tutTargetScale, 0.2f);
	gameScaleCur_ = LerpVec(gameScaleCur_, gameTargetScale, 0.2f);
	backScaleCur_ = LerpVec(backScaleCur_, backTargetScale, 0.2f);

	if (TutorialModel_)
		TutorialModel_->SetScale(tutScaleCur_);
	if (GameSceneModel_)
		GameSceneModel_->SetScale(gameScaleCur_);
	if (TitleBackModel_)
		TitleBackModel_->SetScale(backScaleCur_);

	// --- 決定（Space / Pad A）→ フェード開始 ---
	if ((keyInput.TriggerKey(DIK_SPACE) ||
		keyInput.TriggerPadButton(XINPUT_GAMEPAD_A)) &&
		!fadeStarted_)
	{

		soundManager->SoundPlayWave(decideSE_);

		fade_.Start(FadeState::Out, 60.0f);
		fadeStarted_ = true;
	}

	// --- フェード終了で遷移フラグON ---
	fade_.Update();
	if (fadeStarted_ && fade_.IsEnd())
	{
		if (focusedIndex_ == 0)
			Select_TutorialScene = true;
		else if (focusedIndex_ == 1)
			Select_GameScene = true;
		else if (focusedIndex_ == 2)
			Select_TitleScene = true;
	}

	// --- Model の Update ---
	if (TutorialModel_)
		TutorialModel_->Update();
	if (GameSceneModel_)
		GameSceneModel_->Update();
	if (TitleBackModel_)
		TitleBackModel_->Update();

	//スプライトのUpdate
	if (!fadeStarted_)
	{
		spaceOrA_->Update();
	}

}



void SelectScene::Draw()
{

	background_->Draw();

	if (TutorialModel_)
	{
		TutorialModel_->Draw();
	}
	if (GameSceneModel_)
	{
		GameSceneModel_->Draw();
	}
	if (TitleBackModel_)
	{
		TitleBackModel_->Draw();
	}

	spaceOrA_->Draw();

	fade_.Draw();
}

void SelectScene::LoadAndGetModel(Model*& outModel,
	const std::string& filePath)
{
	auto* mm = ModelManager::GetInstance();
	mm->LoadModel(filePath);
	outModel = mm->FindModel(filePath);
}

Object3d* SelectScene::makeObject(Object3dCom* object3dCom, Model* model,
	const Vector3& pos, const Vector3& scale)
{
	auto* obj = new Object3d();
	obj->Initialize(object3dCom);
	obj->SetModel(model);
	obj->SetTranslate(pos);
	obj->SetScale(scale);
	return obj;
}
