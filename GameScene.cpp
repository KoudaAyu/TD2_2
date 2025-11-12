#include"GameScene.h"
#include<cassert>
#include <algorithm>
#include <random>
#include <unordered_set>
#include <cmath>

extern Object3dCom* object3dCom;
extern SpriteCom* spriteCom;
extern KeyInput keyInput;
extern SoundManager* soundManager;
extern DirectXCom* directXCom;
extern SrvManager* srvManager;


GameScene::~GameScene()
{
	if (emitterBillboard_) { delete emitterBillboard_; emitterBillboard_ = nullptr; }
	if (emitterMesh_) { delete emitterMesh_; emitterMesh_ = nullptr; }
	ParticleManager::GetInstance()->Finalize();

	delete camera;
#ifdef _DEBUG
	delete debugCamera_;
#endif

	for (auto* apple : apples_)
	{
		delete apple;
	}
	apples_.clear();

	// Bomb 本体
	for (auto* bomb : bombs_)
	{
		delete bomb;
	}
	bombs_.clear();

	// 壁Transform
	for (auto& row : WorldTransformWalls_)
	{
		for (auto* obj : row)
		{
			delete obj;
		}
	}
	WorldTransformWalls_.clear();

	// 参照関係のある順に破棄
	delete player_;
	delete player_Model_;


	delete apple_Model_;
	delete bomb_Model_;
	delete wall_Model_;

	delete mapChipField_;
	delete uvChecker;

	delete clearCondition_;
	delete backgroundSprite;
	delete bodySeparation;
	delete reset;
	delete backGamePlay;
	delete backSelectScene;
	delete overlay;
	delete poseExplanation;

	delete move;
	resultMode_ = false;
	if (overlayResult_) { delete overlayResult_; overlayResult_ = nullptr; }
	for (auto*& s : appleDigits_) { delete s; s = nullptr; }
	for (auto*& s : wallDigits_) { delete s; s = nullptr; }

	if (resultLabelApple_) { delete resultLabelApple_; resultLabelApple_ = nullptr; }
	if (resultLabelWall_) { delete resultLabelWall_;  resultLabelWall_ = nullptr; }

	soundManager->SoundUnload(&GameBGM_);

}

