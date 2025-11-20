#include "GameScene.h"

GameScene::~GameScene()
{
	delete player_;
	delete railCameraController_;
}

void GameScene::Initialize(Camera* camera, Object3dCom* object3dCom)
{

	object3dCom_ = object3dCom;
	camera_ = camera;
	// カメラの初期化（アスペクト比設定）
	camera_->Initialize();

	model_ = Object3d::Create(object3dCom_, "apple.obj", { {1,1,1},{0,0,0},{0,0,0} }, camera);

	player_ = new Player();
	player_->Initialize(model_,camera,{0.0f,0.0f,0.0f},object3dCom);

	railCameraController_ = new RailCameraController();
	railCameraController_->SetCamera(camera_);
	railCameraController_->Initialize({ 0.0f, 5.0f, -10.0f }, { 20.0f, 0.0f, 0.0f });
}

void GameScene::Update()
{
	player_->Update();

	railCameraController_->Update();
}

void GameScene::Draw()
{
	player_->Draw();
}
