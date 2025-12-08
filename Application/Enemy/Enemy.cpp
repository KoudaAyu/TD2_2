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

	// decrement temporary invincibility timer if active
	if (invincibilityTimerFrames_ > 0)
	{
		--invincibilityTimerFrames_;
	}

	switch (phase_
	)
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

	// 発射パターンごとに挙動を分ける
	switch (attackPattern_)
	{
		case AttackPattern::Straight:
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
			Vector3 bulletVelocity{ 0.0f, 0.0f, -bulletSpeed_ };
			bullet_->Initialize(bulletModel, spawnPos, object3dCom_, bulletVelocity);

			//弾を登録する
			bullets_.push_back(bullet_);
			break;
		}
		case AttackPattern::Aim:
		{
			// プレイヤー狙いの弾を1発
			AimBullet();
			break;
		}
		case AttackPattern::Rapid:
		{
			// Rapid: configurable shot count and spread, slightly slower for easier dodge
			const float kSpeed = bulletSpeed_ * 0.85f; // a bit slower for easier dodge
			int shotCount = rapidShotCount_;
			float spread = rapidSpread_;

			if (shotCount <= 1) shotCount = 1;

			for (int i = 0; i < shotCount; ++i)
			{
				float t = (float)i / (shotCount - 1);
				// map 0..1 -> -0.5..0.5
				float offset = (t - 0.5f);
				float ang = offset * spread; // radians approx

				// compute direction rotated around Y axis (horizontal spread)
				float dx = std::sin(ang);
				float dz = -std::cos(ang);
				Vector3 vel = { dx * kSpeed, 0.0f, dz * kSpeed };

				Object3d* bulletModel = new Object3d();
				bulletModel->Initialize(object3dCom_);
				if (model_ && model_->GetModel())
				{
					bulletModel->SetModel(new Model(*model_->GetModel()));
					const Vector4 bulletColor{ 1.0f, 0.2f, 0.2f, 1.0f };
					bulletModel->GetModel()->SetColor(bulletColor);
					bulletModel->SetColor(bulletColor);
				}

				EnemyBullet* b = new EnemyBullet();
				Vector3 spawnPos = worldTransform_.GetTranslate();
				spawnPos.z -= 1.0f;
				b->Initialize(bulletModel, spawnPos, object3dCom_, vel);
				bullets_.push_back(b);
			}

			break;
		}
	}
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

	// If configured, apply a random vertical offset to spawn target Y
	if (randomizeSpawnY_)
	{
		float r = Random::GeneratorFloat(-spawnYRandomRange_, spawnYRandomRange_);
		spawnTargetPos_.y += r;
	}

	spawnStartPos_ = { spawnTargetPos_.x, spawnTargetPos_.y + 8.0f, spawnTargetPos_.z + 10.0f };
	spawnTimer_ = 0;
	phase_ = Phase::Spawn;

	// 初期 transform は開始位置に設定
	worldTransform_.SetTranslate(spawnStartPos_);

	// 色はそのまま維持（EVA風カラー変更は行わない）

	// 出現直後の軽いカメラシェイク（演出）
	if (camera_)
	{
		camera_->StartShake(0.25f, 0.18f);
	}
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

	
	float jitterAmp = (1.0f - ease) * 0.06f; // 開始時ほど揺れ大
	newPos.x += Random::GeneratorFloat(-jitterAmp, jitterAmp);
	newPos.y += Random::GeneratorFloat(-jitterAmp * 0.5f, jitterAmp * 0.5f);

	worldTransform_.SetTranslate(newPos);

	// 少し回転しながら現れる演出
	Vector3 rot = worldTransform_.GetRotate();
	rot.x = (1.0f - ease) * 3.14f * 0.5f; // X軸を回転させて倒れながら出現
	rot.z += std::sin(spawnTimer_ * 0.2f) * (1.0f - ease) * 0.08f; // 微振動回転
	worldTransform_.SetRotate(rot);

	spawnTimer_++;
	// 出現時に少し光る、スケールを変化させる演出
	Vector3 scale = worldTransform_.GetScale();
	scale.x = scale.y = scale.z = 0.5f + 0.5f * ease; // 0.5 -> 1.0
	// 到着直前に一瞬膨張
	if (spawnTimer_ > spawnDuration_ - 12)
	{
		float pulse = std::sin((spawnTimer_ - (spawnDuration_ - 12)) * 0.6f) * 0.08f;
		scale.x += pulse; scale.y += pulse; scale.z += pulse;
	}
	worldTransform_.SetScale(scale);

	// 色変更は行わない（元のモデル色を維持）

	// 十字/走査線風の粒子演出
	{
		auto* pm = ParticleManager::GetInstance();
		if (pm)
		{
			// 十字（クロス）
			if ((spawnTimer_ % 8) == 0)
			{
				Vector3 emitPos = newPos; emitPos.z += 0.3f;
				pm->EmitBurst8("default", emitPos, 0.06f, 0.12f, 0.55f);
				pm->EmitBurst8Rotating("defaultMesh", emitPos, 0.02f, 0.45f,
					Random::GeneratorFloat(-6.0f, 6.0f), 0.07f, true, 0.6f, 0.0f);
			}
			// 縦走査線風
			if ((spawnTimer_ % 10) == 0)
			{
				Vector3 emitPos = newPos; emitPos.y += 0.8f; emitPos.z += 0.4f;
				pm->EmitBurst8("default", emitPos, 0.04f, 0.08f, 0.8f);
			}
		}
	}

	if (spawnTimer_ >= spawnDuration_)
	{
		// 出現完了、Approach フェーズへ移行
		phase_ = Phase::Approach;
		ApproachInitialize();
		// grant a few frames of invincibility to avoid being immediately hit by pre-existing bullets
        invincibilityTimerFrames_ = kSpawnInvincibilityFrames;
		// 到着演出: 短いショック
		if (camera_) camera_->StartShake(0.35f, 0.25f);
	}
}

