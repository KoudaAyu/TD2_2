#pragma once
#include"Camera.h"
#include"DebugCamera.h"
#include"Enemy.h"
#include"KeyInput.h"
#include"Skydome.h"
#include"Object3d.h"
#include"Object3dCom.h"
#include"Player.h"
#include"RailCameraController.h"
#include"Fade.h"
#include"Boss.h"
#include "ClearScene.h"

#include <vector>
#include <string>
#include <memory>

// UI
#include "Application/UI/UIManager.h"

// Audio
#include "SoundManager.h"

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

	void InitializeUI(SpriteCom* spriteCom);

public:
	bool IsFinish() const { return isFinish_; }
	// whether boss was defeated and clear scene is active
	bool IsCleared() const { return phase_ == Phase::kClear; }

	// whether the scene requested a game over transition (player died)
	bool IsGameOverRequested() const { return isGameOverRequested_; }

	// デバッグ用: ボスから開始するフラグを設定（Initialize前に設定しておく）
	void SetStartAtBoss(bool enable) { startAtBoss_ = enable; }
	bool GetStartAtBoss() const { return startAtBoss_; }

private:
	bool isFinish_ = false;
	bool isDebugCameraActive_ = false;
	bool isPaused_ = false; 

	//敵の数
	const int enemyCount = 5;

	// 敵の出現Z位置（
	float enemySpawnZ_ = 30.0f;

private:

	Camera* camera_ = nullptr;
	DebugCamera* debugCamera_ = nullptr;
	// Enemy* enemy_ = nullptr; 
	std::vector<Enemy*> enemies_;
	KeyInput* keyInput_ = nullptr;
	Object3d* model_ = nullptr;
	Object3d* playerModel_ = nullptr;
	// Object3d* enemyModel_ = nullptr; 
	Object3dCom* object3dCom_ = nullptr;
	Player* player_ = nullptr;
	RailCameraController* railCameraController_ = nullptr;

	// Boss objects
	Object3d* bossBodyModel_ = nullptr;
	Boss* boss_ = nullptr;

	Fade* fade_ = nullptr;

	Skydome* skydome_ = nullptr;
	
	SpriteCom* spriteCom_ = nullptr;

	// Application-level simple sprite particles for effects that need alpha-fade
	struct AppParticle {
		Sprite* sprite = nullptr;
		float life = 1.0f;
		float age = 0.0f;
		Vector2 pos; // screen position
		Vector2 vel;
	};
	std::vector<AppParticle> appParticles_;
	// texture used for application particles (set during Initialize)
	std::string particleTexturePath_;

	// Application-level mesh particles (OBJ) that fade over time
	struct AppMeshParticle {
		Object3d* obj = nullptr; // owns Object3d
		Model* model = nullptr;   // owns Model copy
		float life = 1.0f;
		float age = 0.0f;
		Vector3 vel; // world-space velocity
	};
	std::vector<AppMeshParticle> appMeshParticles_;

	enum class Phase { kMain, kBoss, kFadeOut, kClear };
	Phase phase_ = Phase::kMain;

	
	int currentWave_ = 0;
	int maxWaves_ = 4; 
	void SpawnWave();

	bool isWaitingForNextWave_ = false;
	float waveDelay_ = 3.0f; 
	float waveDelayTimer_ = 0.0f;

#ifdef _DEBUG
	void ResetScene();
#endif

	// UI manager for this scene
	UIManager uiManager_;

	// Pause overlay sprite
	Sprite* pauseSprite_ = nullptr;
	Sprite* wasdSprite_ = nullptr;
	Sprite* spaceSprite_ = nullptr;

	// デバッグ用: 起動時にボスフェーズから開始するかどうか
	bool startAtBoss_ = false;

	// Clear scene
	ClearScene* clearScene_ = nullptr;

	// flag raised when player death requests transition to GameOver
	bool isGameOverRequested_ = false;

	// --- Audio: BGM support ---
	SoundManager* soundManager_ = nullptr;
	SoundData bgmData_ = {};
	bool hasBgm_ = false;
};
