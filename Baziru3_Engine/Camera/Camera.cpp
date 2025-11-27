#include "Camera.h"
#include"WinApp.h"

/// <summary>
/// コンストラクタ
/// </summary>
Camera::Camera()
	: transform_({ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} }),
	fovY_(0.45f),
	aspectRatio_(1.0f), // 実際のサイズに応じてInitializeで設定
	nearZ_(0.1f), farZ_(100.0f),
	worldMatrix_(MakeAffineMatrix(transform_.GetScale(), transform_.GetRotate(),
		transform_.GetTranslate())),
	viewMatrix_(Inverse(worldMatrix_)),
	projectionMatrix_(MakePerspectiveFovMatrix(fovY_, aspectRatio_, nearZ_, farZ_)),
	viewProjectionMatrix_(Multiply(viewMatrix_, projectionMatrix_))
{
}

/// <summary>
/// 初期化
/// </summary>
void Camera::Initialize()
{
	// ウィンドウサイズからアスペクト比を設定
	aspectRatio_ = float(WinApp::kClientWidth) / float(WinApp::kClientHeight);
	// 行列更新
	Update();
}

void Camera::Update()
{
	transform_.SetRotate(rotation_);
	transform_.SetTranslate(translation_);

	// transformからアフィン変換行列を計算
	worldMatrix_ = MakeAffineMatrix(transform_.GetScale(), transform_.GetRotate(),
		transform_.GetTranslate());
	// worldMatrixの逆行列
	viewMatrix_ = Inverse(worldMatrix_);
	//透視投影行列の生成
	projectionMatrix_ =
		MakePerspectiveFovMatrix(fovY_, aspectRatio_, nearZ_, farZ_);
	//合成行列
	viewProjectionMatrix_ = Multiply(viewMatrix_, projectionMatrix_);
}