#pragma once
#include"Camera.h"
#include"Object3d.h"
#include"Object3dCom.h"
#include"Transform.h"

class Enemy
{
public:
	Enemy() = default;
	~Enemy();
	void Initialize(Object3d* model, Camera* camera, const Vector3 pos, Object3dCom* object3dCom);
	void Update();
	void Draw();

private:
	Vector3 velocity = { 0.0f, 0.0f, -0.5f };

private:
	Camera* camera_ = nullptr;
	Transform worldTransform_ = {};
	Object3d* model_ = nullptr;
	Object3dCom* object3dCom_ = nullptr;	
};