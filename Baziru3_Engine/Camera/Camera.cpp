#include "Camera.h"
#include"WinApp.h"
#include "Random.h"
#include <cmath>

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

void Camera::StartShake(float amplitude, float duration)
{
	if (duration <= 0.0f || amplitude <= 0.0f) return;
	isShaking_ = true;
	shakeAmplitude_ = amplitude;
	shakeDuration_ = duration;
	shakeTimer_ = duration;
	shakeTimeElapsed_ = 0.0f;
	shakeOffset_ = { 0.0f, 0.0f, 0.0f };
}

void Camera::Update()
{
	transform_.SetRotate(rotation_);
	transform_.SetTranslate(translation_);


	const float dt = 1.0f / 60.0f;
	if (isShaking_)
	{
		
		shakeTimeElapsed_ += dt;
		shakeTimer_ -= dt;

		
		float t = (shakeDuration_ > 0.0f) ? (shakeTimer_ / shakeDuration_) : 0.0f;
		if (t < 0.0f) t = 0.0f;

		
		float ax = Random::GeneratorFloat(-1.0f, 1.0f) * shakeAmplitude_ * t;
		float ay = Random::GeneratorFloat(-1.0f, 1.0f) * shakeAmplitude_ * t;
		float az = Random::GeneratorFloat(-1.0f, 1.0f) * shakeAmplitude_ * t;
		shakeOffset_.x = ax;
		shakeOffset_.y = ay;
		shakeOffset_.z = az;

		if (shakeTimer_ <= 0.0f)
		{
			
			isShaking_ = false;
			shakeOffset_ = { 0.0f, 0.0f, 0.0f };
		}
	}
	else
	{
		shakeOffset_ = { 0.0f, 0.0f, 0.0f };
	}


	Vector3 effectiveTranslate = translation_ + shakeOffset_;
	transform_.SetTranslate(effectiveTranslate);

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