void GameScene::Initialize(Object3dCom* object3dCom, SpriteCom* spriteCom)
{
#ifdef _DEBUG

	debugCamera_ = new DebugCamera();
	debugCamera_->Initialize();
	assert(debugCamera_);

#endif

#ifdef _DEBUG

#endif
	// カメラ
	camera = new Camera();
	camera->SetTranslate({ 10.0f, -17.0f, -42.0f });
	camera->SetRotate({ -30.0f * 3.141592f / 180.0f, 0.0f, 0.0f });
#ifdef _DEBUG
	assert(camera);
#endif
	// 絶対Cameraの後
	object3dCom->SetDefaultCamera(camera);

	mapChipField_ = new MapChipField();
	mapChipField_->Initialize();
	mapChipField_->LoadmapChipCsv("Resources/map/Block.csv");

	// スプライト
	uvChecker = new Sprite();
	const std::string uvCheckerPath = "Resources/uvChecker.png";

	// 使うぶんは先に登録
	TextureManager::GetInstance()->LoadTexture("Resources/uvChecker.png");
	uvChecker->Initialize(spriteCom, uvCheckerPath);
	uvChecker->SetPosition(uvChecker->GetPosition());



	clearCondition_ = new Sprite();
	const std::string clearConditionPath("Resources/clearCondition.png");

	TextureManager::GetInstance()->LoadTexture(clearConditionPath);
	clearCondition_->Initialize(spriteCom, clearConditionPath);
	clearCondition_->SetPosition({ 400.0f, 550.0f });
	clearCondition_->SetScale({ 480.0f, 192.0f });


	// Pause overlay setup
	overlay = new Sprite();
	const std::string overlayPath = "Resources/white.png";
	TextureManager::GetInstance()->LoadTexture(overlayPath);
	overlay->Initialize(spriteCom, overlayPath);
	const float margin = 100.0f;
	// 左上の位置を余白ぶんずらす
	overlay->SetPosition({ margin * 4.0f, margin });
	// 画面サイズから余白×2を引いたサイズに
	overlay->SetScale({
		(float)winApp_->GetClientWidth() - margin * 8.0f,
		(float)winApp_->GetClientHeight() - margin * 2.0f
		});

	// Pause overlay setup
	overlayResult_ = new Sprite();
	const std::string overlayResultPath = "Resources/white.png";
	TextureManager::GetInstance()->LoadTexture(overlayResultPath);
	overlayResult_->Initialize(spriteCom, overlayResultPath);
	// 左上の位置を余白ぶんずらす
	overlayResult_->SetPosition({ 0, 0 });
	overlayResult_->SetColor({ 1.0f, 1.0f, 1.0f, 0.9f });
	// 画面サイズから余白×2を引いたサイズに
	overlayResult_->SetScale({
		(float)winApp_->GetClientWidth() ,
		(float)winApp_->GetClientHeight()
		});

	// Pause menu sprites
	backGamePlay = new Sprite();
	const std::string backGamePlayPath = "Resources/backGamePlay.png";
	TextureManager::GetInstance()->LoadTexture(backGamePlayPath);
	backGamePlay->Initialize(spriteCom, backGamePlayPath);
	backGamePlay->SetPosition({ 570.0f, 200.0f });

	reset = new Sprite();
	const std::string resetPath = "Resources/reset.png";
	TextureManager::GetInstance()->LoadTexture(resetPath);
	reset->Initialize(spriteCom, resetPath);
	reset->SetPosition({ 580.0f, 325.0f });

	backSelectScene = new Sprite();
	const std::string backSelectScenePath = "Resources/backSelectScene.png";
	TextureManager::GetInstance()->LoadTexture(backSelectScenePath);
	backSelectScene->Initialize(spriteCom, backSelectScenePath);
	backSelectScene->SetPosition({ 510.0f, 400.0f });

	//// 元のテクスチャサイズを取得
	//Vector2 baseSize = clearCondition_->GetTextureSize();

	//// 倍率をかけた値をセット
	//clearCondition_->SetScale({ baseSize.x * 0.5f, baseSize.y * 0.5f });

	backgroundSprite = new Sprite();
	const std::string bgPath = "Resources/background.png"; // 画面解像度と同サイズ推奨
	TextureManager::GetInstance()->LoadTexture(bgPath);
	backgroundSprite->Initialize(spriteCom, bgPath);
	backgroundSprite->SetPosition({ 0.0f, 0.0f });

	// 見出し画像のロード
	{
		const std::string eatApplePath = "Resources/eatAppleCount.png";
		const std::string createWallPath = "Resources/createWallCount.png";
		TextureManager::GetInstance()->LoadTexture(eatApplePath);
		TextureManager::GetInstance()->LoadTexture(createWallPath);

		resultLabelApple_ = new Sprite();
		resultLabelApple_->Initialize(spriteCom, eatApplePath);
		// 位置はリザルト時に中央寄せで描くので、ここでは仮でOK

		resultLabelWall_ = new Sprite();
		resultLabelWall_->Initialize(spriteCom, createWallPath);
	}

	move = new Sprite();
	const std::string movePath = "Resources/move.png";
	TextureManager::GetInstance()->LoadTexture(movePath);
	move->Initialize(spriteCom, movePath);
	move->SetPosition({ 10.0f, 50.0f });

	bodySeparation = new Sprite();
	const std::string bodySeparationPath = "Resources/bodySeparation.png";
	TextureManager::GetInstance()->LoadTexture(bodySeparationPath);
	bodySeparation->Initialize(spriteCom, bodySeparationPath);
	bodySeparation->SetPosition({ 20.0f, 150.0f });

	poseExplanation = new Sprite();
	const std::string poseExplanationPath = "Resources/poseExplanation.png";
	TextureManager::GetInstance()->LoadTexture(poseExplanationPath);
	poseExplanation->Initialize(spriteCom, poseExplanationPath);
	poseExplanation->SetPosition({ 950.0f, 100.0f });

	// 数字(0~9)のテクスチャを事前ロード（存在するフォルダだけでOK）
	{
		const char* dirs[] = {
			"Resources/number",
			"Resources/Numbers",
			"Resources/Number",
			"Resources/Textures/Numbers",
			"Resources/Textures",
			"Resources"
		};

		for (int n = 0; n <= 9; ++n) {
			for (const char* d : dirs) {
				std::string path = std::string(d) + "/" + std::to_string(n) + ".png";
				if (std::filesystem::exists(path)) {
					TextureManager::GetInstance()->LoadTexture(path);
					break; // 見つかったら次の数字へ
				}
			}
		}
	}

	// 桁スプライト生成（初期は 0.png）
	{
		std::string zero = "Resources/number/0.png"; // 実配置に合わせて
		for (auto*& s : appleDigits_) {
			s = new Sprite();
			s->Initialize(spriteCom, zero);
		}
		// 画面左上近くに小さく配置（お好みで）
		Vector2 base{ 20.0f, 20.0f };     // 表示位置
		Vector2 size{ 48.0f, 64.0f };     // 1桁のピクセルサイズ
		for (int i = 0; i < 3; ++i) {
			appleDigits_[i]->SetPosition({ base.x + i * (size.x + 4.0f), base.y });
			appleDigits_[i]->SetScale(size);
			// 半透明の白いプレートを重ねたい場合はSetColorで調整可
		}
	}

	// --- 壁の桁スプライト生成（初期0.png） ---
	{
		std::string zero = "Resources/number/0.png"; // あなたの配置に合わせる
		for (auto*& s : wallDigits_) {
			s = new Sprite();
			s->Initialize(spriteCom, zero);
		}

		// 表示位置（例：リンゴカウンタのすぐ下）
		Vector2 base{ 20.0f, 20.0f + 72.0f }; // yを少し下げる
		Vector2 size{ 48.0f, 64.0f };

		for (int i = 0; i < 3; ++i) {
			wallDigits_[i]->SetPosition({ base.x + i * (size.x + 4.0f), base.y });
			wallDigits_[i]->SetScale(size);
		}
	}

	Model* modelPlayer = nullptr;
	LoadAndGetModel(modelPlayer, "player.obj");
	player_Model_ = makeObject(object3dCom, modelPlayer, { 0.0f, 0.0f, 0.0f }, { 1.0f,1.0f,1.0f });

	Model* modelApple = nullptr;
	LoadAndGetModel(modelApple, "apple.obj");
	// ★修正1: 共有モデルは、画面外かつ非常に小さなスケールに設定
	apple_Model_ = makeObject(object3dCom, modelApple, { 1000.0f, 1000.0f, 1000.0f }, { 0.001f, 0.001f, 0.001f });

	Model* modelBomb = nullptr;
	LoadAndGetModel(modelBomb, "bomb.obj");
	bomb_Model_ = makeObject(object3dCom, modelBomb, { 0.0f, 0.0f, 0.0f }, { 1.0f,1.0f,1.0f });

	Model* modelWall = nullptr;
	LoadAndGetModel(modelWall, "wall.obj");
	wall_Model_ = makeObject(object3dCom, modelWall, { 0.0f, 0.0f, 0.0f }, { 1.0f,1.0f,1.0f });


	//// パーティクル初期化
	auto* pm = ParticleManager::GetInstance();
	pm->Initialize(directXCom, srvManager, object3dCom);
	//// テクスチャ粒子グループ作成
	pm->CreateParticleGroup("default", "Resources/uvChecker.png");
	//// ParticleEmitter( グループ名 , 発生位置 , 1回の発生数 , 発生間隔(秒) )
	//emitterBillboard_ = new ParticleEmitter("default", { 0, -2, 0 }, 3, 0.20f);
	//// メッシュ粒子グループ作成
	pm->CreateParticleGroupFromModel("apple", "apple.obj");
	pm->CreateParticleGroupFromModel("bomb", "bomb.obj");
	pm->CreateParticleGroupFromModel("player", "player.obj");
	//emitterMesh_ = new ParticleEmitter("defaultMesh", { 0, -2, 0 }, 4, 0.20f);


#ifdef _DEBUG
	assert(player_Model_);
	assert(apple_Model_);
	assert(bomb_Model_);
	assert(wall_Model_);
#endif
	GenerateWalls(object3dCom, modelWall, { 1.0f,1.0f,1.0f });
	player_ = new Player();

	bool playerSpawned = false;

	uint32_t sx, sy;
	if (mapChipField_->TryGetPlayerSpawnIndex(sx, sy))
	{
		auto pos = mapChipField_->GetMapChipPositionByIndex(sx, sy);
		player_->SetPosition(pos);
	}

	for (uint32_t y = 0; y < mapChipField_->GetNumBlockVirtical(); ++y)
	{
		for (uint32_t x = 0; x < mapChipField_->GetNumBlockHorizontal(); ++x)
		{
			MapChipType type = mapChipField_->GetMapChipTypeByIndex(x, y);
			Vector3 pos = mapChipField_->GetMapChipPositionByIndex(x, y);

			if (type == MapChipType::kPlayerSpawn)
			{
				player_->Initialize(player_Model_, { camera }, pos, object3dCom); // ← posを使う
				playerSpawned = true;
			} else if (type == MapChipType::kAppleSpawn)
			{
				//記入しない
			} else if (type == MapChipType::kBombSpawn)
			{
				//記入しない
			}
		}
	}

	// 初期ボムは1つだけにして、以降はゲーム進行に合わせて増やす
	SpawnOneBomb();

	// 動的リンゴの初期化
	appleTouchCount_ = 0;
	apples_.clear();
	SpawnOneApple(); // 最初の1個を生成

	Player_Eat_Apple_SoundData_ = soundManager->SoundLoadWave("Resources/Audio/SE/player_eat.wav");
	assert(&Player_Eat_Apple_SoundData_);
	assert(soundManager);

	for (auto* apple : apples_)
	{
		apple->UpdateAABB();
	}

	// 初期状態で衝突しているリンゴがないか確認し、あればRespawnさせる
	for (auto* apple : apples_)
	{
		// PlayerとAppleのAABBが衝突しているかチェック
		if (IsCollisionAABBAABB(player_->GetAABB(), apple->GetAABB()))
		{
			// 衝突していたら、ランダムで新しい安全な位置にリスポーン
			apple->Respawn(mapChipField_, *player_);
			// 座標変更を即時反映（UpdateAABBの呼び出しに備える）
			apple->Update();


		}
	}


	// --- マップチップ用の波エフェクト ---
	tileWaveEffect_ = std::make_unique<TileWaveEffect>();
	tileWaveEffect_->Initialize(mapChipField_, player_, &WorldTransformWalls_);

	// --- 俳優用の波エフェクト ---
	actorWave_ = std::make_unique<ActorWaveEffect>();
	actorWave_->Initialize(mapChipField_);

	// Apples（見た目演出が必要なときだけ登録）
	for (auto* a : apples_)
	{
		if (!a) continue;
		// 必要なら登録（Bombは登録しない）
		actorWave_->Add(a->GetObject3d(), [a](const Vector3& p)
			{
				a->SetPosition(p);
				a->UpdateAABB();
			});
	}

	// Bombs は ActorWaveEffect に登録しない（初期表示が消えるのを防止）

	// TileWaveEffectと同時に波演出を開始
	actorWave_->SetPerCellDelay(0.06f);
	actorWave_->SetPopDuration(0.28f);

	// ★波の余韻を短くする設定
	actorWave_->SetWaveAmpRatio(0.06f);
	actorWave_->SetWaveFreq(3.5f);
	actorWave_->SetWaveDamp(4.5f);

	introLock_ = true;

	actorWave_->Start();

	fade_.Initialize(spriteCom);
	fade_.Start(FadeState::In, 60.0f);

	// 葉っぱ
	leafEffect_.Initialize(object3dCom);

	GameBGM_ = soundManager->SoundLoadWave("Resources/Audio/BGM/GameScene.wav");
}

