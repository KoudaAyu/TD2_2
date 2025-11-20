#pragma once
#include"Camera.h"
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

private:

	Camera* camera_ = nullptr;
	Object3d* model_ = nullptr;
	Object3dCom* object3dCom_ = nullptr;
	Player* player_ = nullptr;
	RailCameraController* railCameraController_ = nullptr;
};
