#pragma once

#include"Camera.h"
#include"Transform.h"

class Player; // forward declare

class RailCameraController
{
	public:
	RailCameraController();
	~RailCameraController();
	// ワールド座標(平行移動)と回転角を受け取り初期化
	void Initialize(const Vector3& worldPosition, const Vector3& worldRotation);
	void Update();
	// カメラの設定
	void SetCamera(Camera* camera) { camera_ = camera; }
	// 追従対象を設定
	void SetTarget(Player* target) { target_ = target; }

private:
	Camera* camera_ = nullptr;
	Transform worldTransform_ = {};
	// カメラのオフセット（ターゲットからの相対位置）
	Vector3 offset_{0.0f, 0.0f, 0.0f};
	// 追従対象
	Player* target_ = nullptr;
};