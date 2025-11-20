#include "Player.h"
#include<algorithm>
#include<cassert>
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

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
    assert(keyInput_ && "KeyInput::Initialize() が呼ばれていません。mainで生成&初期化してください。");

    // 初期Transform（スケール0で不可視にならないように 1 を設定）
    worldTransform_.Initialize();
	worldTransform_.SetTranslate(pos);

    if (model_)
    {
        model_->ApplyState(worldTransform_, camera_, true);
    }
}



void Player::Update()
{
    //Playerが死亡している場合早期return
    if (!isAlive_)
    {
        return;
    }

    Move();

    MoveLimit();

    // ワールド行列の更新（必要なら維持）
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

void Player::Move()
{
    Vector3 move = { 0.0f,0.0f,0.0f };
    const float kCharacterSpeed = 0.2f;

    if (keyInput_->IsKeyPressed(DIK_A))
    {
        move.x -= kCharacterSpeed;
    }

    if (keyInput_->IsKeyPressed(DIK_D))
    {
        move.x += kCharacterSpeed;
    }

    if (keyInput_->IsKeyPressed(DIK_W))
    {
        move.y += kCharacterSpeed;
    }
    if (keyInput_->IsKeyPressed(DIK_S))
    {
        move.y -= kCharacterSpeed;
    }

    worldTransform_.SetTranslate(worldTransform_.GetTranslate() + move);
}

void Player::MoveLimit()
{
    const float kMoveLimitX = 6.0f;
    const float kMoveLimitY = 4.0f;

    // 位置を取得し、範囲を超えないように clamp して戻す
    Vector3 t = worldTransform_.GetTranslate();
    t.x = std::clamp(t.x, -kMoveLimitX, kMoveLimitX);
    t.y = std::clamp(t.y, -kMoveLimitY, kMoveLimitY);
    worldTransform_.SetTranslate(t);
}

#ifdef USE_IMGUI
void Player::DrawImGui()
{
    if (!ImGui::Begin("Player"))
    {
        ImGui::End();
        return;
    }

    // 基本情報表示
    ImGui::Text("Alive: %s", isAlive_ ? "true" : "false");

    Vector3 translate = worldTransform_.GetTranslate();
    if (ImGui::DragFloat3("Translate", &translate.x, 0.1f))
    {
        worldTransform_.SetTranslate(translate);
        worldTransform_.TransferMatrix();
        if (model_)
        {
            model_->ApplyState(worldTransform_, camera_, true);
        }
    }

    ImGui::End();
}
#endif

