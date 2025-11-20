#include "EnemyBullet.h"

EnemyBullet::~EnemyBullet()
{
	delete model_;

}

void EnemyBullet::Initialize(Object3d* model, const Vector3 pos, Object3dCom* object3dCom, const Vector3& velocity)
{
	object3dCom_ = object3dCom;
	velocity_ = velocity;

	worldTransform_.Initialize();
	worldTransform_.SetTranslate(pos);


	model_ = new Object3d();
	model_->Initialize(object3dCom_);


	if (model && model->GetModel())
	{
		model_->SetModel(new Model(*model->GetModel()));
		const Vector4 bulletColor{ 1.0f, 0.2f, 0.2f, 1.0f };
		model_->GetModel()->SetColor(bulletColor);
		model_->SetColor(bulletColor);
	}


	if (object3dCom_)
	{
		camera_ = object3dCom_->GetDefaultCamera();
	}
	else
	{
		camera_ = nullptr;
	}

	// Apply initial state
	if (model_)
	{
		model_->ApplyState(worldTransform_, camera_, true);
	}
}

void EnemyBullet::Update()
{
	worldTransform_ += velocity_;
	worldTransform_.TransferMatrix();
	if (model_)
	{
		model_->ApplyState(worldTransform_, camera_, true);
		model_->Update();
	}
}

void EnemyBullet::Draw()
{
	if (model_)
	{
		model_->Draw();
	}
}