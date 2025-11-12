#include"Tutorial.h"
#include<cassert>
#include <algorithm>
#include <random>
#include <unordered_set>
#include <cmath>
#include <filesystem>

extern Object3dCom* object3dCom;
extern SpriteCom* spriteCom;
extern KeyInput keyInput; // revert to non-pointer extern to match main.cpp definition
extern SoundManager* soundManager;
extern DirectXCom* directXCom;
extern SrvManager* srvManager;

// ヘルパー: 数値を桁ごとに分解（最大3桁）
static void SplitDigits(int value, int (&out)[3], int& outCount) {
    value = (std::max)(0, value);
    if (value == 0) {
        out[0] = 0; outCount = 1; return;
    }
    int buf[10]; int n = 0;
    while (value > 0 && n < 10) { buf[n++] = value % 10; value /= 10; }
    outCount = (std::min)(n, 3);
    for (int i = 0; i < outCount; ++i) { out[i] = buf[outCount - 1 - i]; }
}

Tutorial::~Tutorial()
{
	if (emitterBillboard_) { delete emitterBillboard_; emitterBillboard_ = nullptr; }
	if (emitterMesh_) { delete emitterMesh_;      emitterMesh_ = nullptr; }
	ParticleManager::GetInstance()->Finalize();

	delete camera;
#ifdef _DEBUG
	delete debugCamera_;
#endif

	for (auto* apple : apples_)
	{
		delete apple;
	}

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

	delete backgroundSprite;
	delete bodySeparation;
	delete tutorialEatApple;
	delete reset;
	delete backGamePlay;
	delete backSelectScene;
	delete eatApple_;
	delete bombCollide;
	delete bodySeparationTutorial;
	delete spaceOrAred;
	delete gameOverExplanation;
	delete poseExplanation;
	delete clearText;
	delete move;
	delete overlay;


	for (auto*& s : countDigits_) { delete s; s = nullptr; }

	soundManager->SoundUnload(&tutorialBGM_);
}