void Enemy::ApproachInitialize()
{
	// reset charge flag for pre-fire particle
	chargeEmitted_ = false;
	chargeTimer_ = 0;
    // store base visual state
    approachBaseScale_ = worldTransform_.GetScale();
    // Model/Object3d don't provide a getter for color; use white as default base
    approachBaseColor_ = {1.0f, 1.0f, 1.0f, 1.0f};

	// If an explicit initial delay is set, use it for the first shot
	if (initialFireDelay_ >= 0)
	{
		fireTimer_ = initialFireDelay_;
		return;
	}

	// fire interval can be overridden by caller (wave settings)
	int interval = (fireIntervalOverride_ > 0) ? fireIntervalOverride_ : kFireInterval;
	fireTimer_ = interval;

	// ランダム初期オフセットを有効にしている場合はタイマーを少しずらす
	if (randomizeInitialFireOffset_)
	{
		float r = Random::GeneratorFloat(0.0f, static_cast<float>(fireTimer_));
		fireTimer_ = static_cast<int>(r);
	}
}

void Enemy::ApproachUpdate()
{
	worldTransform_ += approachVelocity;

	// Pre-fire charge: emit a noticeable particle a few frames before firing
	// Remove the pre-fire particle emission, but keep the visual pulse (scale/color)
	if (!chargeEmitted_ && fireTimer_ <= kChargeFrames && fireTimer_ > 0)
	{
		chargeEmitted_ = true;
		// start visual hold timer
		chargeVisualTimer_ = kChargeVisualFrames;
		// visual pulse: slightly enlarge & tint model
		if (model_) {
			Vector3 s = worldTransform_.GetScale();
			s.x *= 1.18f; s.y *= 1.18f; s.z *= 1.18f;
			worldTransform_.SetScale(s);
			Vector4 c = approachBaseColor_;
			c.x = 1.0f; c.y = 0.6f; c.z = 0.2f; c.w = 1.0f;
			model_->SetColor(c);
			if (model_->GetModel()) model_->GetModel()->SetColor(c);
		}
	}

	// 発射タイマーをデクリメントし、0以下になったら発射してリセット
	if (--fireTimer_ <= 0)
	{
		Fire();
		// reset charge flag for next cycle
		chargeEmitted_ = false;
		// restore visuals immediately on fire
		chargeVisualTimer_ = 0;
		worldTransform_.SetScale(approachBaseScale_);
		if (model_) { model_->SetColor(approachBaseColor_); if (model_->GetModel()) model_->GetModel()->SetColor(approachBaseColor_); }
		int interval = (fireIntervalOverride_ > 0) ? fireIntervalOverride_ : kFireInterval;
		fireTimer_ = interval;
		// if randomize initial offset is active, we only randomized initial timer; normal reset keeps interval
	}

	if (worldTransform_.GetTranslate().z < 0.0f)
	{
		phase_ = Phase::Leave;
		LeaveInitialize();
	}
	// If visual hold timer is active, decrement and restore visuals when expired
	if (chargeVisualTimer_ > 0) {
		--chargeVisualTimer_;
		if (chargeVisualTimer_ == 0) {
			worldTransform_.SetScale(approachBaseScale_);
			if (model_) { model_->SetColor(approachBaseColor_); if (model_->GetModel()) model_->GetModel()->SetColor(approachBaseColor_); }
		}
	}
}

