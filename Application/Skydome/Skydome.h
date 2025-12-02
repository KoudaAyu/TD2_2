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

private:
	Transform worldTransform_;
	Object3d* model_ = nullptr;
	Object3dCom* object3dCom_ = nullptr;
	Camera* camera_ = nullptr;


};