void Tutorial::Initialize(Object3dCom* object3dCom, SpriteCom* spriteCom)
{
#ifdef _DEBUG

	debugCamera_ = new DebugCamera();
	debugCamera_->Initialize();
	assert(debugCamera_);

#endif

#ifdef _DEBUG

#endif
	//カメラ
	camera = new Camera();
	camera->SetTranslate({ 10.0f, -17.0f, -42.0f });
	camera->SetRotate({ -30.0f * 3.141592f / 180.0f, 0.0f, 0.0f });
#ifdef _DEBUG	
	assert(camera);
#endif
	//絶対Cameraの後
	object3dCom->SetDefaultCamera(camera);

	mapChipField_ = new MapChipField();
	mapChipField_->Initialize();
	mapChipField_->LoadmapChipCsv("Resources/map/Block.csv");

	backgroundSprite = new Sprite();
	const std::string bgPath = "Resources/background.png"; // 画面解像度と同サイズ推奨
	TextureManager::GetInstance()->LoadTexture(bgPath);
	backgroundSprite->Initialize(spriteCom, bgPath);
	backgroundSprite->SetPosition({ 0.0f, 0.0f });

	bodySeparation = new Sprite();
	const std::string bodySeparationPath = "Resources/bodySeparation.png";
	TextureManager::GetInstance()->LoadTexture(bodySeparationPath);
	bodySeparation->Initialize(spriteCom, bodySeparationPath);
	bodySeparation->SetPosition({ 20.0f, 150.0f });

	tutorialEatApple = new Sprite();
	const std::string tutorialEatApplePath = "Resources/tutorialEatApple.png";
	TextureManager::GetInstance()->LoadTexture(tutorialEatApplePath);
	tutorialEatApple->Initialize(spriteCom, tutorialEatApplePath);
	tutorialEatApple->SetPosition({ 400.0f, 650.0f });

	eatApple_ = new Sprite();
	const std::string eatApplePath = "Resources/eatApple.png";
	TextureManager::GetInstance()->LoadTexture(eatApplePath);
	eatApple_->Initialize(spriteCom, eatApplePath);
	eatApple_->SetPosition({ 320.0f, 180.0f });
	// デフォルトの表示サイズを明示（必要に応じて変更可能）
	eatApple_->SetScale({ 640.0f, 256.0f });

	bombCollide = new Sprite();
	const std::string bombCollidePath = "Resources/bombCollide.png";
	TextureManager::GetInstance()->LoadTexture(bombCollidePath);
	bombCollide->Initialize(spriteCom, bombCollidePath);
	bombCollide->SetPosition({ 320.0f, 180.0f });
	bombCollide->SetScale({ 640.0f, 256.0f });

	bodySeparationTutorial = new Sprite();
	const std::string bodySeparationTutorialPath = "Resources/bodySeparationTutorial.png";
	TextureManager::GetInstance()->LoadTexture(bodySeparationTutorialPath);
	bodySeparationTutorial->Initialize(spriteCom, bodySeparationTutorialPath);
	bodySeparationTutorial->SetPosition({ 370.0f, 168.0f });
	bodySeparationTutorial->SetScale({ 540.0f, 384.0f });

	spaceOrAred = new Sprite();
	const std::string spaceOrAredPath = "Resources/spaceOrAred.png";
	TextureManager::GetInstance()->LoadTexture(spaceOrAredPath);
	spaceOrAred->Initialize(spriteCom, spaceOrAredPath);
	spaceOrAred->SetPosition({ 320.0f, 350.0f });

	gameOverExplanation = new Sprite();
	const std::string gameOverExplanationPath = "Resources/gameOverExplanation.png";
	TextureManager::GetInstance()->LoadTexture(gameOverExplanationPath);
	gameOverExplanation->Initialize(spriteCom, gameOverExplanationPath);
	gameOverExplanation->SetPosition({ 320.0f, 180.0f });
	gameOverExplanation->SetScale({ 640.0f, 256.0f });


	clearText = new Sprite();
        const std::string clearTextPath =
            "Resources/tutorialClear.png";
        TextureManager::GetInstance()->LoadTexture(clearTextPath);
        clearText->Initialize(spriteCom, clearTextPath);
        clearText->SetPosition({320.0f, 180.0f});
        clearText->SetScale({640.0f, 256.0f});


	poseExplanation = new Sprite();
	const std::string poseExplanationPath = "Resources/poseExplanation.png";
	TextureManager::GetInstance()->LoadTexture(poseExplanationPath);
	poseExplanation->Initialize(spriteCom, poseExplanationPath);
	poseExplanation->SetPosition({ 950.0f, 100.0f });

	move = new Sprite();
	const std::string movePath = "Resources/move.png";
	TextureManager::GetInstance()->LoadTexture(movePath);
	move->Initialize(spriteCom, movePath);
	move->SetPosition({ 10.0f, 50.0f });

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

	{
		const char* candidateDirs[] = {
			"Resources/number",   // 実配置フォルダ（小文字）
			"Resources/Numbers",
			"Resources/Number",
			"Resources/Textures/Numbers",
			"Resources/Textures",
			"Resources"
		};
		for (int n = 0; n <= 9; ++n) {
			bool loaded = false;
			for (const char* dir : candidateDirs) {
				std::string redPath = std::string(dir) + "/" + std::to_string(n) + "red.png";
				if (std::filesystem::exists(redPath)) {
					TextureManager::GetInstance()->LoadTexture(redPath);
					loaded = true;
					break; // 見つかったら次の数字へ
				}
			}
			// 念のため、redが無いときは通常版を読み込んでおく
			if (!loaded) {
				for (const char* dir : candidateDirs) {
					std::string normalPath = std::string(dir) + "/" + std::to_string(n) + ".png";
					if (std::filesystem::exists(normalPath)) {
						TextureManager::GetInstance()->LoadTexture(normalPath);
						break;
					}
				}
			}
		}
	}

	// カウント表示用の桁スプライト作成（最大3桁）: 初期テクスチャは 0red.png（無ければ0.png）
	{
		std::string zeroRedPath = "Resources/number/0red.png";
		std::string zeroPath = std::filesystem::exists(zeroRedPath)
			? zeroRedPath
			: std::string("Resources/number/0.png");
		for (auto*& s : countDigits_) {
			s = new Sprite();
			s->Initialize(spriteCom, zeroPath);
		}
		// 初期配置とサイズ（左上に小さく）
		Vector2 basePos{ 20.0f, 20.0f };
		Vector2 digitSize{ 48.0f, 64.0f };
		for (int i = 0; i < 3; ++i) {
			countDigits_[i]->SetPosition({ basePos.x + i * (digitSize.x + 4.0f), basePos.y });
			countDigits_[i]->SetScale(digitSize);
		}
	}

	Model* modelPlayer = nullptr;
	LoadAndGetModel(modelPlayer, "player.obj");
	player_Model_ = makeObject(object3dCom, modelPlayer, { 0.0f, 0.0f, 0.0f }, { 1.0f,1.0f,1.0f });

	Model* modelApple = nullptr;
	LoadAndGetModel(modelApple, "apple.obj");
	apple_Model_ = makeObject(object3dCom, modelApple, { 0.5f, 0.5f, 0.5f }, { 1.0f,1.0f,1.0f });

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
	pm->CreateParticleGroupFromModel("defaultMesh", "apple.obj");
	pm->CreateParticleGroupFromModel("apple", "apple.obj");
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
				player_->SetBombStepTime(3.0f);
				playerSpawned = true;
			}
			else if (type == MapChipType::kAppleSpawn)
			{
				//記入しない
			}
			else if (type == MapChipType::kBombSpawn)
			{
				//記入しない
			}
		}
	}

	std::random_device rd;
	std::mt19937 gen(rd());
	// カメラの位置をグリッドインデックスに変換
	int camX = static_cast<int>(camera->GetTranslate().x / mapChipField_->GetBlockWidth());
	int camY = static_cast<int>(camera->GetTranslate().y / mapChipField_->GetBlockHeight());

	// 範囲外なら中央に補正
	camX = std::clamp(camX, 0, static_cast<int>(mapChipField_->GetNumBlockHorizontal()) - 1);
	camY = std::clamp(camY, 0, static_cast<int>(mapChipField_->GetNumBlockVirtical()) - 1);

	int minX = (std::max)(0, camX - 5);
	int maxX = (std::min)(static_cast<int>(mapChipField_->GetNumBlockHorizontal()) - 1, camX + 6);
	int minY = (std::max)(0, camY - 5);
	int maxY = (std::min)(static_cast<int>(mapChipField_->GetNumBlockVirtical()) - 1, camY + 5);

	std::uniform_int_distribution<int> distX(minX, maxX);
	std::uniform_int_distribution<int> distY(minY, maxY);


	//Appleの生成
	for (int i = 0; i < 10; ++i)
	{
		int xIndex, yIndex;
		while (true)
		{
			xIndex = distX(gen);
			yIndex = distY(gen);
			if (mapChipField_->GetMapChipTypeByIndex(xIndex, yIndex) != MapChipType::kWall)
			{
				Vector3 pos = mapChipField_->GetMapChipPositionByIndex(xIndex, yIndex);
				if (!(pos.x == player_->GetPosition().x && pos.y == player_->GetPosition().y && pos.z == player_->GetPosition().z))
				{
					bool overlap = false;
					for (const auto* apple : apples_)
					{
						const auto& aPos = apple->GetPosition();
						if (aPos.x == pos.x && aPos.y == pos.y && aPos.z == pos.z)
						{
							overlap = true;
							break;
						}
					}
					for (const auto* bomb : bombs_)
					{
						const auto& bPos = bomb->GetPosition();
						if (bPos.x == pos.x && bPos.y == pos.y && bPos.z == pos.z)
						{
							overlap = true;
							break;
						}
					}
					if (!overlap)
					{
						// ここを修正
						Object3d* appleObj = makeObject(object3dCom, modelApple, pos, { 0.5f, 0.5f, 0.5f });
						Apple* apple = new Apple();
						apple->Initialize(appleObj, camera, pos);
						apples_.push_back(apple);
						break;
					}
				}
			}
		}
	}



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

	// Apples
	for (auto* a : apples_)
	{
		if (!a) continue;
		actorWave_->Add(a->GetObject3d(), [a](const Vector3& p)
			{
				a->SetPosition(p);
				a->UpdateAABB();
			});
	}

	// Bombs（必要なら）
	for (auto* b : bombs_)
	{
		if (!b) continue;
		actorWave_->Add(b->GetObject3d(), [b](const Vector3& p)
			{
				b->SetPosition(p);
				b->UpdateAABB();
			});
	}

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

	//葉っぱ
    leafEffect_.Initialize(object3dCom);

	Player_Eat_Apple_SoundData_ = soundManager->SoundLoadWave("Resources/Audio/SE/player_eat.wav");
	decideSE_ = soundManager->SoundLoadWave("Resources/Audio/SE/decision.wav");
	tutorialBGM_ = soundManager->SoundLoadWave("Resources/Audio/BGM/Tutorial.wav");
