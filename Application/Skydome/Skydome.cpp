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
	model_ = Object3d::Create(object3dCom_, "skydome/skydome.obj", { {1,1,1},{0,0,0},{0,0,0} }, camera_);

	worldTransform_.Initialize();
	
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
		// follow camera position so skydome stays centered on camera
		worldTransform_.SetTranslate(camera_->GetTranslate());

		// advance rotation slowly; assume 60fps tick like rest of app
		const float dt = 1.0f / 60.0f;
		rotationAngle_ += rotationSpeed_ * dt;
		const float twoPi = 2.0f * 3.14159265f;
		if (rotationAngle_ > twoPi) rotationAngle_ -= twoPi;

		// apply rotation around Y axis
		Vector3 rot = worldTransform_.GetRotate();
		rot.y = rotationAngle_;
		worldTransform_.SetRotate(rot);

		model_->ApplyState(worldTransform_, camera_, false);
	}
}
void Skydome::Draw()
{
	if (!model_) return;
	model_->Update();
	model_->Draw();
}
