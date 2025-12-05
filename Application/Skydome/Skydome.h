#pragma once
#include"Camera.h"
#include"Transform.h"
#include"Object3d.h"
#include"Object3dCom.h"

class Skydome
{
public:
	Skydome();
	~Skydome();
	void Initialize(Object3dCom* object3dCom,Camera* camera);
	void Update();
	void Draw();

	// set the rotation speed in radians per second (positive = rotate around Y)
	void SetRotationSpeed(float radPerSec) { rotationSpeed_ = radPerSec; }

private:
	Transform worldTransform_;
	Object3d* model_ = nullptr;
	Object3dCom* object3dCom_ = nullptr;
	Camera* camera_ = nullptr;

	// rotation state (around Y axis)
	float rotationAngle_ = 0.0f;
	float rotationSpeed_ = 0.25f; // rad/s, default slow rotation


};