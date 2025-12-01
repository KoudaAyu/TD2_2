#include "Boss.h"
#include "HeadPart.h"
#include "TurretPart.h"
#include "BodyPart.h"
#include "Player.h"
#include "Random.h"

#include <algorithm>
#include "SpriteCom.h"
#include "Sprite.h"
#include <filesystem>
#include <cmath>

Boss::Boss() {}

Boss::~Boss()
{
	for (EnemyBullet* b : bullets_)
	{
		delete b;
	}
	bullets_.clear();

	// スプライト解放
	for (auto s : phaseSprites_)
	{
		if (s) delete s;
	}
}

Vector3 Boss::GetPlayerWorldTranslate() const
{
	if (player_) return player_->GetWorldTranslate();
	return { 0.0f, 0.0f, 0.0f };
}


static std::string FindNumberTexturePath(const std::string& fileName)
{
	// 候補パス（実行時カレントやプロジェクト構成に応じてチェック）
	const std::vector<std::string> candidates = {
		std::string("Resources/number/") + fileName,
		fileName
	};

	for (const auto& c : candidates)
	{
		if (std::filesystem::exists(std::filesystem::path(c)))
		{
			return c;
		}
	}

	return std::string(); // 見つからない
}


void Boss::Initialize(Object3d* model, Camera* camera, const Vector3 pos, Object3dCom* object3dCom, SpriteCom* spriteCom)
{
	model_ = model;
	camera_ = camera;
	object3dCom_ = object3dCom;

	// SpriteCom を保持（デバッグ表示用）
	spriteCom_ = spriteCom;

	worldTransform_.Initialize();
	spawnTarget_ = pos;
	spawnStart_ = { pos.x, pos.y + 8.0f, pos.z + 40.0f };
	worldTransform_.SetTranslate(spawnTarget_);

	if (model_)
	{
		model_->ApplyState(worldTransform_, camera_, true);
	}

	const int gridSizeX = 3;
	const int gridSizeY = 3;
	const float spacing = 1.2f;

	float offsetX = -(gridSizeX - 1) * 0.5f * spacing;
	float offsetY = -(gridSizeY - 1) * 0.5f * spacing;

	for (int y = 0; y < gridSizeY; ++y)
	{
		for (int x = 0; x < gridSizeX; ++x)
		{
			Vector3 partPos = { offsetX + x * spacing, offsetY + y * spacing, 0.0f };
			parts_.push_back(std::make_unique<BodyPart>());
			parts_.back()->Initialize(this, model_, partPos);
		}
	}

	const int layers = 2;
	for (int l = 1; l < layers; ++l)
	{
		float layerHeight = 0.9f * l;
		for (int y = 0; y < gridSizeY; ++y)
		{
			for (int x = 0; x < gridSizeX; ++x)
			{
				Vector3 partPos = { offsetX + x * spacing, offsetY + y * spacing, layerHeight };
				parts_.push_back(std::make_unique<BodyPart>());
				parts_.back()->Initialize(this, model_, partPos);
			}
		}
	}

	for (size_t i = 0; i < parts_.size(); ++i)
	{
		BossPart* p = parts_[i].get();
		if (!p) continue;
		Vector3 targetLocal = p->GetWorldTranslate();
		Vector3 bossPos = worldTransform_.GetTranslate();
		Vector3 localTarget = { targetLocal.x - bossPos.x, targetLocal.y - bossPos.y, targetLocal.z - bossPos.z };

		float scatterScale = 4.0f;
		Vector3 startLocal = { localTarget.x * scatterScale, localTarget.y * scatterScale + 6.0f, localTarget.z + 12.0f };

		int dur = spawnDuration_;
		p->StartSpawn(startLocal, dur);
	}

	isActive_ = true;


	phase_ = Phase::Spawn;
	spawnTimer_ = 0;

	// 初期 HP 設定（最大 HP にリセット）
	hp_ = maxHP_;

	// ヒットカウント初期化
	hitCount_ = 0;
	hitCooldownTimer_ = 0;

	// デバッグ用スプライト作成 
	if (spriteCom_)
	{
		// 1.png ～ 5.png を読み込み
		for (int i = 0; i < 5; ++i)
		{
			std::string fileName = std::to_string(i + 1) + ".png";
			std::string path = FindNumberTexturePath(fileName);
			if (path.empty())
			{
				// 見つからなければ作成をスキップ（assert を回避）
				phaseSprites_[i] = nullptr;
				continue;
			}

			Sprite* s = spriteCom_->CreateSprite(path, phaseSpritePosition_, phaseSpriteScale_, 0.0f, { 0.0f,0.0f }, false, false);
			phaseSprites_[i] = s;
			// 初期は非表示にするため alpha を 0 に（Sprite の色にアクセス）
			Vector4 c = s->GetColor();
			c.w = 0.0f; // alpha
			s->SetColor(c);
			s->Update();
		}
	}
}