void Enemy::LeaveInitialize()
{
    Vector3 pos = worldTransform_.GetTranslate();

     leaveCenter_ = { 0.0f, 0.5f, pos.z };

    leaveAngle_ = std::atan2(pos.y - leaveCenter_.y, pos.x - leaveCenter_.x);

    leaveAngularVel_ = Random::GeneratorFloat(-8.0f, 8.0f);

    float dx = pos.x - leaveCenter_.x;
    float dy = pos.y - leaveCenter_.y;
    leaveRadius_ = std::sqrt(dx*dx + dy*dy);

    leaveRadialSpeed_ = Random::GeneratorFloat(1.0f, 2.5f);

    leaveVelocity.z = 0.0f;

    leaveVelocity.y = Random::GeneratorFloat(-0.02f, 0.02f);

    collidable_ = false;

   leaveEscapeStarted_ = true;
    leaveEscapeTimer_ = 0.0f;
    leaveStartScale_ = worldTransform_.GetScale();

    leaveStartColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };

     if (auto* pm = ParticleManager::GetInstance())
    {
        Vector3 emitPos = worldTransform_.GetTranslate();
        emitPos.z += 0.3f;
        pm->EmitBurst8("default", emitPos, 0.06f, 0.12f, 0.5f);
    }
    if (camera_) camera_->StartShake(0.18f, 0.12f);

    phase_ = Phase::Leave;
}

