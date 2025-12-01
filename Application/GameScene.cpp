#include "GameScene.h"
#include "Baziru3_Engine/Particle/ParticleManager.h"
#include "Baziru3_Engine/2D/Sprite/Sprite.h"
#include "Baziru3_Engine/3D/Model/Model.h"
#include "Random.h"

GameScene::~GameScene()
{
	for (Enemy* enemy : enemies_)
	{
		delete enemy;
	}
	enemies_.clear();
	delete player_;
	delete railCameraController_;
	if (fade_) delete fade_;
#ifdef _DEBUG
	delete debugCamera_;
#endif

	if (boss_) delete boss_;
	if (bossBodyModel_) delete bossBodyModel_;
}

void GameScene::Initialize(Camera* camera, Object3dCom* object3dCom, SpriteCom* spriteCom)
{
	object3dCom_ = object3dCom;
	camera_ = camera;
	// カメラの初期化（アスペクト比設定）
	camera_->Initialize();

	keyInput_ = KeyInput::GetInstance();
	spriteCom_ = spriteCom; // store for App particles

#ifdef _DEBUG
	// 画面サイズから DebugCamera を初期化 (幅/高さは DirectXCom 経由で取得する想定)
	float width = static_cast<float>(object3dCom_->GetDirectXCom()->GetClientWidth());
	float height = static_cast<float>(object3dCom_->GetDirectXCom()->GetClientHeight());
	debugCamera_ = new DebugCamera(width, height);
	debugCamera_->Initialize();
#endif

	model_ = Object3d::Create(object3dCom_, "apple.obj", { {1,1,1},{0,0,0},{0,0,0} }, camera);
	Object3d* enemyModelTemplate = Object3d::Create(object3dCom_, "wall.obj", { {1,1,1},{0,0,0},{0,0,0} }, camera);

	player_ = new Player();
	player_->Initialize(model_, camera, { 0.0f,0.0f,0.0f }, object3dCom);

	{
		auto* pm = ParticleManager::GetInstance();
		if (pm) {
			if (model_ && model_->GetModel()) {
				std::string texPath = model_->GetModel()->GetTexturePath();
				if (!texPath.empty()) {
					pm->CreateParticleGroup("default", texPath);
					// store texture path for application sprite particles
					particleTexturePath_ = texPath;
				}

				pm->CreateParticleGroupFromModel("defaultMesh", "apple.obj");
			}
		}
	}

	currentWave_ = 0;
	phase_ = Phase::kMain;
	SpawnWave();

	railCameraController_ = new RailCameraController();
	railCameraController_->SetCamera(camera_);
	railCameraController_->Initialize({ 0.0f, 5.0f, -10.0f }, { 20.0f, 0.0f, 0.0f });

	railCameraController_->SetTarget(player_);


	bossBodyModel_ = nullptr;
	boss_ = nullptr;

	fade_ = new Fade();
	fade_->Initialize(spriteCom);
	fade_->Start(Fade::State::kNone, 1.0f);


	isWaitingForNextWave_ = false;
	waveDelayTimer_ = 0.0f;
}

// Helper: world -> screen (pixels)
static Vector2 WorldToScreen(const Vector3& world, Camera* cam, int screenW, int screenH)
{
	const Matrix4x4& vp = cam->GetViewProjectionMatrix();
	float x = world.x * vp.m[0][0] + world.y * vp.m[1][0] + world.z * vp.m[2][0] + vp.m[3][0];
	float y = world.x * vp.m[0][1] + world.y * vp.m[1][1] + world.z * vp.m[2][1] + vp.m[3][1];
	float z = world.x * vp.m[0][2] + world.y * vp.m[1][2] + world.z * vp.m[2][2] + vp.m[3][2];
	float w = world.x * vp.m[0][3] + world.y * vp.m[1][3] + world.z * vp.m[2][3] + vp.m[3][3];
	if (w == 0.0f) w = 1e-6f;
	float nx = x / w;
	float ny = y / w;
	// NDC -> screen
	float sx = (nx * 0.5f + 0.5f) * static_cast<float>(screenW);
	float sy = (-ny * 0.5f + 0.5f) * static_cast<float>(screenH);
	return { sx, sy };
}