void GameScene::Update()
{

	fade_.Update();

	if (!bgmStarted_ && fade_.IsEnd())
	{
		soundManager->SoundPlayWave(GameBGM_, true);
		bgmStarted_ = true;
	}

	if (backgroundSprite)
	{
		backgroundSprite->Update();
	}

	if (clearCondition_)
	{
		clearCondition_->Update();
	}

	if (bodySeparation)
	{
		bodySeparation->Update();
	}

	poseExplanation->Update();


	// Effect は「ポーズ後」に更新するように移動（ここでは呼ばない）
	// Effect::Tick(1.0f / 60.0f);
	// Effect は「ポーズ後」に更新するように移動（ここでは呼ばない）
// Effect::Tick(1.0f / 60.0f);
	if (gameOver_)
	{
		deathEffect_.Update(1.0f / 60.0f);

		Matrix4x4 view = camera->GetViewMatrix();
		Matrix4x4 proj = camera->GetProjectionMatrix();
		ParticleManager::GetInstance()->Update(view, proj);

		if (deathEffect_.IsFinished())
		{
			resultMode_ = true;
			gameOver_ = false;
			return;
		}
		return;
	}

	// GameScene::Update() の冒頭～ポーズ判定の近くに追加
	if (resultMode_)
	{
		if (overlayResult_)   overlayResult_->Update();
		if (overlayResult_)   overlayResult_->Update();
		if (resultLabelApple_) resultLabelApple_->Update();
		if (resultLabelWall_)  resultLabelWall_->Update();
		// Space でステージセレクトに戻る
		if (keyInput.TriggerKey(DIK_SPACE) || keyInput.TriggerPadButton(XINPUT_GAMEPAD_A))
		{
			// 既存のポーズメニューで「セレクトに戻る」時と同じ遷移を踏襲
			finished_ = true;
			selectSelect_ = true;  // ※既存コードで使っている「セレクト遷移」フラグ
			return;
		}

		// リザルト中は演出用スプライトだけ更新（必要なものだけ）
		if (overlay) { overlay->Update(); }


		return; // 通常のゲーム更新は止める
	}


	if (!player_->GetIsAlive()) {
		gameOver_ = true;

		Matrix4x4 view = camera->GetViewMatrix();
		Matrix4x4 proj = camera->GetProjectionMatrix();
		ParticleManager::GetInstance()->Update(view, proj);


		// 既定が BurstAll なので SetMode は不要（Sequentialにしたい時だけ SetMode を呼ぶ）
		// deathEffect_.SetMode(DeathParticle::Mode::BurstAll);

		deathEffect_.Start(mapChipField_, player_);
		return;
	}




	if (!player_)
	{
		// プレイヤー未生成/破棄状態の安全対策
		return;
	}



	if (keyInput.TriggerKey(DIK_R) || keyInput.TriggerPadButton(XINPUT_GAMEPAD_B))
	{
		Reset();
		return;
	}

	camera->Update();

	// === Pause handling (A or SPACE) ===
	const bool spaceDown = keyInput.IsKeyPressed(DIK_SPACE);
	const bool aDown = keyInput.IsPadButtonPressed(XINPUT_GAMEPAD_A);
	if (!paused_)
	{
		if (spaceDown || aDown)
		{
			spaceHoldSec_ += 1.0f / 60.0f; // 固定フレームの場合
			if (spaceHoldSec_ >= pauseHoldThreshold_)
			{
				paused_ = true;
				spaceHoldSec_ = 0.0f; // 次のレジューム入力のためにリセット
				// ポーズに入ったらメニューを先頭に
				pauseMenuIndex_ = 0; // 0: ゲームに戻る
				// 初期色
				if (overlay) overlay->SetColor({ 1.0f, 1.0f, 1.0f, 0.9f });
				if (backGamePlay) backGamePlay->SetColor({ 1,1,1,1 });
				if (reset) reset->SetColor({ 1,1,1,1 });
				if (backSelectScene) backSelectScene->SetColor({ 1,1,1,1 });
			}
		} else
		{
			spaceHoldSec_ = 0.0f;
		}
	} else
	{
		// ポーズ中は A か SPACE のトリガーで復帰 or 決定
		// 上下で選択移動
		bool upTrig = keyInput.TriggerKey(DIK_UP) || keyInput.TriggerKey(DIK_W) || keyInput.TriggerPadButton(XINPUT_GAMEPAD_DPAD_UP);
		bool downTrig = keyInput.TriggerKey(DIK_DOWN) || keyInput.TriggerKey(DIK_S) || keyInput.TriggerPadButton(XINPUT_GAMEPAD_DPAD_DOWN);

		if (upTrig)
		{
			pauseMenuIndex_ = (pauseMenuIndex_ + 3 - 1) % 3; // 0..2 巻き戻し
		} else if (downTrig)
		{
			pauseMenuIndex_ = (pauseMenuIndex_ + 1) % 3;
		}

		// ハイライト（選択中を少し明るく/他を半透明）
		auto applyHighlight = [&](int idx, Sprite* s)
			{
				if (!s) return;
				if (pauseMenuIndex_ == idx) s->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
				else s->SetColor({ 0.7f, 0.7f, 0.7f, 0.7f });
			};
		// 0: ゲームに戻る, 1: リセット, 2: スクリーンに戻る
		applyHighlight(0, backGamePlay);
		applyHighlight(1, reset);
		applyHighlight(2, backSelectScene);

		// 決定
		if (keyInput.TriggerPadButton(XINPUT_GAMEPAD_A) || keyInput.TriggerKey(DIK_SPACE))
		{
			if (pauseMenuIndex_ == 0)
			{
				// ゲームに戻る
				paused_ = false;
				spaceHoldSec_ = 0.0f;
				if (player_) { player_->SuppressCutOnce(); }
			} else if (pauseMenuIndex_ == 1)
			{
				// リセット
				paused_ = false; // リセット後は動かす
				Reset();
			} else if (pauseMenuIndex_ == 2)
			{
				// スクリーン（セレクト）に戻る
				finished_ = true; // mainの遷移でTitleになる仕様なので、セレクトへ戻るフラグもセット
				selectSelect_ = true;
			}
		}

		// ポーズ中のスプライト更新
		if (overlay) { overlay->Update(); }
		if (reset) { reset->Update(); }
		if (backGamePlay) { backGamePlay->Update(); }
		if (backSelectScene) { backSelectScene->Update(); }

		return; // 以降のゲーム更新を止める
	}

	// ポーズで止めた後にエフェクトの更新を行う（ゲーム時間と同步）
	Effect::Tick(1.0f / 60.0f);

	// Player更新の前 or 直後など
	scaleBounce_.Update(1.0f / 60.0f);


	//ModelのUpdate
	for (uint32_t i = 0; i < WorldTransformWalls_.size(); ++i)
	{
		for (uint32_t j = 0; j < WorldTransformWalls_[i].size(); ++j)
		{
			if (WorldTransformWalls_[i][j])
			{
				WorldTransformWalls_[i][j]->Update();
			}
		}
	}

	//呼ばないと小さくならない (共有モデルの更新は必要)
	player_Model_->Update();

	// ★修正2: 共有モデルであるapple_Model_のUpdateを削除
	// apple_Model_->Update(); // 削除

	bomb_Model_->Update();

	//ここまで

	// パーティクル更新
	Matrix4x4 view = camera->GetViewMatrix();
	Matrix4x4 proj = camera->GetProjectionMatrix();
	ParticleManager::GetInstance()->Update(view, proj);

	const float dt = 1.0f / 60.0f;
	if (emitterBillboard_) emitterBillboard_->Update(dt);
	if (emitterMesh_) emitterMesh_->Update(dt);

	if (tileWaveEffect_ && !tileWaveEffect_->IsFinished())
	{
		tileWaveEffect_->Update(1.0f / 60.0f);
	}
	// 既に tileWaveEffect_ を Update している所の近くで
	if (actorWave_) actorWave_->Update(1.0f / 60.0f);

	if (introLock_)
	{
		const bool tileFinished = !tileWaveEffect_ || tileWaveEffect_->IsFinished();
		// ActorWaveにIsFinished()があるならそれを使用
		const bool actorFinished = /* actorWave_->IsFinished() */ true;
		if (tileFinished && actorFinished)
		{
			introLock_ = false;
		}
	}

	// 壁のUpdate
	for (uint32_t y = 0; y < WorldTransformWalls_.size(); ++y)
	{
		for (uint32_t x = 0; x < WorldTransformWalls_[y].size(); ++x)
		{
			if (WorldTransformWalls_[y][x])
			{
				// reinterpret_castでObject3dに戻してUpdate呼ぶ
				reinterpret_cast<Object3d*>(WorldTransformWalls_[y][x])->Update();
			}
		}
	}

	//本体のUpdate処理

	// プレイヤーのUpdate	
	if (!introLock_)
	{
		// ここで初めてプレイヤーと各オブジェクトのロジックを回す
		player_->Update();
	}

	move->Update();


	for (auto* apple : apples_)
	{
		apple->Update();
	}

	for (auto* bomb : bombs_)
	{
		bomb->Update();
	}


	//当たり判定
	//AppleとPlayerの当たり判定
	bool needSpawnExtraApple = false;
	// ★追加：重なり解消後に発火させるためのキュー
	struct AppleSpawnFx { Apple* a; float dur; float start; float amp; };
	std::vector<AppleSpawnFx> appleSpawnFxQueue;

	for (auto* apple : apples_)
	{
		if (!apple)
			continue;

		if (!apple->IsAlive())
		{
			apple->Respawn(mapChipField_, *player_);
			// ★修正ポイント: apple_Model_->Update()を削除済み
			apple->Update();
			continue;
		}

		apple->Update();

		if (IsCollisionAABBAABB(player_->GetAABB(), apple->GetAABB()))
		{
			// リンゴを取った直後の処理群の中に追加
			applesCollected_++;   // ★取った数を加算

			player_->Grow();

			// ★ここで演出呼び出し（ピックアップ演出はその場でOK）
			Effect::PlayApplePickup(
				ParticleManager::GetInstance(),
				apple->GetPosition(),
				player_->GetScalePtr(),
				0.05f, 0.18f, 1.28f
			);

			// ★追加：このフレームで見た目スケールを即時反映（ポーズのタイミングによるズレ防止）
			if (player_Model_)
			{
				player_Model_->SetScale(*player_->GetScalePtr()); // 念のため同値をセット
				player_Model_->Update();
			}

			apple->Respawn(mapChipField_, *player_);

			// ★修正ポイント: apple_Model_->Update()を削除済み
			apple->Update();

			// ★変更：スポーン演出は重なり解消後にまとめて再生するため、ここではキューに積む
			appleSpawnFxQueue.push_back({ apple, 0.40f, 0.0f, 1.0f });

			// 触れた回数をカウントし、最大数までリンゴを増やす
			appleTouchCount_++;
			needSpawnExtraApple = true;

			soundManager->SoundPlayWave(Player_Eat_Apple_SoundData_);

			// ボムの段階的追加：一定数のリンゴ獲得毎に1つ追加
			if (static_cast<int>(bombs_.size()) < kMaxBombs_ && (appleTouchCount_ % kApplesPerBomb_ == 0))
			{
				SpawnOneBomb();
			}
			continue;
		}
	}


	// ループ外で追加スポーン（イテレータ無効化を避ける）
	if (needSpawnExtraApple && static_cast<int>(apples_.size()) < kMaxApples_)
	{
		int spawnCount = 1; // 複数出したいなら 2,3... に
		while (spawnCount-- > 0 && static_cast<int>(apples_.size()) < kMaxApples_)
		{
			SpawnOneApple();

			// いま出した "最後のリンゴ" に演出を掛けるのは重なり解消後にするため、キューに積む
			if (!apples_.empty())
			{
				Apple* justSpawned = apples_.back();
				if (justSpawned && justSpawned->GetObject3d())
				{
					appleSpawnFxQueue.push_back({ justSpawned, 0.30f, 0.0f, 0.18f });
				}
			}
		}
	}

	// Bombs update/collision（1回だけ実行する正しいループ）
	for (auto it = bombs_.begin(); it != bombs_.end(); )
	{
		Bomb* bomb = *it;
		if (!bomb) { ++it; continue; }

		// 非アクティブなら即リスポーンを試みる
		if (!bomb->IsAlive())
		{
			bomb->Respawn(mapChipField_, *player_);
			bomb->Update(); // 行列・AABBを即反映
			++it;
			continue;
		}

		bomb->Update();

		// デバッグ用：ボムがプレイヤーに衝突したら即死させる
#ifdef _DEBUG

#endif

		// ボムがプレイヤーに衝突したらプレイヤーに飲み込まれる
		if (IsCollisionAABBAABB(player_->GetAABB(), bomb->GetAABB()))
		{
			player_->EatBomb();

			// 削除せずランダムリスポーン
			bomb->Respawn(mapChipField_, *player_);
			bomb->Update(); // 行列・AABBを即反映
			// ★出現演出：スケールバウンス（重め）
			scaleBounce_.Play(bomb->GetObject3d(), 0.40f, 0.0f, 0.7f);

			ParticleManager::GetInstance()->EmitBurst8(
				"bomb", bomb->GetPosition(), 0.12f, 0.40f, 0.6f);
		}
		++it;
	}

	// 安全チェックしてから呼ぶ（NULLポインタ逆参照防止）
	if (mapChipField_ && player_)
	{
		ResolveAppleBombOverlap(mapChipField_, *player_, apples_, bombs_);
	}

	// ★ここで最終位置が確定したので、スポーン演出を再生
	for (const auto& fx : appleSpawnFxQueue)
	{
		if (!fx.a || !fx.a->GetObject3d()) { continue; }
		scaleBounce_.Play(fx.a->GetObject3d(), fx.dur, fx.start, fx.amp);
		ParticleManager::GetInstance()->EmitBurst8(
			"apple", fx.a->GetPosition(), 0.12f, 0.20f, 0.4f);
	}

	//WallとPlayerの当たり判定
	for (uint32_t y = 0; y < mapChipField_->GetNumBlockVirtical(); ++y)
	{
		for (uint32_t x = 0; x < mapChipField_->GetNumBlockHorizontal(); ++x)
		{
			MapChipType type = mapChipField_->GetMapChipTypeByIndex(x, y);
			if (static_cast<int>(type) == 1)
			{ // マップチップ番号1
				Vector3 chipPos = mapChipField_->GetMapChipPositionByIndex(x, y);

				// マップチップのAABBを生成
				float width = mapChipField_->GetBlockWidth();
				float height = mapChipField_->GetBlockHeight();
				AABB WallAABB;
				WallAABB.min = { chipPos.x - width / 2.0f, chipPos.y - height / 2.0f, chipPos.z - width / 2.0f };
				WallAABB.max = { chipPos.x + width / 2.0f, chipPos.y + height / 2.0f, chipPos.z + width / 2.0f };

				// AABB同士の当たり判定
				if (IsCollisionAABBAABB(player_->GetAABB(), WallAABB))
				{
					player_->SetIsAlive(false);
				}
			}
		}
	}

	// 葉っぱ
	leafEffect_.Update();

	if (CheckClearFilledInside())
	{
		cleared_ = true;
		finished_ = true;
		return;
	}

	//デバック用シーン切り替え(titleに行く)
#ifdef _DEBUG
	if (keyInput.TriggerKey(DIK_ESCAPE))
	{
		finished_ = true;
	}

#endif

#ifdef _DEBUG
	//if (keyInput.TriggerKey(DIK_SPACE))
	//{
	//	ParticleManager::GetInstance()->EmitBurst8(
	//		"default", player_->GetPosition(), 0.1f, 1.0f, 0.6f);

	//	// 1回だけ8方向に「回転しながら」飛び散る
	//// 原点を中心に、半径0スタート→毎秒 1.2 ずつ広がり、角速度3rad/sで渦巻き
	//// 原点中心、半径 3.0 から内へ収束、全員 反時計回り、炎の向きは接線方向
	//	ParticleManager::GetInstance()->EmitBurst8RotatingInward(
	//		"default",
	//		{ 0.0f, 0.0f, 0.0f },  // 中心
	//		3.0f,                // startRadius（外側スタート）
	//		1.0f,                // life [s]
	//		3.0f,               // angularVel（＋でCCW、−でCW）
	//		1.0f,               // scale
	//		2.5f,                // radialSpeedAbs（毎秒1.2だけ半径が縮む）
	//		0.0f                 // radialAccelAbs（必要なら正の値で更に内向き加速）
	//	);
	//}
#endif
	// 桁分解ヘルパ（リンゴ側で定義済みならそれを再利用）
	auto split3 = [](int v, int(&out)[3]) -> int {
		v = (std::max)(0, v);
		if (v == 0) { out[0] = 0; return 1; }
		int buf[10]; int n = 0;
		while (v > 0 && n < 10) { buf[n++] = v % 10; v /= 10; }
		int count = (std::min)(n, 3);
		for (int i = 0; i < count; ++i) out[i] = buf[count - 1 - i];
		return count;
		};

	// 壁カウンタの見た目更新
	{
		int digits[3];
		int count = split3(wallsPlaced_, digits);
		wallDigitCount_ = count;

		const char* dir = "Resources/number"; // 実配置に合わせて
		for (int i = 0; i < 3; ++i) {
			if (!wallDigits_[i]) continue;

			if (i < count) {
				std::string path = std::string(dir) + "/" + std::to_string(digits[i]) + ".png";
				if (std::filesystem::exists(path)) {
					wallDigits_[i]->SetTextureSrvHandle(
						TextureManager::GetInstance()->GetSrvHandleGPU(path)
					);
				}
				wallDigits_[i]->SetColor({ 1,1,1,1 }); // 表示
				wallDigits_[i]->Update();
			} else {
				wallDigits_[i]->SetColor({ 1,1,1,0 }); // 非表示
			}
		}


	}

	// 既存の split3 ラムダのすぐ下あたりに追加
	{
		int digits[3];
		int count = split3(applesCollected_, digits);
		appleDigitCount_ = count;

		const char* dir = "Resources/number"; // 実配置に合わせて
		for (int i = 0; i < 3; ++i) {
			if (!appleDigits_[i]) continue;

			if (i < count) {
				std::string path = std::string(dir) + "/" + std::to_string(digits[i]) + ".png";
				if (std::filesystem::exists(path)) {
					appleDigits_[i]->SetTextureSrvHandle(
						TextureManager::GetInstance()->GetSrvHandleGPU(path)
					);
				}
				appleDigits_[i]->SetColor({ 1,1,1,1 }); // 表示
				appleDigits_[i]->Update();
			} else {
				appleDigits_[i]->SetColor({ 1,1,1,0 }); // 非表示
			}
		}
	}
	wallsPlaced_ = player_->GetPlacedWallCount();

}

