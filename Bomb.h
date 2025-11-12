#pragma once
#include"AABB.h"
#include"Camera.h"
#include"Object3d.h"
#include"Vector.h"

class MapChipField;
class Player;

class Bomb
{
public:

	Bomb() = default;
	~Bomb();

	void Initialize(Object3d* model,Camera* camera,const Vector3 pos);
	void Update();
	void Draw();

	void Respawn(MapChipField* map, const Player& player);
	void UpdateAABB();

public:
	const Vector3& GetPosition() const { return worldTransform_.translation_; }
	void SetPosition(const Vector3& pos) { worldTransform_.translation_ = pos; }
	const AABB& GetAABB() const { return bombAABB; }
	bool IsAlive() const { return isAlive_; }
	void SetAlive(const bool isAlive) { isAlive_ = isAlive; }
	Object3d* GetObject3d() const { return model_; }
private://変更禁止
	bool isAlive_ = true;
	uint32_t textureHandle = 0u;
	Transform worldTransform_{};
	Object3d* model_ = nullptr;
	Camera* camera_ = nullptr;
	
	AABB bombAABB;

};