#ifdef _DEBUG
	assert(&Player_Eat_Apple_SoundData_);
	assert(&decideSE_);
	assert(soundManager);
	assert(&tutorialBGM_);
#endif // _DEBUG

	// 遅延スポーン用カウンタ初期化
	bombInitialCount_ = 5; // チュートリアルでは5個でフェードに入る
	applesTouchedCount_ = 0;
	bombsSpawned_ = false;
	bombSpawnedThisFrame_ = false;
	bombRevealAwaitSpace_ = false;
	suppressTutorialUI_ = false;
	initialTutorialLocked_ = false; // 初期チュートリアルのロックフラグ
}

void Tutorial::Update()
{

	// フレーム先頭でワンショットフラグをクリア
	bombSpawnedThisFrame_ = false;

	// フェード更新→BGM（重複防止のため一度だけ再生）
	fade_.Update();
	if (!bgmStarted_ && fade_.IsEnd())
	{
		soundManager->SoundPlayWave(tutorialBGM_, true);
		bgmStarted_ = true;
	}

	// フェード中はプレイヤーの移動を停止、終了したら解除
	if (player_)
	{
		if (hasDetachedOnce_) {
			player_->SetMovementLocked(true); // 切り離し後は常に停止
		}
		else if (!fade_.IsEnd()) { player_->SetMovementLocked(true); }
		else { player_->SetMovementLocked(false); }
	}

	if (hasDetachedOnce_) {
          if (keyInput.TriggerKey(DIK_SPACE) ||
			  keyInput.TriggerPadButton(XINPUT_GAMEPAD_A)) {
            if (isChange_) {
              cleared_ = true;
              
            } else {
              isChange_ = true;
            }
		  }      
	}

	// 共通の中間フェード進行（死亡リセット以外）
	if (midFadeActive_ && midFadeReason_ != MidFadeReason::DeathReset)
	{
		if (midFadePhase_ == MidFadePhase::Out && fade_.IsEnd())
		{
			// しきい到達時: リセットのみ。BGMは維持
			Reset();
			midFadePhase_ = MidFadePhase::In;
			fade_.Start(FadeState::In, midFadeDuration_);
		}
		else if (midFadePhase_ == MidFadePhase::In && fade_.IsEnd())
		{
			// フェード明け（爆弾生成時）に告知モードへ入る
			if (midFadeReason_ == MidFadeReason::BombSpawn)
			{
				bombRevealAwaitSpace_ = true;
				// 初期チュートリアルはここで完全ロック
				suppressTutorialUI_ = true;
				initialTutorialLocked_ = true;
			}
			midFadeActive_ = false;
			midFadePhase_ = MidFadePhase::None;
			midFadeReason_ = MidFadeReason::None;
		}
	}

	// === Pause handling (A or SPACE) ===
	const bool spaceDown = keyInput.IsKeyPressed(DIK_SPACE);
	const bool aDown = keyInput.IsPadButtonPressed(XINPUT_GAMEPAD_A);

	// 爆弾告知中はポーズ開始を抑止
	if (!paused_ && !bombRevealAwaitSpace_) {
		if (spaceDown || aDown) {
			spaceHoldSec_ += 1.0f / 60.0f; // 固定フレームならこれでOK
			if (spaceHoldSec_ >= pauseHoldThreshold_) {
				paused_ = true;
				spaceHoldSec_ = 0.0f;
				// ポーズに入ったらメニュー先頭に
				pauseMenuIndex_ = 0;
				// 初期色をリセット
				if (overlay)        overlay->SetColor({ 1,1,1,0.9f });
				if (backGamePlay)   backGamePlay->SetColor({ 1,1,1,1 });
				if (reset)          reset->SetColor({ 1,1,1,1 });
				if (backSelectScene)backSelectScene->SetColor({ 1,1,1,1 });
			}
		} else {
			spaceHoldSec_ = 0.0f;
		}
	} else if (paused_) {
		// 上下移動（キーボード / パッドの十字）
		bool upTrig = keyInput.TriggerKey(DIK_UP) || keyInput.TriggerKey(DIK_W)
			|| keyInput.TriggerPadButton(XINPUT_GAMEPAD_DPAD_UP);
		bool downTrig = keyInput.TriggerKey(DIK_DOWN) || keyInput.TriggerKey(DIK_S)
			|| keyInput.TriggerPadButton(XINPUT_GAMEPAD_DPAD_DOWN);

		if (upTrig)        pauseMenuIndex_ = (pauseMenuIndex_ + 3 - 1) % 3; // 0..2 巻き戻し
		else if (downTrig) pauseMenuIndex_ = (pauseMenuIndex_ + 1) % 3;

		// ハイライト（選択中だけ不透明、その他は半透明）
		auto applyHighlight = [&](int idx, Sprite* s) {
			if (!s) return;
			if (pauseMenuIndex_ == idx) s->SetColor({ 1.0f,1.0f,1.0f,1.0f });
			else                        s->SetColor({ 0.7f,0.7f,0.7f,0.7f });
			};
		applyHighlight(0, backGamePlay);
		applyHighlight(1, reset);
		applyHighlight(2, backSelectScene);

		// 決定（A / SPACE のトリガー）※長押しでなく「押した瞬間」
		if (keyInput.TriggerPadButton(XINPUT_GAMEPAD_A) || keyInput.TriggerKey(DIK_SPACE)) {
			if (pauseMenuIndex_ == 0) {
				// ゲームに戻る
				paused_ = false;
				spaceHoldSec_ = 0.0f;
				if (player_) { player_->SuppressCutOnce(); }
			} else if (pauseMenuIndex_ == 1) {
				// リセット（復帰してから実行）
				paused_ = false;
				Reset();
			} else if (pauseMenuIndex_ == 2) {
				// セレクトへ戻る（Tutorial では finished_ を立てる）
				finished_ = true;
				return; // 以降の更新不要
			}
		}

		// ポーズ中もスプライトの Update は回す（色変更などを反映）
		if (overlay)         overlay->Update();
		if (backGamePlay)    backGamePlay->Update();
		if (reset)           reset->Update();
		if (backSelectScene) backSelectScene->Update();

		// ポーズ中はゲーム進行の更新をここで止めたい場合は適宜 return してください
		return;
	}


	if (cleared_)
	{
		if (!fadeStarted_)
		{
			fade_.Start(FadeState::Out, 60.0f); // 1秒で黒
			fadeStarted_ = true;

		}

		// フェードアウトが終わったらシーン終了
		if (fadeStarted_ && fade_.IsEnd())
		{
			finished_ = true;
			return;
		}
	}

	

	// === 死亡中（ゲームオーバー演出中）は通常更新を止める ===
	
	if (gameOver_)
	{
		// DeathParticle とパーティクルだけ動かす
		deathEffect_.Update(1.0f / 60.0f);

		Matrix4x4 view = camera->GetViewMatrix();
		Matrix4x4 proj = camera->GetProjectionMatrix();
		ParticleManager::GetInstance()->Update(view, proj);

		// --- 演出が終わったらフェードアウト開始（まだ gameOver_ は下ろさない）---
		if (deathEffect_.IsFinished() && !midFadeActive_) {
			midFadeActive_ = true;
			midFadePhase_ = MidFadePhase::Out;
			midFadeReason_ = MidFadeReason::DeathReset;
			fade_.Start(FadeState::Out, midFadeDuration_);
		}

		// --- フェードの進行をここで回す ---
		if (midFadeActive_) {
			// フェードOUT完了 → Reset 実行 → gameOver_ をここで false にする
			if (midFadePhase_ == MidFadePhase::Out && fade_.IsEnd()) {
				Reset();                       // ここでプレイヤーが復活 = IsAlive() が true になる
				midFadePhase_ = MidFadePhase::In;
				fade_.Start(FadeState::In, midFadeDuration_);

				gameOver_ = false;             // ★Reset直後に下ろす（再トリガ防止の肝）
			}
			// フェードIN完了 → フェーズ終了
			else if (midFadePhase_ == MidFadePhase::In && fade_.IsEnd()) {
				midFadeActive_ = false;
				midFadePhase_ = MidFadePhase::None;
				midFadeReason_ = MidFadeReason::None;
			}
		}

		return; // 以降の通常更新を停止
	}



	// 死亡時もフェード演出を挟んでからResetする
	// 死亡時もフェード演出を挟んでからResetする
	if (!player_->GetIsAlive() && !gameOver_) // ★二重起動ガード
	{
		gameOver_ = true;

		// パーティクルは停止中でも進むように、ビュー射影だけ反映
		Matrix4x4 view = camera->GetViewMatrix();
		Matrix4x4 proj = camera->GetProjectionMatrix();
		ParticleManager::GetInstance()->Update(view, proj);

		// DeathParticle 開始（既定は一気に砕け散る）
		deathEffect_.SetOrder(DeathEffect::Order::HeadToTail); // ←クラス名: DeathParticle
		deathEffect_.Start(mapChipField_, player_);

		return; // このフレームは停止
	}


	if (keyInput.TriggerKey(DIK_R) || keyInput.TriggerPadButton(XINPUT_GAMEPAD_B))
	{
		Reset();
		return;
	}

	// ここからは通常のポーズ制御だが、爆弾リビール中はポーズ操作を無視する
	camera->Update();
	if (!paused_ && !bombRevealAwaitSpace_)
	{
		if (spaceDown || aDown)
		{
			spaceHoldSec_ += 1.0f / 60.0f;
			if (spaceHoldSec_ >= pauseHoldThreshold_)
			{
				paused_ = true;
				spaceHoldSec_ = 0.0f;
			}
		}
		else
		{
			spaceHoldSec_ = 0.0f;
		}
	}
	else if (paused_)
	{
		if (keyInput.TriggerPadButton(XINPUT_GAMEPAD_A) || keyInput.TriggerKey(DIK_SPACE))
		{
			paused_ = false;
			spaceHoldSec_ = 0.0f;
			if (player_) { player_->SuppressCutOnce(); }
		}
	}

	// 爆弾出現告知中はSPACE/Aで解除し、それ以外の更新は止める（UIだけ更新）
	if (bombRevealAwaitSpace_)
	{
		if (keyInput.TriggerKey(DIK_SPACE) || keyInput.TriggerPadButton(XINPUT_GAMEPAD_A))
		{
			bombRevealAwaitSpace_ = false; // 解除
			suppressTutorialUI_ = false;    // UIの抑止は解除するが
			soundManager->SoundPlayWave(decideSE_);
			// initialTutorialLocked_ が true のため初期チュートリアルは復帰しない
			// 解除と同時にゲームを再開（SPACE 一回で再度動く）
			started_ = true;
		}

		// UI更新のみ
		if (reset) { reset->Update(); }
		if (backGamePlay) { backGamePlay->Update(); }
		if (backSelectScene) { backSelectScene->Update(); }
		return;
	}

	// カウントUI更新（applesTouchedCount_ に合わせてテクスチャ差し替え）
	{
		int remaining = (std::max)(0, bombInitialCount_ - applesTouchedCount_);
		if (remaining <= 0) {
			currentDigitCount_ = 0;
			for (int i = 0; i < 3; i++) { countDigits_[i]->SetColor({1,1,1,0}); }
		} else {
			int digits[3]; int count = 0; SplitDigits(remaining, digits, count);
			currentDigitCount_ = count;
			const char* dir = "Resources/number";
			for (int i = 0; i < 3; ++i) {
				if (i < count) {
					std::string redPath = std::string(dir) + "/" + std::to_string(digits[i]) + "red.png";
					std::string normalPath = std::string(dir) + "/" + std::to_string(digits[i]) + ".png";
					if (std::filesystem::exists(redPath)) {
						countDigits_[i]->SetTextureSrvHandle(TextureManager::GetInstance()->GetSrvHandleGPU(redPath));
					} else if (std::filesystem::exists(normalPath)) {
						countDigits_[i]->SetTextureSrvHandle(TextureManager::GetInstance()->GetSrvHandleGPU(normalPath));
					}
					countDigits_[i]->SetColor({1,1,1,1});
				} else {
					countDigits_[i]->SetColor({1,1,1,0});
				}
			}
		}
	}

	Effect::Tick(1.0f / 60.0f);
	scaleBounce_.Update(1.0f / 60.0f);

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

	player_Model_->Update();
	apple_Model_->Update();
	bomb_Model_->Update();
	
	leafEffect_.Update();

	Matrix4x4 view = camera->GetViewMatrix();
	Matrix4x4 proj = camera->GetProjectionMatrix();
	ParticleManager::GetInstance()->Update(view, proj);

	const float dt = 1.0f / 60.0f;
	if (emitterBillboard_) emitterBillboard_->Update(dt);
	if (emitterMesh_)      emitterMesh_->Update(dt);

	if (tileWaveEffect_ && !tileWaveEffect_->IsFinished())
	{
		tileWaveEffect_->Update(1.0f / 60.0f);
	}
	if (actorWave_) actorWave_->Update(1.0f / 60.0f);

	if (introLock_)
	{
		const bool tileFinished = !tileWaveEffect_ || tileWaveEffect_->IsFinished();
		const bool actorFinished = /* actorWave_->IsFinished() */ true;
		if (tileFinished && actorFinished)
		{
			introLock_ = false;
		}
	}

	for (uint32_t y = 0; y < WorldTransformWalls_.size(); ++y)
	{
		for (uint32_t x = 0; x < WorldTransformWalls_[y].size(); ++x)
		{
			if (WorldTransformWalls_[y][x])
			{
				reinterpret_cast<Object3d*>(WorldTransformWalls_[y][x])->Update();
			}
		}
	}

	if (!cleared_ && started_)
	{
		if (!introLock_)
		{
			player_->Update();
		}
	}

	// 追加: チュートリアルでプレイヤーが初めて体を切り離したかを検出
	if (player_ && !hasDetachedOnce_ && player_->HasDetachedThisFrame())
	{
		hasDetachedOnce_ = true;
		// 必要ならば: player_->ClearDetachedThisFrame(); // フラグの消費
	}

	for (auto* apple : apples_)
	{
		apple->Update();
	}

	for (auto* bomb : bombs_)
	{
		bomb->Update();
	}


	struct AppleSpawnFx { Apple* a; float dur; float start; float amp; };
	std::vector<AppleSpawnFx> appleSpawnFxQueue;

	for (auto* apple : apples_)
	{
		if (!apple)
			continue;

		if (!apple->IsAlive())
		{
			apple->Respawn(mapChipField_, *player_);
			apple->Update();
			continue;
		}

		apple->Update();

		// リンゴ取得時の処理
		if (IsCollisionAABBAABB(player_->GetAABB(), apple->GetAABB()))
		{
			player_->Grow();

			Effect::PlayApplePickup(
				ParticleManager::GetInstance(),
				apple->GetPosition(),
				player_->GetScalePtr(),
				0.05f, 0.18f, 1.28f
			);

			if (player_Model_)
			{
				player_Model_->SetScale(*player_->GetScalePtr());
				player_Model_->Update();
			}

			applesTouchedCount_++;

			// 爆弾未出現のときだけフェード演出を開始する
			if (!bombsSpawned_ && applesTouchedCount_ >= bombInitialCount_ && !midFadeActive_ && midFadeReason_ == MidFadeReason::None)
			{
				midFadeActive_ = true;
				midFadePhase_ = MidFadePhase::Out;
				midFadeReason_ = MidFadeReason::BombSpawn;
				fade_.Start(FadeState::Out, midFadeDuration_);
				// ここでチュートリアルUIを抑止＋初期チュートリアルを永続ロック
				suppressTutorialUI_ = true;
				initialTutorialLocked_ = true;
			}

			apple->Respawn(mapChipField_, *player_);
			apple->Update();

			appleSpawnFxQueue.push_back({ apple, 0.40f, 0.0f, 1.0f });

			soundManager->SoundPlayWave(Player_Eat_Apple_SoundData_);

			if (applesTouchedCount_ >= 5)
			{
				hasEatenMultipleApples_ = true;
			}

			continue;
		}
	}

	// 念のため、リンゴループ外でも5個到達を監視してフェード開始（保険）
	if (!bombsSpawned_ && applesTouchedCount_ >= bombInitialCount_ && !midFadeActive_ && midFadeReason_ == MidFadeReason::None)
	{
		midFadeActive_ = true;
		midFadePhase_ = MidFadePhase::Out;
		midFadeReason_ = MidFadeReason::BombSpawn;
		fade_.Start(FadeState::Out, midFadeDuration_);
		// 告知中は初期チュートリアルUIを抑止しロック
		suppressTutorialUI_ = true;
		initialTutorialLocked_ = true;
	}

	for (auto it = bombs_.begin(); it != bombs_.end();)
	{
		Bomb* bomb = *it;
		if (!bomb)
		{
			++it;
			continue;
		}

		if (!bomb->IsAlive())
		{
			bomb->Respawn(mapChipField_, *player_);
			bomb->Update();
			++it;
			continue;
		}

		bomb->Update();

		if (IsCollisionAABBAABB(player_->GetAABB(), bomb->GetAABB()))
		{
			player_->EatBomb();

			if (!bombHitPauseUsed_)
			{
				started_ = false;
				bombHitPauseUsed_ = true;
			}

			bomb->Respawn(mapChipField_, *player_);
			bomb->Update();

			++it;
		}
		else
		{
			++it;
		}
	}


	bodySeparation->Update();
	tutorialEatApple->Update();
	for (int i = 0; i < currentDigitCount_; ++i) { countDigits_[i]->Update(); }
	eatApple_->Update();
	bombCollide->Update();
	bodySeparationTutorial->Update();
	spaceOrAred->Update();
	gameOverExplanation->Update();

    clearText->Update();
	move->Update();
	poseExplanation->Update();

	if (mapChipField_ && player_)
	{
		ResolveAppleBombOverlap(mapChipField_, *player_, apples_, bombs_);
	}

	for (const auto& fx : appleSpawnFxQueue) {
		if (!fx.a || !fx.a->GetObject3d()) { continue; }
		scaleBounce_.Play(fx.a->GetObject3d(), fx.dur, fx.start, fx.amp);
		ParticleManager::GetInstance()->EmitBurst8(
			"apple", fx.a->GetPosition(), 0.12f, 0.20f, 0.4f);
	}

	if (mapChipField_)
	{
		for (uint32_t y = 0; y < mapChipField_->GetNumBlockVirtical(); ++y)
		{
			for (uint32_t x = 0; x < mapChipField_->GetNumBlockHorizontal(); ++x)
			{
				MapChipType type = mapChipField_->GetMapChipTypeByIndex(x, y);
				if (static_cast<int>(type) == 1)
				{
					Vector3 chipPos = mapChipField_->GetMapChipPositionByIndex(x, y);

					float width = mapChipField_->GetBlockWidth();
					float height = mapChipField_->GetBlockHeight();
					AABB WallAABB;
					WallAABB.min = { chipPos.x - width / 2.0f, chipPos.y - height / 2.0f, chipPos.z - width / 2.0f };
					WallAABB.max = { chipPos.x + width / 2.0f, chipPos.y + height / 2.0f, chipPos.z + width / 2.0f };

					if (IsCollisionAABBAABB(player_->GetAABB(), WallAABB))
					{
						player_->SetIsAlive(false);
					}
				}
			}
		}
	}

	if (CheckClearFilledInside())
	{
		cleared_ = true;
		/*finished_ = true;*/
		return;
	}


	if (backgroundSprite)
	{
		backgroundSprite->Update();
	}

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
	//		3.0f,               // angularVel（＋でCCW、−でCW）    1.0fだと遅すぎる
	//		2.0f,               // scale
	//		2.5f,                // radialSpeedAbs（毎秒1.2だけ半径が縮む）
	//		0.0f                 // radialAccelAbs（必要なら正の値で更に内向き加速）
	//	);
	//}
#endif
	if (!started_)
	{
		// 導入の波演出やフェードインが終わるまでは開始不可にする場合は条件を足す
		const bool ready = !introLock_ && fade_.IsEnd();

		// ゲームパッドの A ボタンで開始
		if (ready && keyInput.TriggerPadButton(XINPUT_GAMEPAD_A))
		{
			started_ = true;
		}


		if (ready && keyInput.TriggerKey(DIK_SPACE)) { started_ = true; }

		if (started_)
		{
			soundManager->SoundPlayWave(decideSE_);
		}

		// 背景やモデルの描画更新は続けつつ、ゲームロジックはまだ回さない
		return;
	}
}

