#include"Skydome.h"

Skydome::Skydome()
{
}
Skydome::~Skydome()
{
}
void Skydome::Initialize(Object3dCom* object3dCom, Camera* camera)
{
	object3dCom_ = object3dCom;
	camera_ = camera;
	model_ = Object3d::Create(object3dCom_, "skydome.obj", { {1,1,1},{0,0,0},{0,0,0} }, camera_);

	worldTransform_.Initialize();
	worldTransform_.SetScale({ -30.0f, 30.0f, 30.0f });
	if (camera_) {
		worldTransform_.SetTranslate(camera_->GetTranslate());
	}

	if (model_)
	{
		model_->ApplyState(worldTransform_, camera_, true);
	}

}
void Skydome::Update()
{
	if (camera_ && model_)
	{
		worldTransform_.SetTranslate(camera_->GetTranslate());
		model_->ApplyState(worldTransform_, camera_, false);
	}
}
void Skydome::Draw()
{
	if (!model_) return;
	model_->Update();
	model_->Draw();
}
