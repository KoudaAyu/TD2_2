#pragma once

#include"Camera.h"
#include"Object3d.h"
#include"Object3dCom.h"
#include"Transform.h"

class EnemyBullet
{
public:
	EnemyBullet() = default;
	~EnemyBullet();
	void Initialize(Object3d* model, const Vector3 pos, Object3dCom* object3dCom, const Vector3& velocity);
	void Update();
	void Draw();

private:
	Vector3 velocity_ = { 0.0f, 0.0f, -1.0f };
private:
	Camera* camera_ = nullptr;
	Transform worldTransform_ = {};
	Object3d* model_ = nullptr;
	Object3dCom* object3dCom_ = nullptr;

};
