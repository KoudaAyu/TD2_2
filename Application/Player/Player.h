#pragma once
#include"AABB.h"
#include"Camera.h"
#include"KeyInput.h"
#include"MathUtl.h"
#include"Object3d.h"
#include"Object3dCom.h"
#include"PlayerBarrier.h"
#include"SoundManager.h"

#include <vector>

class Controller; // forward declaration for controller pointer

class Player
{
public:
	Player() = default;
	~Player();
	void Initialize(Object3d* model, Camera* camera, const Vector3 pos, Object3dCom* object3dCom);
	void Update();
	void Draw();

	void Move();

	/// <summary>
	/// 移動制限
	/// </summary>
	void MoveLimit();

	/// <summary>
	/// 旋回
	/// </summary>
	void Rotate();

	/// <summary>
	/// バリア関係
	/// </summary>
	void Barrier();

	/// <summary>
	/// 衝突処理
	/// </summary>
	void OnCollision();

	// allow external code to enable/disable firing
	void SetCanFire(bool v) { canFire_ = v; }

	// allow external code to enable/disable movement (used during fade etc.)
	void SetCanMove(bool v) { canMove_ = v; }

	// set current wave id for newly spawned barriers
	void SetCurrentWaveForBarriers(int wave) { currentWaveForBarriers_ = wave; }

#ifdef USE_IMGUI
	
	void DrawImGui();
#endif

public:

	// 生存状態のgetter/setter
	bool IsAlive() const { return isAlive_; }
	void SetAlive(bool isAlive) { isAlive_ = isAlive; }

	// invincibility getter
	bool IsInvincible() const { return invincible_; }

	
	Vector3 GetWorldTranslate() const
	{
		return worldTransform_.GetTranslate();
	};

	// ワールド行列のgetter（参照で返す）
	const Matrix4x4& GetWorldMatrix() const
	{
		return worldTransform_.GetWorldMatrix();
	};

	// ワールド回転のgetter
	const Vector3& GetWorldRotate() const
	{
		return worldTransform_.GetRotate();
	}

	// バリア群を取得（複数化対応）
	const std::vector<PlayerBarrier*>& GetBarriers() const { return barriers_; }

	// remove and delete all active barriers immediately
	void ClearBarriers();

private:

	bool isAlive_ = true;


private:
	Transform worldTransform_ = {};

	AABB aabb_ = {};

	KeyInput* keyInput_ = nullptr;

	Object3d* model_ = nullptr;

	Camera* camera_ = nullptr;

	Object3dCom* object3dCom_ = nullptr;

	Controller* controller_ = nullptr; // added controller pointer

	// 単一のバリアから複数のバリアに変更
	std::vector<PlayerBarrier*> barriers_;

	// --- 移動関連 ---
	// 移動制限（外部でも参照するためクラス定数として定義）
	static constexpr float kMoveLimitX = 6.0f;
	static constexpr float kMoveLimitY = 4.0f;

	// 衝突時のカメラ振動パラメータ
	static constexpr float kCollisionShakeAmplitude = 0.6f; // ワールド単位
	static constexpr float kCollisionShakeDuration = 0.5f;  // 秒

	// whether player is allowed to fire barriers
	bool canFire_ = true;

	// whether player is allowed to move (used to disable movement during fade)
	bool canMove_ = true;

	// --- controller vibration on damage ---
	// duration in seconds for controller vibration when player is hit
	static constexpr float kCollisionVibrationDuration = 0.25f; // seconds (reduced)
	// motor amplitude (0..1)
	static constexpr float kCollisionVibrationAmplitude = 0.35f; // reduced amplitude
	// remaining vibration timer (seconds)
	float controllerVibrationTimer_ = 0.0f;

	// --- model tilt when moving vertically ---
	// maximum tilt angle (radians) applied when moving fully up/down
	static constexpr float kMaxTiltAngle = 0.18f; // ~10 degrees
	// smoothing factor for tilting (0..1)
	static constexpr float kTiltSmoothing = 0.15f;

	// --- invincibility frames ---
	bool invincible_ = false;
	// seconds remaining for invincibility
	float invincibleTimer_ = 0.0f;
	static constexpr float kInvincibleDuration = 1.2f; // seconds of i-frames
	// blink period while invincible
	static constexpr float kInvincibleBlinkPeriod = 0.12f;

	
	SoundManager* soundManager_;
	SoundData shotBarrierSoundData_;

	// track the current wave id so newly spawned barriers know which wave they belong to
	int currentWaveForBarriers_ = 0;

	// --- damage / life ---
	int hitCount_ = 0; // number of times player has been hit
	static constexpr int kMaxHits = 5; // after this many hits player dies
};