void GameScene::Draw()
{
	if (backgroundSprite)
	{
		backgroundSprite->Draw();
	}

	// 葉っぱ
	leafEffect_.Draw();

	if (bodySeparation)
	{
		bodySeparation->Draw();
	}

	if (clearCondition_)
	{
		clearCondition_->Draw();
	}

	if (player_ && player_Model_)
	{
		player_->Draw();
	}

	move->Draw();
	poseExplanation->Draw();

	for (uint32_t y = 0; y < WorldTransformWalls_.size(); y++)
	{
		for (uint32_t x = 0; x < WorldTransformWalls_[y].size(); x++)
		{
			if (WorldTransformWalls_[y][x])
			{
				WorldTransformWalls_[y][x]->Update();
				WorldTransformWalls_[y][x]->Draw();
			}
		}
	}

	for (auto* apple : apples_)
	{
		apple->Draw();
	}

	for (auto* bomb : bombs_)
	{
		bomb->Draw();
	}

	// パーティクル描画
	ParticleManager::GetInstance()->Draw();

	if (tileWaveEffect_)
	{
		tileWaveEffect_->Draw();
	}

	spriteCom->ApplyCommonRenderState();
	if (gameOver_) {
		deathEffect_.Draw(spriteCom); // 後で点滅や文字表示を追加できる
	}

	if (paused_)
	{
		if (overlay) { overlay->Draw(); }
		if (reset) { reset->Draw(); }
		if (backGamePlay) { backGamePlay->Draw(); }
		if (backSelectScene) { backSelectScene->Draw(); }
	}

	if (resultMode_)
	{
		if (overlayResult_) overlayResult_->Draw();

		// 好きな位置に調整してOK
		const Vector2 appleLabelPos = { 320.0f, 250.0f }; // 「リンゴ食べた！」画像の位置
		const Vector2 wallLabelPos = { 320.0f, 400.0f }; // 「壁作った！」画像の位置

		const Vector2 appleDigitBase = { 520.0f, 230.0f }; // 数字の左上位置
		const Vector2 wallDigitBase = { 520.0f, 380.0f };

		const Vector2 digitScale = { 72.0f, 96.0f }; // 桁サイズ
		const float   digitGap = 0.1f;

		// 見出しスプライト
		if (resultLabelApple_) {
			resultLabelApple_->SetPosition(appleLabelPos);
			resultLabelApple_->SetScale({ 640.0f, 80.0f });
			resultLabelApple_->Update();
			resultLabelApple_->Draw();
		}
		if (resultLabelWall_) {
			resultLabelWall_->SetPosition(wallLabelPos);
			resultLabelWall_->SetScale({ 640.0f, 80.0f });
			resultLabelWall_->Update();
			resultLabelWall_->Draw();
		}



		// 数字（リンゴ）
		for (int i = 0; i < appleDigitCount_; ++i) {
			if (!appleDigits_[i]) continue;
			appleDigits_[i]->SetScale(digitScale);
			appleDigits_[i]->SetPosition({
				appleDigitBase.x + i * (digitScale.x + digitGap),
				appleDigitBase.y
				});
			appleDigits_[i]->SetColor({ 1,1,1,1 });
			appleDigits_[i]->Update();
			appleDigits_[i]->Draw();
		}

		// 数字（壁）
		for (int i = 0; i < wallDigitCount_; ++i) {
			if (!wallDigits_[i]) continue;
			wallDigits_[i]->SetScale(digitScale);
			wallDigits_[i]->SetPosition({
				wallDigitBase.x + i * (digitScale.x + digitGap),
				wallDigitBase.y
				});
			wallDigits_[i]->SetColor({ 1,1,1,1 });
			wallDigits_[i]->Update();
			wallDigits_[i]->Draw();
		}
	}

	fade_.Draw();
}

