#pragma once

#include "Bomb.h"
#include "Camera.h"
#include "DebugCamera.h"
#include "DirectXCom.h"
#include "ModelManager.h"
#include "Object3d.h"
#include "Object3dCom.h"
#include "SoundManager.h"
#include "Sprite.h"
#include "SpriteCom.h"
#include "SrvManager.h"
#include "TextureManager.h"
#include "WinApp.h"
#include"Fade.h"
#include"LeafEffect.h"

#include "Apple.h"
#include "Player.h"

// パーティクル
#include "ParticleEmitter.h"
#include "ParticleManager.h"
#include <unordered_map>
#include <array>


#include "Effect.h"
#include "TileWaveEffect.h"
#include "ActorWaveEffect.h"
#include "ScaleBounce.h"
#include "DeathEffect.h"

class Tutorial
{

public:
	Tutorial() = default;
	~Tutorial();

	void Initialize(Object3dCom* object3dCom, SpriteCom* spriteCom);
	void Update();
	void Draw();

	void Reset();

	void Finalize();

	void GenerateWalls(Object3dCom* object3dCom, Model* model, const Vector3& wallScale);

	void LoadAndGetModel(Model*& outModel, const std::string& filePath);

	void RespawnAppleRandom(Apple* apple);

	Object3d* makeObject(Object3dCom* object3dCom, Model* model,
		const Vector3& pos, const Vector3& scale);

	bool CheckClearFilledInside() const;

	void ResolveAppleBombOverlap(MapChipField* map, const Player& player,
		std::vector<Apple*>& apples,
		std::vector<Bomb*>& bombs);

public: // setter getter
	void SetWinApp(WinApp* winApp) { winApp_ = winApp; }
	DebugCamera* GetDebugCamera() const { return debugCamera_; }
	Camera* GetCamera() const { return camera; }

	bool IsFinished() const { return finished_; }
	bool IsCleared() const { return cleared_; }

	// 追加: eatApple_ のスケール操作用
	void SetEatAppleScale(const Vector2& pixelSize);
	void SetEatAppleScaleRatio(float ratio);

	// 爆弾の出現状態
	bool IsBombSpawned() const { return bombsSpawned_; }                 // 持続
	bool WasBombSpawnedThisFrame() const { return bombSpawnedThisFrame_; } // ワンショット
	// 5個後の爆弾出現をユーザー操作で解除するためのフラグ
	bool IsBombRevealActive() const { return bombRevealAwaitSpace_; }

	// 追加: チュートリアルで初めて体を切り離したか
	bool HasDetachedOnce() const { return hasDetachedOnce_; }

private:
	const int32_t kWindowWidth = WinApp::kClientWidth;   // 1280
	const int32_t kWindowHeight = WinApp::kClientHeight; // 720

	Camera* camera = nullptr;
	DebugCamera* debugCamera_ = nullptr;

	Object3d* player_Model_ = nullptr;
	Object3d* apple_Model_ = nullptr;
	Object3d* bomb_Model_ = nullptr;
	Object3d* wall_Model_ = nullptr;

	Sprite* backgroundSprite = nullptr;
	Sprite* bodySeparation = nullptr;
	Sprite* tutorialEatApple = nullptr;
	Sprite* reset = nullptr;
	Sprite* backGamePlay = nullptr;
	Sprite* backSelectScene = nullptr;
	Sprite* eatApple_ = nullptr;
	Sprite* bombCollide = nullptr;
	Sprite* bodySeparationTutorial = nullptr;
	Sprite* spaceOrAred = nullptr;
	Sprite* gameOverExplanation =   nullptr;

    Sprite *clearText = nullptr;
	Sprite* poseExplanation = nullptr;

	Sprite* move = nullptr;


	// applesTouchedCount_ を表示する桁スプライト（最大3桁表示）
	std::array<Sprite*, 3> countDigits_{ nullptr, nullptr, nullptr };
	int currentDigitCount_ = 0; // 現在表示している桁数

	WinApp* winApp_ = nullptr;

	Vector2 positionUv = { 0.0f, 0.0f };
	Vector3 positionCamera = { 0.0f, 0.0f, -10.0f };
	bool drawSprite = false;

	Player* player_ = nullptr;
	std::vector<Apple*> apples_;
	std::vector<Bomb*> bombs_;

	MapChipField* mapChipField_ = nullptr;

	std::vector<std::vector<Object3d*>> WorldTransformWalls_;

	
	bool finished_ = false;
	bool cleared_ = false;

	// パーティクルエミッタ
	ParticleEmitter* emitterBillboard_ = nullptr;
	ParticleEmitter* emitterMesh_ = nullptr;

	//葉っぱ
    LeafEffect leafEffect_;

	// Sound関係
	SoundData Player_Eat_Apple_SoundData_;
	SoundData decideSE_;
	SoundData tutorialBGM_;
	bool bgmStarted_ = false; // フェード完了後に一度だけBGMを再生するためのフラグ


	bool paused_ = false;
	float spaceHoldSec_ = 0.0f;
	float pauseHoldThreshold_ = 1.0f;

	bool started_ = false; // ゲーム開始フラグ

	Fade fade_;

	bool fadeStarted_ = false;

	std::unique_ptr<TileWaveEffect> tileWaveEffect_;
	std::unique_ptr<ActorWaveEffect> actorWave_;
	bool introLock_ = false;

	// 追加: 爆弾生成を5個のリンゴ取得まで遅延するための状態
	int applesTouchedCount_ = 0;   // プレイヤーが触れたリンゴの回数
	bool bombsSpawned_ = false;     // 爆弾を既に生成したか（持続フラグ）
	int bombInitialCount_ = 5;      // 生成する爆弾の数（必要なら調整）

	bool hasEatenMultipleApples_ = false;//リンゴを五個以上食べたか

	// 途中フェード（5個到達などの演出用）
	enum class MidFadePhase { None, Out, In };
	enum class MidFadeReason { None, BombSpawn, DeathReset };
	bool midFadeActive_ = false;
	MidFadePhase midFadePhase_ = MidFadePhase::None;
	MidFadeReason midFadeReason_ = MidFadeReason::None;
	float midFadeDuration_ = 30.0f; // フレーム数（0.5秒）

	// ヒットストップ（爆弾衝突時に一時停止）
	int hitStopFrames_ = 0;
	int hitStopDurationFrames_ = 12; // 0.2秒相当@60fps

	// 爆弾とプレイヤーの衝突で一度だけゲームを停止するためのフラグ
	bool bombHitPauseUsed_ = false;

	// スケールのバウンス演出
	ScaleBounce scaleBounce_;


	// ゲームオーバー演出
	bool gameOver_ = false;
	DeathEffect deathEffect_;

	// 追加: そのフレームに新規に爆弾が出現したことを示すフラグ
	bool bombSpawnedThisFrame_ = false;

	// 追加: フェード後に出現した爆弾の提示をSPACEで解除するためのフラグ
	bool bombRevealAwaitSpace_ = false;

	// 追加: 爆弾告知～解除までチュートリアルUIを確実に非表示にするフラグ
	bool suppressTutorialUI_ = false;

	// 追加: 初期チュートリアルUIを一度消したら二度と表示しないためのロック
	bool initialTutorialLocked_ = false;

	// 追加: チュートリアルでプレイヤーが初めて体を切り離したフラグ（永続）
	bool hasDetachedOnce_ = false;
        bool isChange_ = false;
        bool isPress_ = false;

		Sprite* overlay = nullptr;      // ポーズ時の半透明オーバーレイ
		int     pauseMenuIndex_ = 0;    // 0:戻る, 1:リセット, 2:セレクトへ

};