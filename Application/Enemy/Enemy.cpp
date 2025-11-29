#include "Enemy.h"
#include "Player.h"
#include"Random.h"
#include"ParticleManager.h"

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

	// 出現アニメーション用の初期化を先に行い、モデルに開始位置を反映する
	SpawnInitialize(pos);

	// Apply initial state (now at spawn start position)
	if (model_)
	{
		model_->ApplyState(worldTransform_, camera_, true);
	}

	// 初期状態はアクティブ
	isActive_ = true;
	collidable_ = true;
	leaveParticleTimer_ = 0.0f;
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
		LeaveInitialize();
	}
}

void Enemy::LeaveInitialize()
{
	// 更にカッコよく：離脱は中心を基準にスパイラルしつつ外側へ放射
	Vector3 pos = worldTransform_.GetTranslate();

	// 中心を波のように少しずらして見栄えを調整
	leaveCenter_ = { 0.0f, 0.5f, pos.z };

	// 初期角度は自身のX位置に基づく
	leaveAngle_ = std::atan2(pos.y - leaveCenter_.y, pos.x - leaveCenter_.x);

	// 角速度はランダムで±範囲
	leaveAngularVel_ = Random::GeneratorFloat(-8.0f, 8.0f);

	// 初期半径は距離
	float dx = pos.x - leaveCenter_.x;
	float dy = pos.y - leaveCenter_.y;
	leaveRadius_ = std::sqrt(dx*dx + dy*dy);

	// 半径の拡大速度（外へ広がる）
	leaveRadialSpeed_ = Random::GeneratorFloat(2.0f, 3.5f);

	// Z方向は早めに奥へ行く
	leaveVelocity.z = Random::GeneratorFloat(-1.2f, -0.6f);

	// 少し縦に動かす
	leaveVelocity.y = Random::GeneratorFloat(0.0f, 0.25f);

	// 離脱中は当たり判定を無効化
	collidable_ = false;

	// 回転演出の初期化
	phase_ = Phase::Leave;
}

void Enemy::LeaveUpdate()
{
	// 回転角を進める
	leaveAngle_ += leaveAngularVel_ * (1.0f/60.0f);
	// 半径を広げる
	leaveRadius_ += leaveRadialSpeed_ * (1.0f/60.0f);

	// 位置を極座標から計算（Zは前方へ）
	Vector3 basePos;
	basePos.x = leaveCenter_.x + std::cos(leaveAngle_) * leaveRadius_;
	basePos.y = leaveCenter_.y + std::sin(leaveAngle_) * leaveRadius_;
	basePos.z = worldTransform_.GetTranslate().z + leaveVelocity.z;

	// カメラ右ベクトルを取得してスクリーン方向の動きを追加
	// Camera のワールド行列から右ベクトルを取り出す
	const Matrix4x4& camWorld = camera_->GetWorldMatrix();
	Vector3 camRight = { camWorld.m[0][0], camWorld.m[1][0], camWorld.m[2][0] };

	// カメラ基準で横に流す量（画面上で見やすくするための強度）
	float camSlideStrength = 0.6f; // 調整可

	Vector3 camOffset = camRight * (leaveRadius_ * 0.02f * camSlideStrength);

	Vector3 newPos = basePos + camOffset;

	// 少しスケールダウンしていく
	Vector3 scale = worldTransform_.GetScale();
	scale.x = scale.y = scale.z = (std::max)(0.05f, scale.x - 0.02f);
	worldTransform_.SetScale(scale);

	// 徐々に回転を加える
	Vector3 rot = worldTransform_.GetRotate();
	rot.z += 0.2f * leaveAngularVel_ * (1.0f/60.0f);
	worldTransform_.SetRotate(rot);

	worldTransform_.SetTranslate(newPos);

	// パーティクル発生タイマー更新（秒）
	leaveParticleTimer_ += (1.0f/60.0f);
	if (leaveParticleTimer_ >= leaveParticleInterval_)
	{
		leaveParticleTimer_ = 0.0f;

		// パーティクル生成：小さなバーストと回転バーストを交互に呼ぶ
		auto* pm = ParticleManager::GetInstance();
		if (pm) {
			Vector3 emitPos = newPos;
			// 少し手前に出す
			emitPos.z += 0.5f;
			pm->EmitBurst8("default", emitPos, 0.06f, 0.08f, 0.25f);
			pm->EmitBurst8Rotating("default", emitPos, 0.1f, 0.35f, Random::GeneratorFloat(-12.0f, 12.0f), 0.06f, true, 0.6f);
		}
	}

	if (newPos.z < -20.0f || leaveRadius_ > 80.0f)
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