void GameScene::GenerateWalls(Object3dCom* object3dCom, Model* model, const Vector3& wallScale)
{
	uint32_t height = mapChipField_->GetNumBlockVirtical();
	uint32_t width = mapChipField_->GetNumBlockHorizontal();

	WorldTransformWalls_.resize(height);
	for (auto& row : WorldTransformWalls_)
	{
		row.resize(width, nullptr);
	}

	for (uint32_t y = 0; y < height; y++)
	{
		for (uint32_t x = 0; x < width; x++)
		{
			MapChipType type = mapChipField_->GetMapChipTypeByIndex(x, y);
			if (type == MapChipType::kWall)
			{
				Vector3 pos = mapChipField_->GetMapChipPositionByIndex(x, y);

				// 渡されたスケールで各インスタンスを生成
				Object3d* wall = makeObject(object3dCom, model, pos, wallScale);
				WorldTransformWalls_[y][x] = wall;
			}
		}
	}
}


void GameScene::LoadAndGetModel(Model*& outModel, const std::string& filePath)
{
	auto* mm = ModelManager::GetInstance();
	mm->LoadModel(filePath);
	outModel = mm->FindModel(filePath);

}



Object3d* GameScene::makeObject(Object3dCom* object3dCom, Model* model, const Vector3& pos, const Vector3& scale)
{
	auto* obj = new Object3d();
	obj->Initialize(object3dCom);
	obj->SetModel(model);
	obj->SetTranslate(pos);
	obj->SetScale(scale);
	return obj;
}


