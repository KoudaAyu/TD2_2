#pragma once
#include"AABB.h"
#include"Camera.h"
#include"KeyInput.h"
#include"Object3d.h"
#include"Object3dCom.h"
#include"PlayerBarrier.h"
#include"MathUtl.h"
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

#ifdef USE_IMGUI
	
	void DrawImGui();
#endif

public:

	// 生存状態のgetter/setter
	bool IsAlive() const { return isAlive_; }
	void SetAlive(bool isAlive) { isAlive_ = isAlive; }

	
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

};
