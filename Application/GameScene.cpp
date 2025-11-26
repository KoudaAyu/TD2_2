#include "GameScene.h"

GameScene::~GameScene()
{
	delete enemy_;
	delete player_;
	delete railCameraController_;
#ifdef _DEBUG
	delete debugCamera_;
#endif
}

void GameScene::Initialize(Camera* camera, Object3dCom* object3dCom)
{

	object3dCom_ = object3dCom;
	camera_ = camera;
	// カメラの初期化（アスペクト比設定）
	camera_->Initialize();

	keyInput_ = KeyInput::GetInstance();	

#ifdef _DEBUG
	// 画面サイズから DebugCamera を初期化 (幅/高さは DirectXCom 経由で取得する想定)
	float width = static_cast<float>(object3dCom_->GetDirectXCom()->GetClientWidth());
	float height = static_cast<float>(object3dCom_->GetDirectXCom()->GetClientHeight());
	debugCamera_ = new DebugCamera(width, height);
	debugCamera_->Initialize();
#endif

	model_ = Object3d::Create(object3dCom_, "apple.obj", { {1,1,1},{0,0,0},{0,0,0} }, camera);
	enemyModel_ = Object3d::Create(object3dCom_, "wall.obj", { {1,1,1},{0,0,0},{0,0,0} }, camera);

	player_ = new Player();
	player_->Initialize(model_, camera, { 0.0f,0.0f,0.0f }, object3dCom);

	enemy_ = new Enemy();
	enemy_->Initialize(enemyModel_, camera, { 0.0f,0.0f,10.0f }, object3dCom);
	enemy_->SetPlayer(player_);

	railCameraController_ = new RailCameraController();
	railCameraController_->SetCamera(camera_);
	railCameraController_->Initialize({ 0.0f, 5.0f, -10.0f }, { 20.0f, 0.0f, 0.0f });
}

void GameScene::Update()
{
#ifdef _DEBUG
#ifdef USE_IMGUI
	// ImGuiフレーム中 (ImGuiManager::Begin() 呼び出し後) にのみUI描画
	player_->DrawImGui();
#endif

	if(keyInput_->TriggerKey(DIK_F1))
	{
		isDebugCameraActive_ = !isDebugCameraActive_;
	}

	if (isDebugCameraActive_ && debugCamera_)
	{
		debugCamera_->Update();
		// デバッグカメラの行列をメインカメラへコピー
		camera_->OverrideViewProjection(debugCamera_->GetViewMatrix(), debugCamera_->GetProjectionMatrix());
	}
	else
	{
		// 通常のレールカメラ更新
		railCameraController_->Update();
	}
#else
	// リリース時は通常カメラのみ
	railCameraController_->Update();
#endif
	enemy_->Update();
	player_->Update();

	CheckAllCollisions();
}

void GameScene::Draw()
{
	enemy_->Draw();
	player_->Draw();
}

void GameScene::CheckAllCollisions()
{
	Vector3 posA, posB;

	//バリア
	const PlayerBarrier* barrier = player_->GetBarrier();
	//敵の弾のリスト
	const std::list<EnemyBullet*>& enemyBullets = enemy_->GetBullets();

#pragma region 自キャラと敵の弾の当たり判定
	posA = player_->GetWorldTranslate();
	for(EnemyBullet* bullet : enemyBullets)
	{
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
#pragma endregion

#pragma region バリアと敵の弾の当たり判定
#pragma endregion
}
