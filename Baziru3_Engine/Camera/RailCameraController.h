#pragma once

#include"Camera.h"
#include"Transform.h"

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

private:
	Camera* camera_ = nullptr;
	Transform worldTransform_ = {};
};