bool GameScene::CheckClearFilledInside() const
{
	if (!mapChipField_ || !player_) { return false; }

	const int H = static_cast<int>(mapChipField_->GetNumBlockVirtical());
	const int W = static_cast<int>(mapChipField_->GetNumBlockHorizontal());

	auto key = [](int x, int y) -> long long
		{
			return (static_cast<long long>(y) << 32) | static_cast<unsigned int>(x);
		};

	// 非壁セル総数を数える
	int fillableCount = 0;
	for (int y = 0; y < H; ++y)
	{
		for (int x = 0; x < W; ++x)
		{
			if (mapChipField_->GetMapChipTypeByIndex(x, y) != MapChipType::kWall)
			{
				++fillableCount;
			}
		}
	}

	// Playerの占有セル（頭・体）
	std::unordered_set<long long> occupied;
	occupied.reserve(128);

	// 注意: Player::GetOccupiedGridPositions の座標系がMapのインデックスと一致している前提。
	// もし上下反転などがある場合は、ここでH-1-gy等の補正を入れてください。
	for (const auto& g : player_->GetOccupiedGridPositions())
	{
		int gx = g.first;
		int gy = g.second;
		if (0 <= gx && gx < W && 0 <= gy && gy < H)
		{
			// 非壁のみカウント対象
			if (mapChipField_->GetMapChipTypeByIndex(gx, gy) != MapChipType::kWall)
			{
				occupied.insert(key(gx, gy));
			}
		}
	}

	// Playerが生成した壁も占有として扱う
	for (const auto& g : player_->GetWallGridPositions())
	{
		int gx = g.first;
		int gy = g.second;
		if (0 <= gx && gx < W && 0 <= gy && gy < H)
		{
			if (mapChipField_->GetMapChipTypeByIndex(gx, gy) != MapChipType::kWall)
			{
				occupied.insert(key(gx, gy));
			}
		}
	}

	// すべての非壁セルを占有していればクリア
	// なお、非壁セルにApple/Bombがいても、Playerが占有していなければ未充足として残ります。
	return static_cast<int>(occupied.size()) >= fillableCount;
}



void GameScene::Reset()
{
	// 軽量リセット：重いリソースは破棄・再ロードしない

	// フラグ・状態の初期化
	finished_ = false;
	cleared_ = false;
	paused_ = false;
	gameOver_ = false;
	spaceHoldSec_ = 0.0f;
	pauseMenuIndex_ = 0;
	selectSelect_ = false;
	started_ = false;
	resultMode_ = false;

	if (overlayResult_) {

		overlayResult_->SetScale({ 1.05f, 1.05f });
	}

	// カメラを初期姿勢へ戻す
	if (camera)
	{
		camera->SetTranslate({ 10.0f, -17.0f, -42.0f });
		camera->SetRotate({ -30.0f * 3.141592f / 180.0f, 0.0f, 0.0f });
		camera->Update();
	}

	// プレイヤーを初期スポーンへ配置し直す（モデルは再利用）
	if (player_)
	{
		delete player_;
		player_ = nullptr;
	}
	player_ = new Player();

	Vector3 spawnPos{ 0.0f, 0.0f, 0.0f };
	bool foundSpawn = false;
	if (mapChipField_)
	{
		for (uint32_t y = 0; y < mapChipField_->GetNumBlockVirtical() && !foundSpawn; ++y)
		{
			for (uint32_t x = 0; x < mapChipField_->GetNumBlockHorizontal() && !foundSpawn; ++x)
			{
				if (mapChipField_->GetMapChipTypeByIndex(x, y) == MapChipType::kPlayerSpawn)
				{
					spawnPos = mapChipField_->GetMapChipPositionByIndex(x, y);
					foundSpawn = true;
					break;
				}
			}
		}
	}
	player_->Initialize(player_Model_, camera, spawnPos, object3dCom);
	player_->UpdateAABB();

	// 既存のApple/Bombを削除せず、位置だけ再配置
	for (auto* apple : apples_)
	{
		if (!apple) continue;
		apple->SetAlive(true);
		apple->Respawn(mapChipField_, *player_);
		// ★修正ポイント: apple_Model_->Update()を削除済み
		apple->Update();
	}
	for (auto* bomb : bombs_)
	{
		if (!bomb) continue;
		bomb->SetAlive(true);
		bomb->Respawn(mapChipField_, *player_);
		bomb->Update();
	}

	// プレイヤーとの初期衝突を解消
	for (auto* apple : apples_)
	{
		if (!apple) continue;
		if (IsCollisionAABBAABB(player_->GetAABB(), apple->GetAABB()))
		{
			apple->Respawn(mapChipField_, *player_);
			// ★修正ポイント: apple_Model_->Update()を削除済み
			apple->Update();
		}
	}

	// Apple と Bomb の重複も解消
	ResolveAppleBombOverlap(mapChipField_, *player_, apples_, bombs_);
}

