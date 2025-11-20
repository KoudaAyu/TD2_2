#pragma once
#include"Camera.h"
#include"DebugCamera.h"
#include"Enemy.h"
#include"KeyInput.h"
#include"Object3d.h"
#include"Object3dCom.h"
#include"Player.h"
#include"RailCameraController.h"

class GameScene
{
public:
	GameScene() = default;
	~GameScene();
	void Initialize(Camera* camera, Object3dCom* object3dCom);
	void Update();
	void Draw();

public:
	bool IsFinish() const { return isFinish_; }

private:

	bool isFinish_ = false;
	bool isDebugCameraActive_ = false;

private:

	Camera* camera_ = nullptr;
	DebugCamera* debugCamera_ = nullptr;
	Enemy* enemy_ = nullptr;
	KeyInput* keyInput_ = nullptr;
	Object3d* model_ = nullptr;
	Object3d* enemyModel_ = nullptr;
	Object3dCom* object3dCom_ = nullptr;
	Player* player_ = nullptr;
	RailCameraController* railCameraController_ = nullptr;


};
