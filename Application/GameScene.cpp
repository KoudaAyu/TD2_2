#include "GameScene.h"

GameScene::~GameScene()
{
	delete player_;
}

void GameScene::Initialize(Camera* camera, Object3dCom* object3dCom)
{

	object3dCom_ = object3dCom;
	camera_ = camera;

	model_ = Object3d::Create(object3dCom_, "apple.obj", { {1,1,1},{0,0,0},{0,0,0} }, camera);

	player_ = new Player();
	player_->Initialize(model_,camera,{0.0f,0.0f,0.0f},object3dCom);
}

void GameScene::Update()
{
	player_->Update();
}

void GameScene::Draw()
{
	player_->Draw();
}
