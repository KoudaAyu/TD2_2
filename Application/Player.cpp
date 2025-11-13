#include "Player.h"
#include<cassert>

Player::~Player()
{
    keyInput_ = nullptr;
    model_ = nullptr;
    camera_ = nullptr;
    object3dCom_ = nullptr;
}

void Player::Initialize(Object3d* model, Camera* camera, const Vector3 pos, Object3dCom* object3dCom)
{
    // モデル/カメラは外部で生成される想定。nullptr許可にして安全に扱う
    model_ = model;
    camera_ = camera;
    object3dCom_ = object3dCom;

    keyInput_ = KeyInput::GetInstance();

    // 初期Transform（スケール0で不可視にならないように 1 を設定）
    worldTransform_.scale_ = {1.0f,1.0f,1.0f};
    worldTransform_.rotation_ = {0.0f,0.0f,0.0f};
    worldTransform_.translation_ = pos;

    if (model_)
    {
        model_->ApplyState(worldTransform_, camera_, true);
    }
}



void Player::Update()
{
    // ワールド行列の更新（必要なら維持）
    worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
    worldTransform_.TransferMatrix();

    // Object3d 側へ反映し、即時Updateはせずこの後にUpdateを呼ぶ
    if (model_)
    {
        model_->ApplyState(worldTransform_, camera_, false);
        model_->Update();
    }
}

void Player::Draw()
{
    if (model_)
    {
        model_->Draw();
    }
}

