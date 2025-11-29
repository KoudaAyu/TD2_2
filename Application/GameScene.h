#pragma once
#include"Camera.h"
#include"DebugCamera.h"
#include"Enemy.h"
#include"KeyInput.h"
#include"Object3d.h"
#include"Object3dCom.h"
#include"Player.h"
#include"RailCameraController.h"
#include"Fade.h"
#include "Boss.h"

class GameScene
{
public:
	GameScene() = default;
	~GameScene();
	void Initialize(Camera* camera, Object3dCom* object3dCom, SpriteCom* spriteCom);
	void Update();
	void Draw();

	/// <summary>
	/// 衝突判定と衝突処理の実行
	/// </summary>
	void CheckAllCollisions();

public:
	bool IsFinish() const { return isFinish_; }

private:

	bool isFinish_ = false;
	bool isDebugCameraActive_ = false;

private:

	Camera* camera_ = nullptr;
	DebugCamera* debugCamera_ = nullptr;
	Enemy* enemy_ = nullptr;
	KeyInput* keyInput_ = nullptr;
	Object3d* model_ = nullptr;
	Object3d* enemyModel_ = nullptr;
	Object3dCom* object3dCom_ = nullptr;
	Player* player_ = nullptr;
	RailCameraController* railCameraController_ = nullptr;

	// Debug boss body model (bomb.obj)
	Object3d* bossBodyModel_ = nullptr;
	Boss* boss_ = nullptr;

	Fade* fade_ = nullptr;

	// SpriteCom pointer saved for resets
	SpriteCom* spriteCom_ = nullptr;

	enum class Phase { kMain, kFadeOut };
	Phase phase_ = Phase::kMain;

	// Reset helper (only available in debug builds)
#ifdef _DEBUG
	void ResetScene();
#endif

};
