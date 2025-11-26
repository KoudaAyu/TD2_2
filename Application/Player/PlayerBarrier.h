#pragma once
#include"Camera.h"
#include"Object3d.h"
#include"Transform.h"

class PlayerBarrier
{
public:
    PlayerBarrier() = default;
    ~PlayerBarrier();
   
    void Initialize(Object3d* model, const Vector3 pos, Object3dCom* object3dCom,const Vector3& velocity);
    void Update();
    void Draw(const Camera& camera);

    /// <summary>
    /// 衝突処理
    /// </summary>
    void OnCollision();

public:
	bool IsActive() const { return isActive_; }

private:
    // バリアの寿命（フレーム数）
	static const int32_t kLifeTime = 60; 
    // バリアの寿命カウント用タイマー
	int32_t deathTimer_ = kLifeTime;
	// バリアのアクティブ状態
    bool isActive_ = true;

private:
    Object3d* model_ = nullptr; 
    Object3dCom* object3dCom_ = nullptr;
    Object3d* barrierModel_ = nullptr; 
    Transform worldTransform = {};

    Vector3 velocity_;

};