void GameScene::Finalize()
{
	if (emitterBillboard_) { delete emitterBillboard_; emitterBillboard_ = nullptr; }
	if (emitterMesh_) { delete emitterMesh_; emitterMesh_ = nullptr; }
	ParticleManager::GetInstance()->Finalize();

	delete camera;
#ifdef _DEBUG
	delete debugCamera_;
#endif

	for (auto* apple : apples_)
	{
		delete apple;
	}
	apples_.clear();

	for (auto* bomb : bombs_)
	{
		delete bomb;
	}
	bombs_.clear();

	for (auto& row : WorldTransformWalls_)
	{
		for (auto* obj : row)
		{
			delete obj;
		}
	}
	WorldTransformWalls_.clear();

	delete player_;
	delete player_Model_;
	delete apple_Model_;
	delete bomb_Model_;
	delete wall_Model_;
	delete mapChipField_;
	delete uvChecker;
}


void GameScene::ResolveAppleBombOverlap(MapChipField* map, const Player& player,
	std::vector<Apple*>& apples,
	std::vector<Bomb*>& bombs)
{
	if (!map) return;

	const int w = static_cast<int>(map->GetNumBlockHorizontal());
	const int h = static_cast<int>(map->GetNumBlockVirtical());

	// セルごとの前回の勝者を記憶（フレーム間で安定化）
	static std::unordered_map<long long, const void*> stickyOwner; // cellKey -> itemPtr

	auto cellKey = [](int gx, int gy) -> long long
		{
			return (static_cast<long long>(gy) << 32) | static_cast<unsigned int>(gx);
		};

	auto findGrid = [&](const Vector3& pos, int& outX, int& outY) -> bool
		{
			for (int y = 0; y < h; ++y)
			{
				for (int x = 0; x < w; ++x)
				{
					Vector3 p = map->GetMapChipPositionByIndex(x, y);
					if (p.x == pos.x && p.y == pos.y && p.z == pos.z)
					{
						outX = x; outY = y;
						return true;
					}
				}
			}
			return false;
		};

	// 1) 空きセル（壁×, プレイヤー×）候補を列挙
	std::vector<std::pair<int, int>> freeCells;
	freeCells.reserve(static_cast<size_t>(w) * h);
	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			if (map->GetMapChipTypeByIndex(x, y) == MapChipType::kWall) continue;
			const int gyBottom = h - 1 - y; // 既存座標系に合わせる
			if (player.IsOccupyingGrid(x, gyBottom)) continue;
			freeCells.emplace_back(x, y);
		}
	}
	if (freeCells.empty()) return;

	// 2) 現在配置の占有状況を収集
	struct ItemRef { bool isApple; void* ptr; int gx; int gy; };
	std::vector<ItemRef> items; items.reserve(apples.size() + bombs.size());
	std::unordered_map<long long, int> counts;

	auto pushItem = [&](bool isApple, void* ptr, const Vector3& pos)
		{
			int gx = -1, gy = -1;
			if (!findGrid(pos, gx, gy)) return;
			items.push_back({ isApple, ptr, gx, gy });
			counts[cellKey(gx, gy)]++;
		};

	for (auto* a : apples)
	{
		if (a && a->IsAlive()) pushItem(true, static_cast<void*>(a), a->GetPosition());
	}
	for (auto* b : bombs)
	{
		if (b && b->IsAlive()) pushItem(false, static_cast<void*>(b), b->GetPosition());
	}

	// 重複がなければ何もしない
	bool hasOverlap = false;
	for (auto& kv : counts) { if (kv.second > 1) { hasOverlap = true; break; } }
	if (!hasOverlap)
	{
		// stickyOwner を最新に更新しておく（将来の安定化のため）
		stickyOwner.clear();
		for (const auto& it : items)
		{
			stickyOwner[cellKey(it.gx, it.gy)] = it.ptr;
		}
		return;
	}

	// 3) freeCells から「現在占有されている全セル（ユニーク＋重複）」を除外
	std::unordered_set<long long> occupiedKeys; occupiedKeys.reserve(counts.size());
	for (auto& kv : counts) occupiedKeys.insert(kv.first);

	freeCells.erase(std::remove_if(freeCells.begin(), freeCells.end(),
		[&](const std::pair<int, int>& c)
		{
			return occupiedKeys.count(cellKey(c.first, c.second)) > 0;
		}), freeCells.end());

	// 以降で使う空きが無ければ、ここで打ち切り（その場に留める）
	if (freeCells.empty())
	{
		// stickyOwner を最新の占有で再構築
		stickyOwner.clear();
		for (const auto& it : items)
		{
			stickyOwner[cellKey(it.gx, it.gy)] = it.ptr;
		}
		return;
	}

	// 4) 重複セルごとにグルーピングして、勝者と敗者を決める
	std::unordered_map<long long, std::vector<size_t>> groups; // key -> indices of items
	for (size_t i = 0; i < items.size(); ++i)
	{
		long long k = cellKey(items[i].gx, items[i].gy);
		if (counts[k] > 1) groups[k].push_back(i);
	}

	std::vector<size_t> losers;
	losers.reserve(items.size());

	auto ptrValue = [](const void* p) -> uintptr_t { return reinterpret_cast<uintptr_t>(p); };

	for (auto& g : groups)
	{
		long long k = g.first;
		auto& idxs = g.second;

		// 既知のオーナーがいれば優先
		const void* keepPtr = nullptr;
		auto itOwner = stickyOwner.find(k);
		if (itOwner != stickyOwner.end())
		{
			// 同じ個体がいればそれを残す
			for (size_t idx : idxs)
			{
				if (items[idx].ptr == itOwner->second)
				{
					keepPtr = items[idx].ptr;
					break;
				}
			}
		}
		// 見つからなければ、ポインタ値が最小の個体を勝者に（安定で偏りが少ない）
		if (!keepPtr)
		{
			size_t best = idxs.front();
			for (size_t idx : idxs)
			{
				if (ptrValue(items[idx].ptr) < ptrValue(items[best].ptr)) best = idx;
			}
			keepPtr = items[best].ptr;
		}

		// 敗者を収集
		for (size_t idx : idxs)
		{
			if (items[idx].ptr != keepPtr) losers.push_back(idx);
		}
	}

	// 5) 敗者を空きセルへ移動（空きが尽きたら動かさずに留める）
	std::random_device rd;
	std::mt19937 rng(rd());
	std::shuffle(freeCells.begin(), freeCells.end(), rng);
	size_t freeIndex = 0;

	auto moveItem = [&](ItemRef& it, int nx, int ny)
		{
			Vector3 newPos = map->GetMapChipPositionByIndex(nx, ny);
			if (it.isApple)
			{
				auto* a = static_cast<Apple*>(it.ptr);
				a->SetPosition(newPos);
				a->Update(); // 行列/AABB即反映
			} else
			{
				auto* b = static_cast<Bomb*>(it.ptr);
				b->SetPosition(newPos);
				b->Update(); // 行列/AABB即反映
			}
			it.gx = nx; it.gy = ny;
		};

	for (size_t loserIdx : losers)
	{
		if (freeIndex >= freeCells.size())
		{
			// 空きが無い
			continue;
		}
		auto [nx, ny] = freeCells[freeIndex++];
		moveItem(items[loserIdx], nx, ny);
	}

	// 6) stickyOwner を最新の占有で再構築（次フレームの安定化用）
	stickyOwner.clear();
	for (const auto& it : items)
	{
		stickyOwner[cellKey(it.gx, it.gy)] = it.ptr;
	}
}

