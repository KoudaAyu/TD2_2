#include "RailCameraController.h"
#include <cmath>

RailCameraController::RailCameraController()
{
}

RailCameraController::~RailCameraController()
{
}

static inline float DegreesToRadians(float d) { return d * 3.14159265358979323846f / 180.0f; }

void RailCameraController::Initialize(const Vector3& worldPosition, const Vector3& worldRotation)
{
	// 受け取った回転は度数法とみなしてラジアンへ変換
	Vector3 rotationRad = { DegreesToRadians(worldRotation.x), DegreesToRadians(worldRotation.y), DegreesToRadians(worldRotation.z) };

	// ワールドトランスフォームへ設定
	worldTransform_.SetTranslate(worldPosition);
	worldTransform_.SetRotate(rotationRad);
	worldTransform_.TransferMatrix();

	if (camera_)
	{
		camera_->SetTranslate(worldPosition);
		camera_->SetRotate(rotationRad);
		camera_->Initialize(); // アスペクト比設定
		camera_->Update();     // 行列更新
	}
}

void RailCameraController::Update()
{
	if (camera_)
	{
		// レール制御の処理を追加する予定。現状は保持しているトランスフォームを反映
		camera_->SetTranslate(worldTransform_.GetTranslate());
		camera_->SetRotate(worldTransform_.GetRotate());
		camera_->Update();
	}
}