void Enemy::LeaveUpdate()
{
    const float dt = 1.0f / 60.0f;

    if (leaveEscapeStarted_)
    {
        leaveEscapeTimer_ += dt;
        float t = leaveEscapeTimer_ / leaveEscapeDuration_;
        if (t > 1.0f) t = 1.0f;
       
        float ease = 1.0f - (1.0f - t) * (1.0f - t);

        
        Vector3 newScale;
        newScale.x = leaveStartScale_.x * (1.0f - ease);
        newScale.y = leaveStartScale_.y * (1.0f - ease);
        newScale.z = leaveStartScale_.z * (1.0f - ease);
      
        const float kMinScale = 0.02f;
        newScale.x = (std::max)(kMinScale, newScale.x);
        newScale.y = (std::max)(kMinScale, newScale.y);
        newScale.z = (std::max)(kMinScale, newScale.z);
        worldTransform_.SetScale(newScale);

        
        float alpha = 1.0f - ease;
        if (model_)
        {
           
            Vector4 c = leaveStartColor_;
            c.w = alpha;
            model_->SetColor(c);
            if (model_->GetModel()) model_->GetModel()->SetColor(c);
        }

        
        leaveAngle_ += leaveAngularVel_ * dt * 0.5f;
        leaveRadius_ += leaveRadialSpeed_ * dt * 0.5f;

     
        Vector3 basePos;
        basePos.x = leaveCenter_.x + std::cos(leaveAngle_) * leaveRadius_;
        basePos.y = leaveCenter_.y + std::sin(leaveAngle_) * leaveRadius_;
        basePos.z = worldTransform_.GetTranslate().z; 

       
        const Matrix4x4& camWorld = camera_->GetWorldMatrix();
        Vector3 camRight = { camWorld.m[0][0], camWorld.m[1][0], camWorld.m[2][0] };
        Vector3 camOffset = camRight * (leaveRadius_ * 0.01f);
        Vector3 newPos = basePos + camOffset;
        worldTransform_.SetTranslate(newPos);

      
        if (leaveEscapeTimer_ >= leaveEscapeDuration_ * 0.8f)
        {
            if (auto* pm = ParticleManager::GetInstance())
            {
                Vector3 emitPos = newPos; emitPos.z += 0.3f;
                pm->EmitBurst8("default", emitPos, 0.04f, 0.08f, 0.6f);
            }
        }

     
        if (t >= 1.0f)
        {
           
            if (auto* pm = ParticleManager::GetInstance())
            {
                Vector3 emitPos = worldTransform_.GetTranslate(); emitPos.z += 0.3f;
                pm->EmitBurst8("defaultMesh", emitPos, 0.06f, 0.35f, 0.9f);
            }

           
            isActive_ = false;
            worldTransform_.SetTranslate({ 10000.0f, 10000.0f, 10000.0f });
            return;
        }

    
        Vector3 rot = worldTransform_.GetRotate();
        rot.z += 0.5f * leaveAngularVel_ * dt;
        worldTransform_.SetRotate(rot);

        return;
    }

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
    const Matrix4x4& camWorld = camera_->GetWorldMatrix();
    Vector3 camRight = { camWorld.m[0][0], camWorld.m[1][0], camWorld.m[2][0] };

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

    // まず非アクティブ化のしきい値を判定し、該当するならパーティクルを出さずに終了
    if (newPos.z < leaveDeactivateZ_ || leaveRadius_ > leaveDeactivateRadius_)
    {
        isActive_ = false;
        worldTransform_.SetTranslate({ 10000.0f, 10000.0f, 10000.0f });
        return;
    }

    // 離脱フェーズのパーティクル生成はフラグで制御
    if (leaveEmitParticles_)
    {
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
                pm->EmitBurst8Rotating("default", emitPos, 0.1f, 0.35f, Random::GeneratorFloat(-12.0f, 12.0f), 0.06f, true, 0.6f, 0.0f);
                pm->EmitBurst8Rotating("default", emitPos, 0.1f, 0.35f, Random::GeneratorFloat(-12.0f, 12.0f), 0.06f, true, 0.6f, 0.0f);
            }
        }
    }
}

// 衝突処理の実装
void Enemy::OnCollision()
{
    Vector3 emitCenter = worldTransform_.GetTranslate();
    auto* pm = ParticleManager::GetInstance();
    if (pm)
    {
        Vector3 emitPos = emitCenter;
        emitPos.z += 0.5f;

        // 中心を強調する少し大きめのオムニバースト（1回）
        pm->Emit("default", emitPos, 32);

        // メッシュ片を回転・外向きに飛ばす（スケールを控えめに）
        pm->EmitBurst8Rotating("defaultMesh", emitPos,
            0.0f,   // startRadius
            1.0f,   // life
            Random::GeneratorFloat(-8.0f, 8.0f), // angularVel
            0.9f,   // scale (小さめに)
            true,
            1.8f,   // radialSpeed (外へ)
            0.3f);  // radialAccel

        // 一度外側に弾けたあと、中心へ収束する内向きの渦（スケールを小さく）
        pm->EmitBurst8RotatingInward("default", emitPos,
            3.0f,   // startRadius（外側スタート）
            0.8f,   // life
            Random::GeneratorFloat(-5.0f, 5.0f), // angularVel
            0.6f,   // scale (小さめ)
            1.6f,   // radialSpeedAbs (収縮の速さ)
            1.2f);  // radialAccelAbs

        // 小さめの光片を散らしてディテールを追加（スケール小）
        pm->EmitBurst8("default", emitPos, 0.25f, 0.5f, 0.9f);
        pm->EmitBurst8("defaultMesh", emitPos, 0.28f, 0.45f, 0.95f);
    }

    // カメラの演出（衝撃を強めに）
    if (camera_)
    {
        // 少し長めで強めの揺れ
        camera_->StartShake(0.8f, 0.6f);
    }

    // 衝突を受けたら簡単に画面外へ移動させ、保持している弾を無効化する
    worldTransform_.SetTranslate({ 10000.0f, 10000.0f, 10000.0f });
    for (EnemyBullet* b : bullets_)
    {
        if (b) b->OnCollision();
    }

    // 敵を非アクティブ化
    isActive_ = false;
}
