#pragma once
#include"AABB.h"
#include"Camera.h"
#include"KeyInput.h"
#include"Object3d.h"
#include"Object3dCom.h"
#include"PlayerBarrier.h"
#include"MathUtl.h"
#include <vector>

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

private:

	bool isAlive_ = true;


private:
	Transform worldTransform_ = {};

	AABB aabb_ = {};

	KeyInput* keyInput_ = nullptr;

	Object3d* model_ = nullptr;

	Camera* camera_ = nullptr;

	Object3dCom* object3dCom_ = nullptr;

	// 単一のバリアから複数のバリアに変更
	std::vector<PlayerBarrier*> barriers_;

	// --- ジャンプ関連変数 ---
	// 重力（フレームごとに垂直速度へ加算）
	static constexpr float kGravity = -0.035f;
	// 一段目のジャンプ初速（低めに設定）
	static constexpr float kFirstJumpVelocity = 0.65f;
	// 二段目のジャンプ初速
	static constexpr float kSecondJumpVelocity = 0.9f;
	// 最大二段ジャンプ
	static constexpr int kMaxJumpCount = 2;

	// 移動制限（外部でも参照するためクラス定数として定義）
	static constexpr float kMoveLimitX = 6.0f;
	static constexpr float kMoveLimitY = 4.0f;

	// 衝突時のカメラ振動パラメータ
	static constexpr float kCollisionShakeAmplitude = 0.6f; // ワールド単位
	static constexpr float kCollisionShakeDuration = 0.5f;  // 秒

	// 現在の垂直速度
	float verticalVelocity_ = 0.0f;
	// 現在のジャンプ回数（着地でリセット）
	int jumpCount_ = 0;
};
