#pragma once

#include "Fade.h"
#include "KeyInput.h"
#include "Sprite.h"
#include "SpriteCom.h"


class Object3d;
class Object3dCom;
class Camera;

class ClearScene {
public:
    enum class Phase {
        kFadeIn,
        kMain,
        kFadeOut,
    };

    ClearScene() = default;
    ~ClearScene();


    void Initialize(SpriteCom* spriteCom);
    void Update();
    void Draw();

    bool IsFinish() const { return isFinish_; }

private:
    // spaceOrA明滅用
    const float kDeltaTime = 1.0f / 60.0f;
    float elapsedTime_ = 0.0f;          // 経過時間
    const float duration_ = 3.0f;       // 片道(0~1や1~0)の長さ
    // 0~1,1~0を繰り返すための計算をする関数
    float CalculatePingPongTime(float elapsedTime, float duration);
    // 明滅させる関数
    void SpaceOrAPingPong();

    // spaceOrAのリアクション用
    float elapsedReactionTime_ = 0.0f;     // 経過時間
    // 決定キーを押した際のspaceOrAのリアクション
    void PushKeyReaction();

    bool isFinish_ = false;
    Fade* fade_ = nullptr;
    KeyInput* keyInput_ = nullptr;
    Phase phase_ = Phase::kFadeIn;

    Sprite* backGroundSprite_ = nullptr;
    Sprite* clearTextSprite_ = nullptr;
    Sprite* spaceOrASprite_ = nullptr;

    SpriteCom* spriteCom_ = nullptr;
};
