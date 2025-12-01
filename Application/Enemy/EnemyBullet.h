#pragma once

#include"Camera.h"
#include"Object3d.h"
#include"Object3dCom.h"
#include"Transform.h"

class Player;

class EnemyBullet
{
public:
	EnemyBullet() = default;
	~EnemyBullet();
	void Initialize(Object3d* model, const Vector3 pos, Object3dCom* object3dCom, const Vector3& velocity);
	void Update();
	void Draw();


	/// <summary>
	/// 衝突処理
	/// </summary>
	void OnCollision();

	// 追従（ホーミング）を有効化
	void EnableHoming(Player* target, float speed, float turnRate)
	{
		target_ = target;
		speed_ = speed;
		turnRate_ = turnRate;
		isHoming_ = (target_ != nullptr);
	}

	// 寿命（フレーム数）を設定。0 以下なら無制限。
	void SetLifeDuration(int frames) { lifeDuration_ = frames; lifeTimer_ = 0; }

public:
	bool IsActive() const { return isActive_; }

	Vector3 GetWorldTranslate() const
	{
		return worldTransform_.GetTranslate();
	};

	const Matrix4x4& GetWorldMatrix() const
	{
		return worldTransform_.GetWorldMatrix();
	};
	

private:
	Vector3 velocity_ = { 0.0f, 0.0f, -1.0f };
	float speed_ = 1.0f;
	float turnRate_ = 0.05f; // 方向の補正係数（0~1）
	bool isHoming_ = false;
	Player* target_ = nullptr;

	// 寿命管理
	int lifeTimer_ = 0;
	int lifeDuration_ = 0; // 0: 無制限。>0 でそのフレーム数で消滅。

	bool isActive_ = true;
private:
	Camera* camera_ = nullptr;
	Transform worldTransform_ = {};
	Object3d* model_ = nullptr;
	Object3dCom* object3dCom_ = nullptr;

};
