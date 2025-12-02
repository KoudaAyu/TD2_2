#pragma once

#include"Camera.h"
#include"Fade.h"
#include"KeyInput.h"
#include"Object3d.h"
#include"Object3dCom.h"
#include"Sprite.h"
#include"SpriteCom.h"

class TitleScene
{
public:

	enum class Phase
	{
		kFadeIn,
		kMain,
		kFadeOut,
	};

	TitleScene() = default;
	~TitleScene();
	void Initialize(Camera* camera, Object3dCom* object3dCom, SpriteCom* spriteCom);
	void Update();
	void Draw();

public:
	bool IsFinish() const { return isFinish_; }

private:
	bool isFinish_ = false;

	// spaceOrA明滅用
	const float kDeltaTime = 1.0f / 60.0f;
	float elapsedTime_ = 0.0f;          // 経過時間
	const float duration_ = 3.0f;       // 片道(0~1や1~0)の長さ
	// 0~1,1~0を繰り返すための計算をする関数
	float CalculatePingPongTime(float elapsedTime, float duration);
	// 明滅させる関数
	void SpaceOrAPingPong();

	// spaceOrAのリアクション用
	float elapsedReactionTime_ = 0.0f;     // 経過時間
	// 決定キーを押した際のspaceOrAのリアクション
	void PushKeyReaction();

private:
	Camera* camera_ = nullptr;

	Fade* fade_ = nullptr;

	KeyInput* keyInput_ = nullptr;

	Phase phase_ = Phase::kFadeIn;

	Object3d* titleTextModel_ = nullptr;

	Object3dCom* object3dCom_ = nullptr;

	Sprite* backGroundSprite_ = nullptr;
	Sprite* spaceOrASprite_ = nullptr;

	SpriteCom* spriteCom_ = nullptr;
};