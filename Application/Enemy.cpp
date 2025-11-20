#include "Enemy.h"

Enemy::~Enemy()
{
}

void Enemy::Initialize(Object3d* model, Camera* camera, const Vector3 pos, Object3dCom* object3dCom)
{
	model_ = model;
	camera_ = camera;
	object3dCom_ = object3dCom;
	worldTransform_.Initialize();
	worldTransform_.SetTranslate(pos);
	if (model_)
	{
		model_->ApplyState(worldTransform_, camera_, true);
	}
}

void Enemy::Update()
{
	worldTransform_ += velocity;
	worldTransform_.TransferMatrix();
	if (model_) model_->ApplyState(worldTransform_, camera_, true); 
}

void Enemy::Draw()
{
	model_->Draw();
}