void GameScene::Update()
{
#ifdef _DEBUG
#ifdef USE_IMGUI
	// ImGuiフレーム中 (ImGuiManager::Begin() 呼び出し後) にのみUI描画
	player_->DrawImGui();
#endif

	if (keyInput_->TriggerKey(DIK_F1))
	{
		isDebugCameraActive_ = !isDebugCameraActive_;
	}


#ifdef _DEBUG
	if (keyInput_->TriggerKey(DIK_R))
	{
		ResetScene();
		return;
	}
#endif


	for (Enemy* enemy : enemies_)
	{
		if (enemy) enemy->Update();
	}
	player_->Update();


	const float kEnemyActiveZThreshold = -5.0f;
	bool anyActive = false;
	for (Enemy* enemy : enemies_)
	{
		if (!enemy) continue;
		if (!enemy->IsActive()) continue;
		Vector3 epos = enemy->GetWorldTranslate();
	
		if (epos.z > kEnemyActiveZThreshold) { anyActive = true; break; }
	}
	if (!anyActive && phase_ == Phase::kMain)
	{

		if (currentWave_ + 1 < maxWaves_)
		{

			if (!isWaitingForNextWave_)
			{

				for (Enemy* enemy : enemies_) { delete enemy; }
				enemies_.clear();

				isWaitingForNextWave_ = true;
				waveDelayTimer_ = waveDelay_;
			}
			else
			{
				const float dt = 1.0f / 60.0f;
				waveDelayTimer_ -= dt;
				if (waveDelayTimer_ <= 0.0f)
				{
					isWaitingForNextWave_ = false;
					++currentWave_;
					SpawnWave();
				}
			}
		}
		else
		{
			
			phase_ = Phase::kBoss;
			
			if (!boss_)
			{
				bossBodyModel_ = Object3d::Create(object3dCom_, "bomb.obj", { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,15.0f} }, camera_);
				boss_ = new Boss();
				boss_->Initialize(bossBodyModel_, camera_, { 0.0f, 0.0f, 15.0f }, object3dCom_, spriteCom_);
				boss_->SetPlayer(player_);
			}
		}
	}

	if (isDebugCameraActive_ && debugCamera_)
	{
		debugCamera_->Update();
		// デバッグカメラの行列をメインカメラへコピー
		camera_->OverrideViewProjection(debugCamera_->GetViewMatrix(), debugCamera_->GetProjectionMatrix());
	}
	else
	{
		// 通常のレールカメラ更新（player の更新後に行う）
		railCameraController_->Update();
	}
#else
	// リリース時は通常カメラのみ
	for (Enemy* enemy : enemies_)
	{
		if (enemy) enemy->Update();
	}
	player_->Update();
	railCameraController_->Update();


	const float kEnemyActiveZThreshold = -5.0f;
	bool anyActive = false;
	for (Enemy* enemy : enemies_)
	{
		if (!enemy) continue;
		if (!enemy->IsActive()) continue;
		Vector3 epos = enemy->GetWorldTranslate();
		if (epos.z > kEnemyActiveZThreshold) { anyActive = true; break; }
	}
	if (!anyActive && phase_ == Phase::kMain)
	{

		if (currentWave_ + 1 < maxWaves_)
		{

			if (!isWaitingForNextWave_)
			{
				for (Enemy* e : enemies_) { delete e; }
				enemies_.clear();

				isWaitingForNextWave_ = true;
				waveDelayTimer_ = waveDelay_;
			}
			else
			{
				const float dt = 1.0f / 60.0f;
				waveDelayTimer_ -= dt;
				if (waveDelayTimer_ <= 0.0f)
				{
					isWaitingForNextWave_ = false;
					++currentWave_;
					SpawnWave();
				}
			}
		}
		else
		{

			phase_ = Phase::kBoss;
			if (!boss_)
			{
				bossBodyModel_ = Object3d::Create(object3dCom_, "bomb.obj", { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,15.0f} }, camera_);
				boss_ = new Boss();
				boss_->Initialize(bossBodyModel_, camera_, { 0.0f, 0.0f, 15.0f }, object3dCom_, spriteCom_);
				boss_->SetPlayer(player_);
			}
		}
	}
