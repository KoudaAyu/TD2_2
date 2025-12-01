#include "EnemyBullet.h"
#include "Player.h"

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

	// 初期速度の大きさから speed を初期化
	float vlen = sqrtf(velocity_.x * velocity_.x + velocity_.y * velocity_.y + velocity_.z * velocity_.z);
	if (vlen > 0.0001f) speed_ = vlen;

	if (model_)
	{
		model_->ApplyState(worldTransform_, camera_, true);
	}
}

void EnemyBullet::Update()
{
	if (!isActive_)
	{
		return;
	}

	// 寿命管理
	if (lifeDuration_ > 0)
	{
		++lifeTimer_;
		if (lifeTimer_ >= lifeDuration_)
		{
			isActive_ = false;
			return;
		}
	}

	// ホーミング処理
	if (isHoming_ && target_)
	{
		Vector3 pos = worldTransform_.GetTranslate();
		Vector3 targetPos = target_->GetWorldTranslate();
		Vector3 toTarget = { targetPos.x - pos.x, targetPos.y - pos.y, targetPos.z - pos.z };
		// 目標方向の正規化
		float len = sqrtf(toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z);
		if (len > 0.0001f)
		{
			toTarget.x /= len; toTarget.y /= len; toTarget.z /= len;
			// 現在速度方向の正規化
			float vlen = sqrtf(velocity_.x * velocity_.x + velocity_.y * velocity_.y + velocity_.z * velocity_.z);
			Vector3 velDir = velocity_;
			if (vlen > 0.0001f) { velDir.x /= vlen; velDir.y /= vlen; velDir.z /= vlen; }
			// 緩やかに方向を補間
			Vector3 newDir = {
				velDir.x + (toTarget.x - velDir.x) * turnRate_,
				velDir.y + (toTarget.y - velDir.y) * turnRate_,
				velDir.z + (toTarget.z - velDir.z) * turnRate_
			};
			// 正規化
			float ndlen = sqrtf(newDir.x * newDir.x + newDir.y * newDir.y + newDir.z * newDir.z);
			if (ndlen > 0.0001f) { newDir.x /= ndlen; newDir.y /= ndlen; newDir.z /= ndlen; }
			velocity_.x = newDir.x * speed_;
			velocity_.y = newDir.y * speed_;
			velocity_.z = newDir.z * speed_;
		}
	}

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
	if(!isActive_)
	{
		return;
	}

	if (model_)
	{
		model_->Draw();
	}
}

void EnemyBullet::OnCollision()
{
	isActive_ = false;
}