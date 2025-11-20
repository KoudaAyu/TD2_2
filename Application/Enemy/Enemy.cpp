#include "Enemy.h"
#include "Player.h"

Enemy::~Enemy()
{
	for (EnemyBullet* bullet : bullets_)
	{
		delete bullet;
	}
	bullets_.clear();
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

	// 発射関数を初期化時に呼び出す
	ApproachInitialize();
}

void Enemy::Update()
{

	switch (phase_)
	{
		case Phase::Approach:
		default:
		{
			ApproachUpdate();
		}
		break;
		case Phase::Leave:
		{
			LeaveUpdate();
			break;
		}
	}
	for (EnemyBullet* bullet : bullets_)
	{
		bullet->Update();
	}

	
	worldTransform_.TransferMatrix();
	if (model_) model_->ApplyState(worldTransform_, camera_, true);
}

void Enemy::Draw()
{

	for (EnemyBullet* bullet : bullets_)
	{
		bullet->Draw();
	}
	if (model_)
	{
		model_->Draw();
	}
}

void Enemy::Fire()
{

	Object3d* bulletModel = new Object3d();
	bulletModel->Initialize(object3dCom_);


	if (model_)
	{
		if (auto* src = model_->GetModel())
		{

			bulletModel->SetModel(new Model(*src));

			const Vector4 bulletColor{ 1.0f, 0.2f, 0.2f, 1.0f };
			bulletModel->GetModel()->SetColor(bulletColor);
			bulletModel->SetColor(bulletColor);
		}
	}

	EnemyBullet* bullet_ = new EnemyBullet();
	Vector3 spawnPos = worldTransform_.GetTranslate();
	spawnPos.z -= 1.0f;
	Vector3 bulletVelocity{ 0.0f, 0.0f, -1.0f };
	bullet_->Initialize(bulletModel, spawnPos, object3dCom_, bulletVelocity);

	//弾を登録する
	bullets_.push_back(bullet_);
}

void Enemy::AimBullet()
{
#ifdef _DEBUG
	assert(player_);
#endif

	//弾の速さ
	const float kBulletSpeed = 1.0f;

	// 敵とプレイヤーの位置差を計算し、正規化して速さに合わせる
	Vector3 enemyPos = worldTransform_.GetTranslate();
	Vector3 playerPos = player_->GetWorldTranslate();
	Vector3 dir{ playerPos.x - enemyPos.x, playerPos.y - enemyPos.y, playerPos.z - enemyPos.z };

	// 正規化
	Vector3 normDir = Normalize(dir);
	Vector3 bulletVelocity{ 0.0f, 0.0f, -1.0f };
	if (Length(dir) > 1e-6f)
	{
		bulletVelocity = normDir * kBulletSpeed;
	}
	else
	{
		bulletVelocity = { 0.0f, 0.0f, -kBulletSpeed };
	}

	Object3d* bulletModel = new Object3d();
	bulletModel->Initialize(object3dCom_);


	if (model_)
	{
		if (auto* src = model_->GetModel())
		{

			bulletModel->SetModel(new Model(*src));

			const Vector4 bulletColor{ 1.0f, 0.2f, 0.2f, 1.0f };
			bulletModel->GetModel()->SetColor(bulletColor);
			bulletModel->SetColor(bulletColor);
		}
	}

	EnemyBullet* bullet_ = new EnemyBullet();
	Vector3 spawnPos = worldTransform_.GetTranslate();
	spawnPos.z -= 1.0f;
	// bulletVelocity は上で計算済み
	bullet_->Initialize(bulletModel, spawnPos, object3dCom_, bulletVelocity);

	//弾を登録する
	bullets_.push_back(bullet_);
}

void Enemy::ApproachInitialize()
{
	
	fireTimer_ = kFireInterval;
}

void Enemy::ApproachUpdate()
{
	worldTransform_ += approachVelocity;

	// 発射タイマーをデクリメントし、0以下になったら発射してリセット
	if (--fireTimer_ <= 0)
	{
		Fire();
		fireTimer_ = kFireInterval;
	}

	if (worldTransform_.GetTranslate().z < 0.0f)
	{
		phase_ = Phase::Leave;
	}
}

void Enemy::LeaveUpdate()
{
	worldTransform_ += leaveVelocity;
}
