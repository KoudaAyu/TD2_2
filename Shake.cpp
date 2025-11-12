#include "Shake.h"
#include "Random.h"

/// <summary>
/// 初期化
/// </summary>
void Shake::Initialize()
{
	//シェイク用のタイマーをリセット
	shakeTimer_ = 0.0f;
	position_ = { 0.0f, 0.0f };
}

void Shake::Start(const float& min, const float& max, const float& shakeTime)
{
	min_ = min;
	max_ = max;
	shakeTime_ = shakeTime;
	isShaking_ = true;
	shakeTimer_ = 0.0f;
}

/// <summary>
/// 更新
/// </summary>
void Shake::Update()
{
	if (isShaking_)
	{
		shakeTimer_++;

		if (shakeTimer_ >= shakeTime_)
		{
			position_ = { 0.0f, 0.0f };
			isShaking_ = false;
		}

		float offsetX = Random::GeneraterFloat(min_, max_);
		float offsetY = Random::GeneraterFloat(min_, max_);

		// 揺れの強さを減衰させる（経過時間に応じて）
		float t = shakeTimer_ / shakeTime_;
		float power = (1.0f - t) * 0.07f; // 最大5ピクセル → 徐々に減衰

		position_.x = offsetX * power;
		position_.y = offsetY * power;
	}

}