#endif


	if (boss_)
	{
		boss_->Update();
	
		if (!boss_->IsActive() && phase_ == Phase::kBoss)
		{
			phase_ = Phase::kFadeOut;
			fade_->Start(Fade::State::kFadeOut, 1.0f);
		}
	}

	if (phase_ == Phase::kFadeOut)
	{
		fade_->Update();
		if (fade_->IsFinished())
		{
			isFinish_ = true;
		}
	}

	
	auto* pm = ParticleManager::GetInstance();
	if (pm) {
		pm->Update(camera_->GetViewMatrix(), camera_->GetProjectionMatrix());
	}

	// --- Application sprite particles update ---
	const float dt = 1.0f / 60.0f;
	if (!appParticles_.empty()) {
		int screenW = object3dCom_->GetDirectXCom()->GetClientWidth();
		int screenH = object3dCom_->GetDirectXCom()->GetClientHeight();
		for (size_t i = 0; i < appParticles_.size();) {
			auto &p = appParticles_[i];
			p.age += dt;
			if (p.sprite) {
				// move
				p.pos.x += p.vel.x * dt;
				p.pos.y += p.vel.y * dt;
				p.sprite->SetPosition(p.pos);
				// fade alpha
				float a = 1.0f - (p.age / p.life);
				if (a < 0.0f) a = 0.0f;
				Vector4 c = { 1.0f, 1.0f, 1.0f, a };
				p.sprite->SetColor(c);
				p.sprite->Update();
			}
			if (p.age >= p.life) {
				if (p.sprite) { delete p.sprite; p.sprite = nullptr; }
				// remove
				appParticles_.erase(appParticles_.begin() + i);
			} else {
				++i;
			}
		}
	}

	// --- Application mesh (OBJ) particles update ---
	if (!appMeshParticles_.empty()) {
		for (size_t i = 0; i < appMeshParticles_.size();) {
			auto &mp = appMeshParticles_[i];
			mp.age += dt;
			// move in world
			if (mp.obj) {
				Vector3 cur = mp.obj->GetTranslate();
				cur.x += mp.vel.x * dt;
				cur.y += mp.vel.y * dt;
				cur.z += mp.vel.z * dt;
				mp.obj->SetTranslate(cur);
				// fade alpha on model
				float a = 1.0f - (mp.age / mp.life);
				if (a < 0.0f) a = 0.0f;
				if (mp.model) mp.model->SetColor({1.0f, 1.0f, 1.0f, a});
				// apply transform so it is drawn correctly
				Transform t;
				t.Initialize();
				t.SetScale(mp.obj->GetScale());
				t.SetRotate(mp.obj->GetRotate());
				t.SetTranslate(cur);
				mp.obj->ApplyState(t, camera_, true);
			}
			if (mp.age >= mp.life) {
				if (mp.obj) { delete mp.obj; mp.obj = nullptr; }
				if (mp.model) { delete mp.model; mp.model = nullptr; }
				appMeshParticles_.erase(appMeshParticles_.begin() + i);
			} else {
				++i;
			}
		}
	}

	CheckAllCollisions();
}

void GameScene::Draw()
{

	for (Enemy* enemy : enemies_)
	{
		if (enemy) enemy->Draw();
	}
	player_->Draw();

	if (boss_) boss_->Draw();

		auto* pm = ParticleManager::GetInstance();
	if (pm) {
		pm->Draw();
	}

	// draw application sprite particles
	for (auto &p : appParticles_) {
		if (p.sprite) p.sprite->Draw();
	}

	// draw application mesh particles
	for (auto &mp : appMeshParticles_) {
		if (mp.obj) {
			mp.obj->Draw();
		}
	}

	if (phase_ == Phase::kFadeOut)
	{
		fade_->Draw();
	}
}