void Tutorial::Draw()
{
	if (backgroundSprite)
	{
		backgroundSprite->Draw();
	}

	leafEffect_.Draw();
	bodySeparation->Draw();
	poseExplanation->Draw();
	move->Draw();
	
	


	if (player_ && player_Model_)
	{
		player_->Draw();
	}
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


	// ポーズ時のメニュー描画（オーバーレイ）
	if (paused_)
	{
		if (reset) { reset->Draw(); }
		if (backGamePlay) { backGamePlay->Draw(); }
		if (backSelectScene) { backSelectScene->Draw(); }
	}

	

	// 初期チュートリアルUIは抑止中またはロック済みなら描画しない
	if (!suppressTutorialUI_ && !initialTutorialLocked_)
	{
		if (!started_)
		{
			eatApple_->Draw();
			spaceOrAred->Draw();
		}
		if (applesTouchedCount_ < 5)
		{
			tutorialEatApple->Draw();
			for (int i = 0; i < currentDigitCount_; ++i)
			{
				countDigits_[i]->SetPosition({ 610, 650.0f });
				countDigits_[i]->Draw();
			}
		}
	}

	// 爆弾告知中は専用UIを描画
	if (IsBombRevealActive())
	{
		bombCollide->Draw();
		spaceOrAred->Draw();
	}

	if (!started_ && bombHitPauseUsed_)
	{
		bodySeparationTutorial->Draw();
		spaceOrAred->SetPosition({ 320.0f, 450.0f });
		spaceOrAred->Draw();
	}

	if (hasDetachedOnce_)
	{
          if (isChange_) {
              clearText->Draw();
		  }else {
            gameOverExplanation->Draw();
          }
	}

	// === Pause中のオーバーレイ＆メニュー ===
	if (paused_) {
		if (overlay)         overlay->Draw();
		if (backGamePlay)    backGamePlay->Draw();
		if (reset)           reset->Draw();
		if (backSelectScene) backSelectScene->Draw();
	}
	fade_.Draw();

}

