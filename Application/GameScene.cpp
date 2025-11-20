#include "GameScene.h"

GameScene::~GameScene()
{
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

#ifdef _DEBUG
	// 画面サイズから DebugCamera を初期化 (幅/高さは DirectXCom 経由で取得する想定)
	float width = static_cast<float>(object3dCom_->GetDirectXCom()->GetClientWidth());
	float height = static_cast<float>(object3dCom_->GetDirectXCom()->GetClientHeight());
	debugCamera_ = new DebugCamera(width, height);
	debugCamera_->Initialize();
#endif

	model_ = Object3d::Create(object3dCom_, "apple.obj", { {1,1,1},{0,0,0},{0,0,0} }, camera);

	player_ = new Player();
	player_->Initialize(model_, camera, { 0.0f,0.0f,0.0f }, object3dCom);

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



	if (debugCamera_) { debugCamera_->Update(); }
#endif

	player_->Update();


	railCameraController_->Update();
}

void GameScene::Draw()
{
	player_->Draw();
}