void Boss::OnHit()
{
	// Spawn/Leave 時や無敵中はカウントしない
	if (phase_ == Phase::Spawn || phase_ == Phase::Leave) return;
	if (hitCooldownTimer_ > 0) return; // 無敵フレーム中は無視

	++hitCount_;

	if (hitCount_ > hitsPerFull) hitCount_ = hitsPerFull;

	// 1ヒットで Phase2 に見せたい場合はインデックスずらしを行う
	int clampedIndex = (std::min)(hitCount_, hitsPerFull - 1);
	Phase newPhase = static_cast<Phase>(static_cast<int>(Phase::Phase1) + clampedIndex);
	phase_ = newPhase;

	// カメラ揺れ
	if (camera_ && cameraShakeCooldown_ <= 0.0f)
	{
		camera_->StartShake(0.2f, 0.2f);
		cameraShakeCooldown_ = kCameraShakeCooldownSeconds;
	}

	// スプライト更新
	if (spriteCom_)
	{
		int currentPhaseIndex = (phase_ >= Phase::Phase1 && phase_ <= Phase::Phase5) ?
			static_cast<int>(phase_) - static_cast<int>(Phase::Phase1) : 0;
		for (int i = 0; i < 5; ++i)
		{
			Sprite* s = phaseSprites_[i];
			if (!s) continue;
			Vector4 col = s->GetColor();
			col.w = (currentPhaseIndex == i) ? 1.0f : 0.0f;
			s->SetColor(col);
			s->Update();
		}
	}

	// 無敵タイマーをリセットして短時間多重カウントを防止
	hitCooldownTimer_ = kHitCooldownFrames;

	// ヒットが上限に達したらボスを撃破扱いにする（シーン遷移のトリガ）
	if (hitCount_ >= hitsPerFull)
	{
		// ここで部位を破壊する演出を追加しても良いが、簡易的に衝突処理を呼びボスを無効化する
		OnCollision();
	}
}

void Boss::Shoot()
{
	// 一定間隔で前方向へ直進弾を発射する
	++ShootTimer_;
	if (ShootTimer_ < ShootInterval_) return;
	ShootTimer_ = 0;

	// 生存している部位のインデックスを収集
	std::vector<int> aliveIndices;
	aliveIndices.reserve(parts_.size());
	for (size_t i = 0; i < parts_.size(); ++i)
	{
		if (parts_[i] && !parts_[i]->IsDestroyed()) aliveIndices.push_back(static_cast<int>(i));
	}

	for (int i = 0; i < phase1BulletsPerShot_; ++i)
	{
		Vector3 spawnPos;
		if (!aliveIndices.empty())
		{
			// aliveIndices の中からランダムに選択
			float r = Random::GeneratorFloat(0.0f, static_cast<float>(aliveIndices.size() - 1));
			int pick = static_cast<int>(std::floor(r + 0.5f));
			if (pick < 0) pick = 0;
			if (pick >= static_cast<int>(aliveIndices.size())) pick = static_cast<int>(aliveIndices.size() - 1);
			spawnPos = parts_[aliveIndices[pick]]->GetWorldTranslate();
		}
		else
		{
			// どの部位も無い（全壊）ならボス中心から出す
			spawnPos = worldTransform_.GetTranslate();
		}

		// 方向: -Z（カメラやプレイヤーが前方にいる想定）
		Vector3 dir = { 0.0f, 0.0f, -1.0f };

		Vector3 vel = { dir.x * phase1BulletSpeed_, dir.y * phase1BulletSpeed_, dir.z * phase1BulletSpeed_ };

		EnemyBullet* b = new EnemyBullet();
		// model_ を複製して渡す（EnemyBullet が内部でコピーする既存実装に合わせる）
		b->Initialize(model_, spawnPos, object3dCom_, vel);
		bullets_.push_back(b);
	}
}

