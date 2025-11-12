#pragma once

#include <string>  
#include <cmath>

#include "Vector.h"
#include "KeyInput.h"
#include "Fade.h"

#include"SoundManager.h"

class WinApp;
class Object3dCom;
class Object3d;
class Model;
class SpriteCom;
class Sprite;
class Camera;

class TitleScene
{
public:
	TitleScene() = default;
	~TitleScene();

	void Initialize(Object3dCom* object3dCom, SpriteCom* spriteCom = nullptr);
	void Update();
	void Draw();

	bool IsFinished() const { return finished_; }

	void SetWinApp(WinApp* winApp) { winApp_ = winApp; }

private:
	void LoadAndGetModel(Model*& outModel, const std::string& filePath);

	Object3d* makeObject(Object3dCom* object3dCom, Model* model,
		const Vector3& pos, const Vector3& scale);

	static float EaseOutBack(float t);

private:
	WinApp* winApp_ = nullptr;
	Object3dCom* object3dCom_ = nullptr;
	SpriteCom* spriteCom_ = nullptr;
	Sprite* spaceOrA_ = nullptr;
    Sprite *background_ = nullptr;

	Camera* camera_ = nullptr;

	Object3d* titleTextModel_ = nullptr;

	float timer_ = 0.0f;         // 進行
	float easeDuration_ = 120.0f; // 60フレームで停止（=1秒想定・好みで調整）

	Vector3 startScale_{ 0.0f, 0.0f, 0.0f };
	Vector3 endScale_{ 1.0f, 1.0f, 1.0f };

	Vector3 startPos_{ 0.0f, 12.0f, 0.0f }; // 画面下から入ってくる開始位置
	Vector3 endPos_{ 1.0f, 5.0f, 0.0f };    // 最終位置（あなたの今の配置）

	// Idle 演出用
	float totalTime_ = 0.0f;   // 経過時間（秒換算でもフレーム換算でもOK）
	bool idle_ = false;        // イージング終了後に true
	float bobAmp_ = 0.30f;     // 上下ゆれの振幅
	float bobHz_ = 2.0f;       // 上下ゆれの周波数(Hz)
	float swayAmp_ = 0.08f;    // 首振り角度(ラジアン)
	float swayHz_ = 1.3f;      // 首振りの周波数
	float breatheAmp_ = 0.04f; // 呼吸スケール振幅
	float breatheHz_ = 2.4f;   // 呼吸周波数

	bool isGameStart_ = false;
	Vector3 t = { 0.0f, 0.0f, 0.0f };

	Fade fade_;
	bool fadeStarted_ = false;

	SoundData titleBGM_;
	SoundData decideSE_;  // 決定音


private:
	// Titleシーンを終了する
	bool finished_ = false;
};
