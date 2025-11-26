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
	// ImGui用のウィンドウ描画。ImGuiManager::Begin() と End() の間で呼び出してください。
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

	const PlayerBarrier* GetBarrier() const { return barrier_; }


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
};
