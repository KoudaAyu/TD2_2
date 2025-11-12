#pragma once

#include"Bomb.h"
#include"Camera.h"
#include"DebugCamera.h"
#include"ModelManager.h"
#include"Object3d.h"
#include"Object3dCom.h"
#include"SoundManager.h"
#include"Sprite.h"
#include"SpriteCom.h"
#include"TextureManager.h"
#include"WinApp.h"
#include "DirectXCom.h"
#include "SrvManager.h" 
#include"Fade.h"

#include"Apple.h"
#include"Player.h"


// パーティクル
#include "ParticleManager.h"
#include "ParticleEmitter.h"
#include <unordered_map>
#include "LeafEffect.h"

#include "Effect.h"
#include "TileWaveEffect.h"
#include "ActorWaveEffect.h"
#include "ScaleBounce.h"
#include "DeathEffect.h" // 追加: 死亡演出
#include <filesystem>

class GameScene
{
public:
	GameScene() = default;
	~GameScene();

	void Initialize(Object3dCom* object3dCom, SpriteCom* spriteCom);
	void Update();
	void Draw();

	void Reset();

	void Finalize();

	void GenerateWalls(Object3dCom* object3dCom, Model* model, const Vector3& wallScale);

	void LoadAndGetModel(Model*& outModel, const std::string& filePath);



	Object3d* makeObject(Object3dCom* object3dCom, Model* model, const Vector3& pos, const Vector3& scale);


	bool CheckClearFilledInside() const;

	void ResolveAppleBombOverlap(MapChipField* map, const Player& player,
		std::vector<Apple*>& apples,
		std::vector<Bomb*>& bombs);

public://setter getter
	void SetWinApp(WinApp* winApp) { winApp_ = winApp; }
	DebugCamera* GetDebugCamera() const { return debugCamera_; }
	Camera* GetCamera() const { return camera; }

	bool IsFinished() const { return finished_; }
	bool IsCleared() const { return cleared_; }


private:

	const int32_t kWindowWidth = WinApp::kClientWidth;//1280
	const int32_t kWindowHeight = WinApp::kClientHeight;//720

	Camera* camera = nullptr;
	DebugCamera* debugCamera_ = nullptr;

	Object3d* player_Model_ = nullptr;
	Object3d* apple_Model_ = nullptr;
	Object3d* bomb_Model_ = nullptr;
	Object3d* wall_Model_ = nullptr;

	Sprite* uvChecker = nullptr;
	Sprite* clearCondition_ = nullptr;
	Sprite* backgroundSprite = nullptr;
	Sprite* bodySeparation = nullptr;
	Sprite* reset = nullptr;
	Sprite* backGamePlay = nullptr;
	Sprite* backSelectScene = nullptr;
	Sprite* overlay = nullptr;
	Sprite* move = nullptr;
	Sprite* poseExplanation = nullptr;

	WinApp* winApp_ = nullptr;

	Vector2 positionUv = { 0.0f, 0.0f };
	Vector3 positionCamera = { 0.0f, 0.0f, -10.0f };
	bool drawSprite = false;

	Player* player_ = nullptr;
	std::vector<Apple*>apples_;
	std::vector<Bomb*> bombs_;

	MapChipField* mapChipField_ = nullptr;

	std::vector<std::vector<Object3d*>> WorldTransformWalls_;

	bool finished_ = false;
	bool cleared_ = false;

	// パーティクルエミッタ
	ParticleEmitter* emitterBillboard_ = nullptr;
	ParticleEmitter* emitterMesh_ = nullptr;

	//背景の葉っぱ
	LeafEffect leafEffect_;

	//Sound関係
	SoundData Player_Eat_Apple_SoundData_;

	bool paused_ = false;
	float spaceHoldSec_ = 0.0f;
	float pauseHoldThreshold_ = 1.0f;

	bool started_ = false; // ゲーム開始フラグ

	std::unique_ptr<TileWaveEffect> tileWaveEffect_;
	std::unique_ptr<ActorWaveEffect> actorWave_;

	bool introLock_ = false;

	Fade fade_;

	bool bgmStarted_ = false;

	SoundData GameBGM_;

	ScaleBounce scaleBounce_;

	// 追加: ゲームオーバー処理/演出
	bool gameOver_ = false;
	DeathEffect deathEffect_;

	// Pause メニュー選択インデックス
	int pauseMenuIndex_ = 0;

	// セレクトに戻るフラグ（必要に応じて使用）
	bool selectSelect_ = false;

	// 動的リンゴ数制御
	int appleTouchCount_ = 0; // プレイヤーが触れた累計
	static constexpr int kMaxApples_ = 10; // 上限

	// ボム出現制御
	static constexpr int kMaxBombs_ = 5; // 同時最大ボム数
	static constexpr int kApplesPerBomb_ = 3; // 何個のリンゴ取得ごとに1個ボム追加するか

	// 1個のリンゴを安全に生成して配置
	void SpawnOneApple();
	// 1個のボムを安全に生成して配置
	void SpawnOneBomb();

	// リザルトフラグ
	bool resultMode_ = false;
	Sprite* overlayResult_ = nullptr;

	// --- リンゴ取得カウンタ ---
	int applesCollected_ = 0;        // 取った総数

	// 数字スプライト（最大3桁想定）
	std::array<Sprite*, 3> appleDigits_{ nullptr, nullptr, nullptr };
	int appleDigitCount_ = 0;        // 今フレーム表示している桁数

	// --- 壁設置カウンタ ---
	int wallsPlaced_ = 0;                             // 置いた壁の総数
	std::array<Sprite*, 3> wallDigits_{ nullptr, nullptr, nullptr };
	int wallDigitCount_ = 0;

	Sprite* resultLabelApple_ = nullptr; // 「リンゴ○食べた！」見出し
	Sprite* resultLabelWall_ = nullptr; // 「壁○作った！」見出し
};