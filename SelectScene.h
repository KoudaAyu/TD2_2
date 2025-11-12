#pragma once

#include "Fade.h"
#include "KeyInput.h"
#include"SoundManager.h"
#include "Vector.h"

class Camera;
class Object3d;
class Object3dCom;
class SpriteCom;
class Model;

class SelectScene
{
public:
	SelectScene() = default;
	~SelectScene();

	void Initialize(Object3dCom* object3dCom, SpriteCom* spriteCom);
	void Update();
	void Draw();

	void LoadAndGetModel(Model*& outModel, const std::string& filePath);
	Object3d* makeObject(Object3dCom* object3dCom, Model* model,
		const Vector3& pos, const Vector3& scale);

	bool IsSelectTutorialScene() const { return Select_TutorialScene; }
	bool IsSelectGameScene() const { return Select_GameScene; }
	bool IsSelectTitleScene() const { return Select_TitleScene; }

private:
	// 遷移フラグ
	bool Select_GameScene = false;
	bool Select_TutorialScene = false;
	bool Select_TitleScene = false;

	// カメラ・モデル
	Camera* camera_ = nullptr;
	Object3d* TutorialModel_ = nullptr;  // index 0
	Object3d* GameSceneModel_ = nullptr; // index 1
	Object3d* TitleBackModel_ = nullptr; // index 2

	//スプライト
	Sprite *spaceOrA_ = nullptr;
    Sprite *background_ = nullptr;

	// 依存
	Object3dCom* object3dCom_ = nullptr;
	SpriteCom* spriteCom_ = nullptr;

	// フェード
	Fade fade_;
	bool fadeStarted_ = false;
	bool bgmStarted_ = false;

	// カメラ（中央固定を見せるシンプル構図）
	Vector3 cameraPosition_{ 5.0f, 0.0f, -30.0f };

	// 選択状態
	int focusedIndex_ = 0; // 0=Tutorial, 1=Game, 2=Title

	// 固定3座標（左・中央・右）
	const Vector3 P_left = { 0.0f, 0.0f, 10.0f };
	const Vector3 P_center = { 5.0f, 3.0f, 0.0f };
	const Vector3 P_right = { 10.0f, 0.0f, 10.0f };

	// スライド用：現在座標と目標座標（毎フレームLerp）
	Vector3 tutPosCur_{ P_center }, tutPosDst_{ P_center };
	Vector3 gamePosCur_{ P_right }, gamePosDst_{ P_right };
	Vector3 backPosCur_{ P_left }, backPosDst_{ P_left };

	float posLerp_ = 0.18f; // 位置補間係数（体感で気持ちよく動く値）
	float snapEps_ = 0.01f; // 目標に十分近ければスナップ

	// スケール演出（中央だけ大きく）
	Vector3 baseScale_{ 1.0f, 1.0f, 1.0f };
	Vector3 focusScale_{ 1.2f, 1.2f, 1.2f };
	Vector3 tutScaleCur_{ 2.0f, 2.0f, 2.0f };
	Vector3 gameScaleCur_{ 0.8f, 0.8f, 0.8f };
	Vector3 backScaleCur_{ 0.8f, 0.8f, 0.8f };

	inline static Vector3 LerpVec(const Vector3& a, const Vector3& b, float t)
	{
		return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t,
				a.z + (b.z - a.z) * t };
	}

	Vector2 lStick = { 0.0f, 0.0f };

	SoundData decideSE_;
	SoundData swipeSE_;
	SoundData selectBGM_;
};
