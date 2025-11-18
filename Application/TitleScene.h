#pragma once

#include"Fade.h"
#include"KeyInput.h"

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
	void Initialize(SpriteCom* spriteCom);
	void Update();
	void Draw();

public:
	bool IsFinish() const { return isFinish_; }

private:
	bool isFinish_ = false;

private:
	Fade* fade_ = nullptr;

	KeyInput* keyInput_ = nullptr;

	Phase phase_ = Phase::kFadeIn;
};