void Boss::Move()
{
	const float dt = 1.0f / 60.0f;

	phase1Time_ += dt;

	// ベースとなる注視点（スポーン位置またはプレイヤー）
	Vector3 baseTarget = spawnTarget_;
	if (player_)
	{
		Vector3 playerPos = player_->GetWorldTranslate();
		// 横方向（X）と前後（Z）はプレイヤーに追従するが、Y（高さ）は追従させない
		baseTarget.x = playerPos.x;
		baseTarget.z = playerPos.z + 5.0f;
	}

	// 軌道オフセットを計算（左右:X, 上下:Y）。Z は変化させない
	const float twoPi = 2.0f * 3.14159265f;
	float angleX = twoPi * phase1OrbitSpeedX_ * phase1Time_;
	float angleY = twoPi * phase1OrbitSpeedY_ * phase1Time_;

	float offsetX = std::cos(angleX) * phase1OrbitRadiusX_;
	float offsetY = std::sin(angleY) * phase1OrbitRadiusY_;

	phase1TargetOffset_.x = offsetX;
	phase1TargetOffset_.y = offsetY;
	phase1TargetOffset_.z = 0.0f; // Z は動かさない

	Vector3 desiredTarget = { baseTarget.x + phase1TargetOffset_.x,
						 baseTarget.y + phase1TargetOffset_.y,
						 baseTarget.z };

	Vector3 cur = worldTransform_.GetTranslate();

	// 水平方向（X 軸）をターゲットに向かって移動させる。Z は変更しない。
	float dirX = desiredTarget.x - cur.x;
	float distX = std::fabs(dirX);
	if (distX > 0.0001f)
	{
		float moveDist = phase1MoveSpeed_ * 60.0f * dt;
		if (moveDist > distX) moveDist = distX;
		cur.x += (dirX > 0.0f ? 1.0f : -1.0f) * moveDist;
	}

	// 垂直方向の Y: 軌道オフセット + ボビング
	float bob = phase1BobAmplitude_ * sinf(twoPi * phase1BobFrequency_ * phase1Time_);
	cur.y = baseTarget.y + phase1TargetOffset_.y + bob;

	// Z は変更しない（既存の cur.z を維持）

	worldTransform_.SetTranslate(cur);
}

void Boss::UpdatePhase1()
{
	Move();

	Shoot();
}

void Boss::Update()
{
	if (!isActive_)
	{
		for (EnemyBullet* b : bullets_)
		{
			if (b) b->Update();
		}
		return;
	}

	// ヒット無敵タイマー更新
	if (hitCooldownTimer_ > 0) --hitCooldownTimer_;

	// カメラシェイクのクールダウン更新
	const float dt = 1.0f / 60.0f;
	if (cameraShakeCooldown_ > 0.0f)
	{
		cameraShakeCooldown_ -= dt;
		if (cameraShakeCooldown_ < 0.0f) cameraShakeCooldown_ = 0.0f;
	}

	if (phase_ == Phase::Spawn)
	{
		++spawnTimer_;
		float t = (spawnDuration_ <= 0) ? 1.0f : (float)spawnTimer_ / (float)spawnDuration_;
		if (t > 1.0f) t = 1.0f;


		for (auto& p : parts_)
		{
			if (p) p->UpdateSpawn(t);
		}

		if (spawnTimer_ >= spawnDuration_)
		{
			phase_ = Phase::Phase1; // スポーン後はフェーズ1 から開始

			if (camera_ && cameraShakeCooldown_ <= 0.0f)
			{
				camera_->StartShake(0.6f, 0.6f);
				cameraShakeCooldown_ = kCameraShakeCooldownSeconds;
			}
		}
	}

	// フェーズ更新
	UpdatePhaseByHP();

	// デバッグ用: フェーズに合わせてスプライトの表示切替
	if (spriteCom_)
	{
		for (int i = 0; i < 5; ++i)
		{
			Sprite* s = phaseSprites_[i];
			if (!s) continue;
			Vector4 c = s->GetColor();
			int currentPhaseIndex = 0;
			if (phase_ >= Phase::Phase1 && phase_ <= Phase::Phase5)
			{
				currentPhaseIndex = static_cast<int>(phase_) - static_cast<int>(Phase::Phase1);
			}
			c.w = (currentPhaseIndex == i) ? 1.0f : 0.0f;
			s->SetColor(c);
			s->Update();
		}
	}

	// Phase1 の行動を実行
	if (phase_ == Phase::Phase1)
	{
		UpdatePhase1();
	}

	for (auto& p : parts_)
	{
		if (p) p->Update();
	}

	for (EnemyBullet* b : bullets_)
	{
		if (b) b->Update();
	}

	worldTransform_.TransferMatrix();
	if (model_) model_->ApplyState(worldTransform_, camera_, true);

	bool allDestroyed = true;
	for (auto& p : parts_)
	{
		if (p && !p->IsDestroyed())
		{
			allDestroyed = false;
			break;
		}
	}
	if (allDestroyed)
	{
		isActive_ = false;
	}
}