void GameScene::CheckAllCollisions()
{
	Vector3 posA, posB;

	// バリア群を取得
	const std::vector<PlayerBarrier*>& barriers = player_->GetBarriers();
	//敵の弾のリスト: aggregate from all enemies + boss
	std::list<EnemyBullet*> enemyBullets;
	for (Enemy* enemy : enemies_)
	{
		if (!enemy) continue;
		const std::list<EnemyBullet*>& enemy_Bullets = enemy->GetBullets();
		for (EnemyBullet* enemy_Bullet : enemy_Bullets)
		{
			enemyBullets.push_back(enemy_Bullet);
		}
	}
	
	if (boss_)
	{
		const std::list<EnemyBullet*>& bossBullets = boss_->GetBullets();
		for (EnemyBullet* b : bossBullets)
		{
			enemyBullets.push_back(b);
		}
	}

#pragma region 自キャラと敵の弾の当たり判定
	posA = player_->GetWorldTranslate();
	for (EnemyBullet* bullet : enemyBullets)
	{
		// 無効な弾は当たり判定対象外にする
		if (!bullet || !bullet->IsActive()) continue;

		posB = bullet->GetWorldTranslate();

		// 距離（MathUtl の Distance を使用）
		float distance = Distance(posA, posB);

		const float threshold = 1.0f;
		if (distance < threshold)
		{
			player_->OnCollision();
			bullet->OnCollision();
		}
	}
#pragma endregion

#pragma region バリアと敵の当たり判定
	// バリアと敵本体の当たり判定を実装（書き方を他と統一）
	for (Enemy* enemy_ : enemies_)
	{
		if (!enemy_) continue;
		if (!enemy_->IsActive()) continue;

		posB = enemy_->GetWorldTranslate();

		for (const PlayerBarrier* barrier : barriers)
		{
			if (!barrier) continue;
			if (!barrier->IsActive()) continue;

			posA = barrier->GetWorldTranslate();

			float distance = Distance(posA, posB);
			const float threshold = 1.5f; // 判定半径 (必要に応じて調整)
			if (distance < threshold)
			{
				// 衝突発生: バリアと敵に衝突処理を通知
				const_cast<PlayerBarrier*>(barrier)->OnCollision();
				enemy_->OnCollision();
				break; // 敵は一度当たれば十分なのでループを抜ける
			}
		}
	}

	if (boss_ && boss_->IsActive())
	{
		posB = boss_->GetWorldTranslate();
		for (const PlayerBarrier* barrier : barriers)
		{
			if (!barrier) continue;
			if (!barrier->IsActive()) continue;

			posA = barrier->GetWorldTranslate();
			float distance = Distance(posA, posB);
			const float threshold = 2.5f; // bossは大きめ
			if (distance < threshold)
			{
				const_cast<PlayerBarrier*>(barrier)->OnCollision();
				// バリアで当たった場合は即死させず、ヒット扱いにする
				boss_->OnHit();
				break;
			}
		}
	}
#pragma endregion

#pragma region バリアと敵の弾の当たり判定
    // すべての弾に対して、任意のアクティブなバリアと衝突したら弾を無効化
    for (EnemyBullet* bullet : enemyBullets)
    {
        if (!bullet->IsActive()) continue;
        posB = bullet->GetWorldTranslate();

        for (const PlayerBarrier* barrier : barriers)
        {
            if (!barrier) continue;
            if (!barrier->IsActive()) continue;

            posA = barrier->GetWorldTranslate();

            float distance = Distance(posA, posB);
            const float threshold = 1.0f; // バリアのサイズに合わせて調整
            if (distance < threshold)
            {
                bullet->OnCollision();
                break; // この弾は処理済みなので次の弾へ
            }
        }
    }
#pragma endregion

	for (Enemy* enemy_ : enemies_)
	{
		if (!enemy_) continue;
		if (!enemy_->IsActive()) continue;

		posB = enemy_->GetWorldTranslate();

		for (const PlayerBarrier* barrier : barriers)
		{
			if (!barrier) continue;
			if (!barrier->IsActive()) continue;

			posA = barrier->GetWorldTranslate();

			float distance = Distance(posA, posB);
			const float threshold = 1.5f; // 判定半径 (必要に応じて調整)
			if (distance < threshold)
			{
				// 衝突発生: バリアと敵に衝突処理を通知
				const_cast<PlayerBarrier*>(barrier)->OnCollision();
				enemy_->OnCollision();

				// Spawn application sprite particles for fade-out effect
				if (spriteCom_ && !particleTexturePath_.empty()) {
					int count = 24;
					int screenW = object3dCom_->GetDirectXCom()->GetClientWidth();
					int screenH = object3dCom_->GetDirectXCom()->GetClientHeight();
					Vector2 base = WorldToScreen(posB, camera_, screenW, screenH);
					for (int i = 0; i < count; ++i) {
						Sprite* s = spriteCom_->CreateSprite(particleTexturePath_, {0.0f,0.0f}, {32.0f,32.0f}, 0.0f, {0.5f,0.5f});
						AppParticle ap;
						ap.sprite = s;
						ap.life = Random::GeneratorFloat(0.6f, 1.2f);
						ap.age = 0.0f;
						ap.pos = base;
						// random velocity in screen space
						float ang = Random::GeneratorFloat(0.0f, 6.2831853f);
						float spd = Random::GeneratorFloat(30.0f, 120.0f); // pixels/sec
						ap.vel = { std::cos(ang) * spd, std::sin(ang) * spd };
						// initial color
						s->SetPosition(ap.pos);
						s->SetScale({ 24.0f,24.0f });
						s->SetColor({1.0f,1.0f,1.0f,1.0f});
						s->Update();
						appParticles_.push_back(ap);
					}
				}

				// Spawn application mesh particles (OBJ) that fade over time
				if (model_ && model_->GetModel()) {
					Model* src = model_->GetModel();
					int meshCount = 8;
					for (int mi = 0; mi < meshCount; ++mi) {
						Model* mcopy = new Model(*src);
						Object3d* o = new Object3d();
						o->Initialize(object3dCom_);
						o->SetModel(mcopy);

						// small random velocity in world space
						float ang = Random::GeneratorFloat(0.0f, 6.2831853f);
						float r = Random::GeneratorFloat(0.5f, 2.0f);
						AppMeshParticle mp;
						mp.obj = o;
						mp.model = mcopy;
						mp.life = Random::GeneratorFloat(0.8f, 1.6f);
						mp.age = 0.0f;
						mp.vel = { std::cos(ang) * r, std::sin(ang) * r, Random::GeneratorFloat(-0.5f, 0.5f) };

						// set initial transform at world pos
						Transform tt; tt.Initialize();
						tt.SetTranslate(posB);
						tt.SetScale({0.3f, 0.3f, 0.3f});
						o->ApplyState(tt, camera_, true);

						// ensure initial color alpha 1
						mcopy->SetColor({1.0f,1.0f,1.0f,1.0f});

						appMeshParticles_.push_back(mp);
					}
				}

				break; // 敵は一度当たれば十分なのでループを抜ける
			}
		}
	}

	if (boss_ && boss_->IsActive())
	{
		posB = boss_->GetWorldTranslate();
		for (const PlayerBarrier* barrier : barriers)
		{
			if (!barrier) continue;
			if (!barrier->IsActive()) continue;

			posA = barrier->GetWorldTranslate();
			float distance = Distance(posA, posB);
			const float threshold = 2.5f; // bossは大きめ
			if (distance < threshold)
			{
				const_cast<PlayerBarrier*>(barrier)->OnCollision();
				// バリアで当たった場合は即死させず、ヒット扱いにする
				boss_->OnHit();
				break;
			}
		}
	}
#pragma endregion
}



