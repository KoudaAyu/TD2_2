#pragma once

#include"Camera.h"
#include"Object3dCom.h"
#include"Object3d.h"
#include"Fade.h"
#include"KeyInput.h"
#include "ParticleManager.h"
#include "SoundManager.h"

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
	void Initialize(Camera* camera, Object3dCom* object3dCom,SpriteCom* spriteCom);
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

	Camera* camera_ = nullptr;

	Object3dCom* object3dCom_ = nullptr;
	Object3d* model_ = nullptr;
	// SPACE text model shown below the title
	Object3d* spaceModel_ = nullptr;

	// background sprite
	Sprite* background_ = nullptr;

	float motionTime_ = 0.0f; // モデル動作用タイマー

	// BGM management
	SoundManager* soundManager_ = nullptr;
	SoundData bgmData_{};
	bool hasBgm_ = false;
};