void Boss::Draw()
{
	for (EnemyBullet* b : bullets_)
	{
		if (b) b->Draw();
	}


	for (auto& p : parts_)
	{
		if (p) p->Draw();
	}

	// デバッグ用スプライトの描画（Phase1~Phase5 のみ表示）
	if (spriteCom_)
	{
		for (int i = 0; i < 5; ++i)
		{
			Sprite* s = phaseSprites_[i];
			if (!s) continue;
			s->Draw();
		}
	}
}

void Boss::OnCollision()
{
	worldTransform_.SetTranslate({ 10000.0f, 10000.0f, 10000.0f });
	for (EnemyBullet* b : bullets_)
	{
		if (b) b->OnCollision();
	}
	isActive_ = false;
}

// HP に基づいてフェーズを更新する
void Boss::UpdatePhaseByHP()
{
	// Spawn / Leave 中は遷移を行わない
	if (phase_ == Phase::Spawn || phase_ == Phase::Leave) return;

	// まずヒット数ベースの優先処理：ヒットでフェーズを進める設計になっている場合はこちらを優先
	if (hitCount_ > 0)
	{
		int clampedIndex = (std::min)(hitCount_, hitsPerFull - 1);
		Phase newPhase = static_cast<Phase>(static_cast<int>(Phase::Phase1) + clampedIndex);
		if (newPhase != phase_)
		{
			phase_ = newPhase;
			if (camera_ && cameraShakeCooldown_ <= 0.0f)
			{
				camera_->StartShake(0.3f, 0.3f);
				cameraShakeCooldown_ = kCameraShakeCooldownSeconds;
			}
		}
		return; // ヒットベース優先なので HP による更新は行わない
	}

	if (maxHP_ <= 0) return;

	float hpRatio = static_cast<float>(hp_) / static_cast<float>(maxHP_);

	// hpPhaseThresholds_ 配列は {1.0, 0.8, 0.6, 0.4, 0.2} のように 5 段階を保持
	// フェーズは Phase1 ~ Phase5 と対応する。hpRatio がしきい値を下回ったら次のフェーズへ。

	Phase newPhase = Phase::Phase1;

	for (size_t i = 0; i < hpPhaseThresholds_.size(); ++i)
	{
		float threshold = hpPhaseThresholds_[i];
		if (hpRatio <= threshold)
		{
			newPhase = static_cast<Phase>(static_cast<int>(Phase::Phase1) + static_cast<int>(i));
		}
	}

	if (newPhase != phase_)
	{
		phase_ = newPhase;
		if (camera_)
		{
			if (cameraShakeCooldown_ <= 0.0f)
			{
				camera_->StartShake(0.3f, 0.3f);
				cameraShakeCooldown_ = kCameraShakeCooldownSeconds;
			}
		}
	}
}