// 1個のリンゴを安全に生成して追加
void GameScene::SpawnOneApple()
{
	if (!mapChipField_ || !player_ || !apple_Model_) return;

	const int w = static_cast<int>(mapChipField_->GetNumBlockHorizontal());
	const int h = static_cast<int>(mapChipField_->GetNumBlockVirtical());

	// 候補セル列挙（壁×、プレイヤー×、既存リンゴ×、爆弾×）
	std::vector<std::pair<int, int>> candidates;
	candidates.reserve(static_cast<size_t>(w) * static_cast<size_t>(h));

	auto posEquals = [](const Vector3& a, const Vector3& b) -> bool
		{
			return a.x == b.x && a.y == b.y && a.z == b.z;
		};

	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			if (mapChipField_->GetMapChipTypeByIndex(x, y) == MapChipType::kWall) continue;
			const int gyBottom = h - 1 - y;
			if (player_->IsOccupyingGrid(x, gyBottom)) continue;

			Vector3 pos = mapChipField_->GetMapChipPositionByIndex(x, y);

			bool overlap = false;
			for (const auto* a : apples_) { if (a && posEquals(a->GetPosition(), pos)) { overlap = true; break; } }
			if (overlap) continue;
			for (const auto* b : bombs_) { if (b && posEquals(b->GetPosition(), pos)) { overlap = true; break; } }
			if (overlap) continue;

			candidates.emplace_back(x, y);
		}
	}

	if (candidates.empty())
	{
		// フォールバック：とりあえずプレイヤー以外の非壁に出す
		for (int y = 0; y < h; ++y)
		{
			for (int x = 0; x < w; ++x)
			{
				if (mapChipField_->GetMapChipTypeByIndex(x, y) == MapChipType::kWall) continue;
				candidates.emplace_back(x, y);
			}
		}
		if (candidates.empty()) return;
	}

	std::random_device rd;
	std::mt19937 rng{ rd() };
	std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
	auto [gx, gy] = candidates[dist(rng)];
	Vector3 pos = mapChipField_->GetMapChipPositionByIndex(static_cast<uint32_t>(gx), static_cast<uint32_t>(gy));

	// 生成
	// モデルはapple_Model_から取得
	Object3d* appleObj = makeObject(object3dCom, apple_Model_->GetModel(), pos, { 0.5f, 0.5f, 0.5f });
	Apple* apple = new Apple();
	apple->Initialize(appleObj, camera, pos);
	apple->UpdateAABB();
	apples_.push_back(apple);

	// ★修正ポイント: apple_Model_->Update()を削除済み

	// ActorWaveEffect へは、初期演出が不要なら登録しない
	if (!introLock_ && actorWave_ && apple && apple->GetObject3d())
	{
		Apple* a = apple;
		actorWave_->Add(a->GetObject3d(), [a](const Vector3& p)
			{
				a->SetPosition(p);
				a->UpdateAABB();
			});
	}
}

// 1個のボムを安全に生成して追加
void GameScene::SpawnOneBomb()
{
	if (!mapChipField_ || !player_ || !bomb_Model_) return;

	const int w = static_cast<int>(mapChipField_->GetNumBlockHorizontal());
	const int h = static_cast<int>(mapChipField_->GetNumBlockVirtical());

	// 候補セル列挙（壁×、プレイヤー×、既存リンゴ×、既存爆弾×）
	std::vector<std::pair<int, int>> candidates;
	candidates.reserve(static_cast<size_t>(w) * static_cast<size_t>(h));

	auto posEquals = [](const Vector3& a, const Vector3& b) -> bool
		{
			return a.x == b.x && a.y == b.y && a.z == b.z;
		};

	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			if (mapChipField_->GetMapChipTypeByIndex(x, y) == MapChipType::kWall) continue;
			const int gyBottom = h - 1 - y;
			if (player_->IsOccupyingGrid(x, gyBottom)) continue;

			Vector3 pos = mapChipField_->GetMapChipPositionByIndex(x, y);

			bool overlap = false;
			for (const auto* a : apples_) { if (a && posEquals(a->GetPosition(), pos)) { overlap = true; break; } }
			if (overlap) continue;
			for (const auto* b : bombs_) { if (b && posEquals(b->GetPosition(), pos)) { overlap = true; break; } }
			if (overlap) continue;

			candidates.emplace_back(x, y);
		}
	}

	// Apple同様のフォールバック：候補が空なら非壁セルから出す（重なりは後で解消）
	if (candidates.empty())
	{
		for (int y = 0; y < h; ++y)
		{
			for (int x = 0; x < w; ++x)
			{
				if (mapChipField_->GetMapChipTypeByIndex(x, y) == MapChipType::kWall) continue;
				candidates.emplace_back(x, y);
			}
		}
		if (candidates.empty()) return;
	}

	std::random_device rd;
	std::mt19937 rng{ rd() };
	std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
	auto [gx, gy] = candidates[dist(rng)];
	Vector3 pos = mapChipField_->GetMapChipPositionByIndex(static_cast<uint32_t>(gx), static_cast<uint32_t>(gy));

	// 生成
	Object3d* bombObj = makeObject(object3dCom, bomb_Model_->GetModel(), pos, { 1.0f, 1.0f, 1.0f });
	Bomb* bomb = new Bomb();
	bomb->Initialize(bombObj, camera, pos);
	bomb->UpdateAABB();
	bombs_.push_back(bomb);

	// Bomb は ActorWaveEffect に登録しない（見た目が消えるのを防止）

	// 出現演出
	scaleBounce_.Play(bomb->GetObject3d(), 0.40f, 0.0f, 0.7f);
	ParticleManager::GetInstance()->EmitBurst8(
		"bomb", bomb->GetPosition(), 0.12f, 0.40f, 0.6f);
}