void Tutorial::GenerateWalls(Object3dCom* object3dCom, Model* model, const Vector3& wallScale)
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


void Tutorial::LoadAndGetModel(Model*& outModel, const std::string& filePath)
{
	auto* mm = ModelManager::GetInstance();
	mm->LoadModel(filePath);
	outModel = mm->FindModel(filePath);

}



Object3d* Tutorial::makeObject(Object3dCom* object3dCom, Model* model, const Vector3& pos, const Vector3& scale)
{
	auto* obj = new Object3d();
	obj->Initialize(object3dCom);
	obj->SetModel(model);
	obj->SetTranslate(pos);
	obj->SetScale(scale);
	return obj;
}


bool Tutorial::CheckClearFilledInside() const
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

void Tutorial::Reset()
{
	// 軽量リセット：重いリソースは破棄・再ロードしない

	// フラグ・状態の初期化
	finished_ = false;
	cleared_ = false;
	paused_ = false;
	spaceHoldSec_ = 0.0f;
	started_ = false;
	spaceHoldSec_ = 0.0f;
	bombHitPauseUsed_ = false; // 爆弾衝突による一時停止はリセットで解除

	// 中間フェード状態はここでは触らない。
	// フェードの進行は Update 側の midFadeActive_/phase ロジックで管理する。

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
		apple->Update();
	}
	for (auto* bomb : bombs_)
	{
		if (!bomb) continue;
		bomb->SetAlive(true);
		bomb->Respawn(mapChipField_, *player_);
		bomb->Update();
	}

	// 爆弾は一旦全破棄して、再度5個のリンゴ取得までは出さない
	for (auto* bomb : bombs_)
	{
		delete bomb;
	}
	bombs_.clear();
	bombsSpawned_ = false;
	bombSpawnedThisFrame_ = false; // リセット直後は未出現
	applesTouchedCount_ = 0;

	// プレイヤーとの初期衝突を解消
	for (auto* apple : apples_)
	{
		if (!apple) continue;
		if (IsCollisionAABBAABB(player_->GetAABB(), apple->GetAABB()))
		{
			apple->Respawn(mapChipField_, *player_);
			apple->Update();
		}
	}

	// Apple と Bomb の重複も解消
	ResolveAppleBombOverlap(mapChipField_, *player_, apples_, bombs_);

	// ★条件達成済みなら、このResetのタイミングでBombを生成
	if (hasEatenMultipleApples_ && bomb_Model_ && mapChipField_ && player_)
	{
		// 1個生成（必要に応じて数は調整）
		Object3d* bombObj = makeObject(object3dCom, bomb_Model_->GetModel(), player_->GetPosition(), { 1.0f, 1.0f, 1.0f });
		Bomb* bomb = new Bomb();
		bomb->Initialize(bombObj, camera, player_->GetPosition());
		bomb->Respawn(mapChipField_, *player_); // 安全な空きセルへ移動
		bomb->Update();
		bombs_.push_back(bomb);
		bombsSpawned_ = true;            // 持続フラグON
		bombSpawnedThisFrame_ = true;    // ワンショットON（このフレームのみ）

		// 追加: 死亡リセット直後も爆弾告知UIを再表示する
		bombRevealAwaitSpace_ = true;    // 再びユーザー操作で解除させる
		suppressTutorialUI_ = true;       // 告知中はチュートリアルUIを抑止
		initialTutorialLocked_ = true;   // 初期チュートリアルは引き続き非表示
	}
}

void Tutorial::Finalize()
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
}


void Tutorial::ResolveAppleBombOverlap(MapChipField* map, const Player& player,
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
	std::mt19937 rng{ std::random_device{}() };
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
			}
			else
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

// 追加: eatApple_ スケール操作 API 実装
void Tutorial::SetEatAppleScale(const Vector2& pixelSize)
{
	if (eatApple_)
	{
		eatApple_->SetScale(pixelSize);
	}
}

void Tutorial::SetEatAppleScaleRatio(float ratio)
{
	if (!eatApple_) { return; }
	Vector2 s = eatApple_->GetScale();
	eatApple_->SetScale({ s.x * ratio, s.y * ratio });
}