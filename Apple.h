#pragma once

#include"AABB.h"
#include"Camera.h"
#include<cstdint>
#include"Object3d.h"
#include"Vector.h"

class MapChipField;
class Player;

class Apple
{
public:

	Apple() = default;
	~Apple();

	void Initialize(Object3d* model, Camera* camera_, const Vector3 pos);
	void Update();
	void Draw();

	void Respawn(MapChipField* map, const Player& player);

	void UpdateAABB();

public:
	const Vector3& GetPosition() const { return worldTransform_.translation_; }
	void SetPosition(const Vector3& pos) { worldTransform_.translation_ = pos; }
	bool IsAlive() const { return isAlive_; }
	void SetAlive(bool isAlive) { isAlive_ =  isAlive; }
	const AABB& GetAABB() const { return appleAABB; }
	Object3d* GetObject3d() const { return model_; }

private:
	bool isAlive_ = true;

private://変更禁止
	Transform worldTransform_{};

	Camera* camera_ = nullptr;
	Object3d* model_ = nullptr;
	uint32_t textureHandle = 0u;

	AABB appleAABB;
};