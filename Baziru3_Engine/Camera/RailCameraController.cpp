#include "RailCameraController.h"
#include "../../Application/Player/Player.h"
#include <cmath>

RailCameraController::RailCameraController()
{
}

RailCameraController::~RailCameraController()
{
}

static inline float DegreesToRadians(float d) { return d * 3.14159265358979323846f / 180.0f; }

// 線形補間
static inline Vector3 Lerp(const Vector3& a, const Vector3& b, float t)
{
    return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t };
}

void RailCameraController::Initialize(const Vector3& worldPosition, const Vector3& worldRotation)
{
    // 受け取った回転は度数法とみなしてラジアンへ変換
    Vector3 rotationRad = { DegreesToRadians(worldRotation.x), DegreesToRadians(worldRotation.y), DegreesToRadians(worldRotation.z) };

    // ワールドトランスフォームへ設定
    worldTransform_.SetTranslate(worldPosition);
    worldTransform_.SetRotate(rotationRad);
    worldTransform_.TransferMatrix();

    if (camera_)
    {
        camera_->SetTranslate(worldPosition);
        camera_->SetRotate(rotationRad);
        camera_->Initialize(); // アスペクト比設定
        camera_->Update();     // 行列更新
    }
}

void RailCameraController::Update()
{
    if (camera_)
    {
        if (target_)
        {
            // ターゲットのワールド座標を取得
            Vector3 targetPos = target_->GetWorldTranslate();

            // 初回オフセット計算: offset_ がゼロベクトルなら現在のカメラ位置から計算する
            if (offset_.x == 0.0f && offset_.y == 0.0f && offset_.z == 0.0f)
            {
                Vector3 camPos = worldTransform_.GetTranslate();
                offset_.x = camPos.x - targetPos.x;
                offset_.y = camPos.y - targetPos.y;
                offset_.z = camPos.z - targetPos.z;
            }

            // 目的地としてターゲット位置+オフセットを設定し、追従する
            Vector3 desired = { targetPos.x + offset_.x, targetPos.y + offset_.y, targetPos.z + offset_.z };
            Vector3 current = worldTransform_.GetTranslate();
            // 補間係数（滑らかさ）
            const float kLerp = 0.15f;
            Vector3 next = Lerp(current, desired, kLerp);

            worldTransform_.SetTranslate(next);
            worldTransform_.TransferMatrix();

            camera_->SetTranslate(worldTransform_.GetTranslate());
            camera_->SetRotate(worldTransform_.GetRotate());
            camera_->Update();
        }
        else
        {
            // レール制御の処理を追加する予定。現状は保持しているトランスフォームを反映
            camera_->SetTranslate(worldTransform_.GetTranslate());
            camera_->SetRotate(worldTransform_.GetRotate());
            camera_->Update();
        }
    }
}



