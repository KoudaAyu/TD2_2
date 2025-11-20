#pragma once
#include"AABB.h"
#include"Camera.h"
#include"KeyInput.h"
#include"Object3d.h"
#include"Object3dCom.h"
#include"Transform.h"

class Player
{
public:
	Player() = default;
	~Player();
	void Initialize(Object3d* model, Camera* camera, const Vector3 pos, Object3dCom* object3dCom);
	void Update();
	void Draw();

	void Move();

	/// <summary>
	/// 移動制限
	/// </summary>
	void MoveLimit();


#ifdef USE_IMGUI
	// ImGui用のウィンドウ描画。ImGuiManager::Begin() と End() の間で呼び出してください。
	void DrawImGui();
#endif

public:

	// 生存状態のgetter/setter
	bool IsAlive() const { return isAlive_; }
	void SetAlive(bool isAlive) { isAlive_ = isAlive; }

private:

	bool isAlive_ = true;


private:
	Transform worldTransform_ = {};

	AABB aabb_ = {};

	KeyInput* keyInput_ = nullptr;

	Object3d* model_ = nullptr;

	Camera* camera_ = nullptr;

	Object3dCom* object3dCom_ = nullptr;
};