void GameScene::SpawnWave()
{
	for (int i = 0; i < enemyCount; ++i)
	{
		Object3d* enemyModel = Object3d::Create(object3dCom_, "wall.obj", { {1,1,1},{0,0,0},{0,0,0} }, camera_);
		Enemy* enemy = new Enemy();
		Vector3 enemyPos = { static_cast<float>((i - enemyCount / 2) * 2), 0.0f, 10.0f };
		enemy->Initialize(enemyModel, camera_, enemyPos, object3dCom_);
		enemy->SetPlayer(player_);
		enemies_.push_back(enemy);
	}
}

#ifdef _DEBUG
void GameScene::ResetScene()
{
    // delete existing enemies
    for (Enemy* e : enemies_) { delete e; }
    enemies_.clear();

    // delete player
    if (player_) { delete player_; player_ = nullptr; }

    // delete rail camera controller
    if (railCameraController_) { delete railCameraController_; railCameraController_ = nullptr; }

    // delete boss and models
    if (boss_) { delete boss_; boss_ = nullptr; }
    if (bossBodyModel_) { delete bossBodyModel_; bossBodyModel_ = nullptr; }

    // delete fade
    if (fade_) { delete fade_; fade_ = nullptr; }

    // delete debug camera
#ifdef _DEBUG
    if (debugCamera_) { delete debugCamera_; debugCamera_ = nullptr; }
#endif

    // clear application particles
    for (auto &p : appParticles_) {
        if (p.sprite) { delete p.sprite; p.sprite = nullptr; }
    }
    appParticles_.clear();

    // reset wave state
    currentWave_ = 0;
    isWaitingForNextWave_ = false;
    waveDelayTimer_ = 0.0f;

    // Reinitialize the scene using stored pointers
    Initialize(camera_, object3dCom_, spriteCom_);
}
#endif
