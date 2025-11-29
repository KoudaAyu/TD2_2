#include "Enemy.h"
#include "Player.h"

Enemy::~Enemy()
{
	for (EnemyBullet* bullet : bullets_)
	{
		delete bullet;
		bullet = nullptr;
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
	SpawnInitialize(pos);

	// 初期状態はアクティブ
	isActive_ = true;
}

void Enemy::Update()
{
	if (!isActive_)
	{
		// 非アクティブ時は弾のみ更新して描画はしない
		for (EnemyBullet* bullet : bullets_)
		{
			if (bullet) bullet->Update();
		}
		return;
	}

		switch (phase_)
	{
		case Phase::Spawn:
		{
			SpawnUpdate();
			break;
		}
		case Phase::Approach:
		{
			ApproachUpdate();
			break;
		}
		case Phase::Leave:
		{
			LeaveUpdate();
			break;
		}
	}
	for (EnemyBullet* bullet : bullets_)
	{
		if (bullet) bullet->Update();
	}

	
	worldTransform_.TransferMatrix();
	if (model_) model_->ApplyState(worldTransform_, camera_, true);
}

void Enemy::Draw()
{
	for (EnemyBullet* bullet : bullets_)
	{
		if (bullet) bullet->Draw();
	}
	if (model_ && isActive_)
	{
		model_->Draw();
	}
}

void Enemy::Fire()
{
	if (!isActive_) return; // 非アクティブ時は発射しない

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

void Enemy::SpawnInitialize(const Vector3& targetPos)
{
	// 出現アニメーションの開始位置を画面外上方に固定、あるいはランダム化しても良い
	spawnTargetPos_ = targetPos;
	spawnStartPos_ = { targetPos.x, targetPos.y + 8.0f, targetPos.z + 10.0f };
	spawnTimer_ = 0;
	phase_ = Phase::Spawn;

	// 初期 transform は開始位置に設定
	worldTransform_.SetTranslate(spawnStartPos_);
}

void Enemy::SpawnUpdate()
{
	// イーズインのための簡単なイージング（2乗で滑らかに減速）
	float t = static_cast<float>(spawnTimer_) / static_cast<float>(spawnDuration_);
	if (t > 1.0f) t = 1.0f;
	// ease out quad
	float ease = 1.0f - (1.0f - t) * (1.0f - t);

	Vector3 newPos;
	newPos.x = spawnStartPos_.x + (spawnTargetPos_.x - spawnStartPos_.x) * ease;
	newPos.y = spawnStartPos_.y + (spawnTargetPos_.y - spawnStartPos_.y) * ease;
	newPos.z = spawnStartPos_.z + (spawnTargetPos_.z - spawnStartPos_.z) * ease;

	worldTransform_.SetTranslate(newPos);

	// 少し回転しながら現れる演出
	Vector3 rot = worldTransform_.GetRotate();
	rot.x = (1.0f - ease) * 3.14f * 0.5f; // X軸を回転させて倒れながら出現
	worldTransform_.SetRotate(rot);

	spawnTimer_++;
	// 出現時に少し光る、スケールを変化させる演出
	Vector3 scale = worldTransform_.GetScale();
	scale.x = scale.y = scale.z = 0.5f + 0.5f * ease; // 0.5 -> 1.0
	worldTransform_.SetScale(scale);

	if (spawnTimer_ >= spawnDuration_)
	{
		// 出現完了、Approach フェーズへ移行
		phase_ = Phase::Approach;
		ApproachInitialize();
	}
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


	if (worldTransform_.GetTranslate().z < -20.0f)
	{
		isActive_ = false;
		
		worldTransform_.SetTranslate({ 10000.0f, 10000.0f, 10000.0f });
	}
}

// 衝突処理の実装
void Enemy::OnCollision()
{
	// 衝突を受けたら簡単に画面外へ移動させ、保持している弾を無効化する
	worldTransform_.SetTranslate({ 10000.0f, 10000.0f, 10000.0f });
	for (EnemyBullet* b : bullets_)
	{
		if (b) b->OnCollision();
	}

	// 敵を非アクティブ化
	isActive_ = false;
}
