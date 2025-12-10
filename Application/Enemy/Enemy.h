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
	};

	// 攻撃パターンを外部から設定できるように追加
	enum class AttackPattern
	{
		Straight, // まっすぐ弾を撃つ
		Aim,      // プレイヤー狙い
		Rapid     // 連射・多弾
	};

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
	bool IsCollidable() const { return collidable_ && (invincibilityTimerFrames_ <= 0); }

	// 攻撃パターンの設定
	void SetAttackPattern(AttackPattern p) { attackPattern_ = p; }
	// 発射間隔を外から上書き（0 でデフォルト）
	void SetFireInterval(int interval) { fireIntervalOverride_ = interval; }
	// 弾速を設定
	void SetBulletSpeed(float sp) { bulletSpeed_ = sp; }
	// ランダムに初期発射オフセットを作るかどうか
	void SetRandomizeInitialFire(bool v) { randomizeInitialFireOffset_ = v; }
	// 初回発射までの個別遅延（フレーム）を設定
	void SetInitialFireDelay(int frames) { initialFireDelay_ = frames; }
	// 離脱で非アクティブ化する Z と半径のしきい値を設定
	void SetLeaveDeactivateZ(float z) { leaveDeactivateZ_ = z; }
	void SetLeaveDeactivateRadius(float r) { leaveDeactivateRadius_ = r; }
	// 離脱フェーズのパーティクルを出すかどうか
	void SetLeaveEmitParticles(bool v) { leaveEmitParticles_ = v; }

	// Rapid パラメータの調整
	void SetRapidSpread(float s) { rapidSpread_ = s; }
	void SetRapidShotCount(int c) { rapidShotCount_ = c; }

	// 出現時のY座標ランダム化を設定
	void SetRandomizeSpawnY(bool v) { randomizeSpawnY_ = v; }
	void SetSpawnYRandomRange(float r) { spawnYRandomRange_ = r; }

	// 爆発演出と近接ダメージ（1回のみ）
	void ExplodeOnceAndDamagePlayer();

private:
	
	//フェーズ
	Phase phase_ = Phase::Spawn;
	//接近時の速度
	Vector3 approachVelocity = { 0.0f, 0.0f, -0.06f };
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
	bool leaveEmitParticles_ = false; // 離脱時のパーティクル無効化（初期は出さない）

	// 派手に逃がす（エスケープ）演出用
	float leaveEscapeTimer_ = 0.0f;       // 経過秒
	float leaveEscapeDuration_ = 0.6f;    // エスケープ演出の長さ（秒）
	bool leaveEscapeStarted_ = false;     // エスケープ演出中フラグ

	// 出現モーション用
	Vector3 spawnStartPos_ = { 0.0f, 5.0f, 15.0f };
	Vector3 spawnTargetPos_ = { 0.0f, 0.0f, 10.0f };
	int32_t spawnTimer_ = 0;
	int32_t spawnDuration_ = 60; // フレーム数での出現時間

	// 弾関係
	//発射間隔
	static const int kFireInterval = 60; 
	//発射タイマー
	int32_t fireTimer_ = 0;
	// 上書き可能な発射間隔
	int fireIntervalOverride_ = 0;

	// 攻撃パターン
	AttackPattern attackPattern_ = AttackPattern::Straight;

	// 弾の速さ（外部から変更可）
	float bulletSpeed_ = 2.0f; 

	// ランダム化フラグ（初期発射オフセットをランダム化する）
	bool randomizeInitialFireOffset_ = false;

	// 初回発射までの個別遅延（フレーム）。-1なら使用しない
	int initialFireDelay_ = -1;

	// 離脱フェーズで非アクティブ化するしきい値（調整用）
	float leaveDeactivateZ_ = -60.0f;      // 画面奥に十分行ってから
	float leaveDeactivateRadius_ = 140.0f; // スパイラルが十分広がってから

	// エスケープ時の開始スケールと色（フェード用）
	Vector3 leaveStartScale_ = {1.0f, 1.0f, 1.0f};
	Vector4 leaveStartColor_ = {1.0f, 1.0f, 1.0f, 1.0f};

	// Rapid 設定（デフォルトは既存挙動）
	float rapidSpread_ = 0.22f;
	int rapidShotCount_ = 3;

	// 発射前のチャージ表現フラグ
	bool chargeEmitted_ = false;
	static constexpr int kChargeFrames = 8; // 発射前の予告フレーム数
	int chargeTimer_ = 0;

	// Approach phase visual base state (for pulsing charge)
	Vector3 approachBaseScale_ = {1.0f, 1.0f, 1.0f};
	Vector4 approachBaseColor_ = {1.0f, 1.0f, 1.0f, 1.0f};

	// How long the visual tint/pulse remains (frames) after charge
	static constexpr int kChargeVisualFrames = 20; // default hold frames
	int chargeVisualTimer_ = 0;

	// temporary invincibility after spawn->approach transition (frames)
	int invincibilityTimerFrames_ = 0;
	static constexpr int kSpawnInvincibilityFrames = 30; // configurable small i-frames

	// 出現時のY座標ランダム化設定
	bool randomizeSpawnY_ = true;
	float spawnYRandomRange_ = 2.5f; // +- range in world units

	// 前方境界到達時に爆発を既に実行したか
	bool explodedOnce_ = false;

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