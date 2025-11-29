#pragma once

#include<list>

#include"Camera.h"
#include"EnemyBullet.h"
#include"Object3d.h"
#include"Object3dCom.h"
#include"Transform.h"

class Player;

class Enemy
{
public:

	enum class Phase
	{
		Spawn, // 出現モーション
		Approach,//接近する
		Leave,//離脱する
	};;


public:

	Enemy() = default;
	~Enemy();
	void Initialize(Object3d* model, Camera* camera, const Vector3 pos, Object3dCom* object3dCom);
	void Update();
	void Draw();

	/// <summary>
	/// 衝突処理
	/// </summary>
	void OnCollision();

	/// <summary>
	/// 弾発射
	/// </summary>
	void Fire();

	void AimBullet();

	//フェーズごとの更新

	/// <summary>
	/// 出現フェーズの初期化
	/// </summary>
	void SpawnInitialize(const Vector3& targetPos);

	/// <summary>
	/// 出現フェーズの更新
	/// </summary>
	void SpawnUpdate();

	/// <summary>
	/// 接近フェーズの初期化
	/// </summary>
	void ApproachInitialize();

	/// <summary>
	/// 接近フェーズの更新
	/// </summary>
	void ApproachUpdate();

	/// <summary>
	/// 離脱フェーズの初期化
	/// </summary>
	void LeaveInitialize();

	/// <summary>
	/// 離脱フェーズの更新
	/// </summary>
	void LeaveUpdate();

public:
	void SetPlayer(Player* player) { player_ = player; } 	

	const Matrix4x4& GetWorldMatrix() const
	{
		return worldTransform_.GetWorldMatrix();
	};

	Vector3 GetWorldTranslate() const { return worldTransform_.GetTranslate(); }

	const std::list<EnemyBullet*>& GetBullets() const { return bullets_; }

	// 敵の生存/アクティブ状態
	bool IsActive() const { return isActive_; }
	// 離脱中など当たり判定を無効にする場合に使用
	bool IsCollidable() const { return collidable_; }

private:
	
	//フェーズ
	Phase phase_ = Phase::Spawn;
	//接近時の速度
	Vector3 approachVelocity = { 0.0f, 0.0f, -0.2f };
	//離脱時の速度 (初期は0、離脱開始時に設定する)
	Vector3 leaveVelocity = { 0.0f, 0.0f, -0.2f };

	// 離脱時のスパイラル演出パラメータ
	float leaveAngle_ = 0.0f;
	float leaveAngularVel_ = 0.0f;
	float leaveRadius_ = 0.0f;
	float leaveRadialSpeed_ = 0.0f;
	Vector3 leaveCenter_ = { 0.0f, 0.0f, 0.0f };

	// 離脱時のパーティクル発生 (秒)
	float leaveParticleTimer_ = 0.0f;
	float leaveParticleInterval_ = 0.08f; // 0.08s 間隔で小さなバースト

	// 出現モーション用
	Vector3 spawnStartPos_ = { 0.0f, 5.0f, 15.0f };
	Vector3 spawnTargetPos_ = { 0.0f, 0.0f, 10.0f };
	int32_t spawnTimer_ = 0;
	int32_t spawnDuration_ = 60; // フレーム数での出現時間

	//弾関係
	//発射間隔
	static const int kFireInterval = 20;
	//発射タイマー
	int32_t fireTimer_ = 0;

private:
	Camera* camera_ = nullptr;

	std::list<EnemyBullet*> bullets_;
	Transform worldTransform_ = {};
	Object3d* model_ = nullptr;
	Object3dCom* object3dCom_ = nullptr;

	Player* player_ = nullptr;

	// 生存フラグ。衝突などで false にする。
	bool isActive_ = true;
	// 当たり判定を許すかどうか（離脱中は false にする）
	bool collidable_ = true;
};