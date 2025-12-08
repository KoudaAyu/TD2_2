#pragma once

#include "Fade.h"
#include "KeyInput.h"
#include "SpriteCom.h"


class Object3d;
class Object3dCom;
class Camera;

class SelectScene
{
public:
    enum class Phase
    {
        kFadeIn,
        kMain,
        kFadeOut,
    };

    enum class Choice
    {
        kGame = 0,
        kTutorial = 1,
    };

    SelectScene() = default;
    ~SelectScene();

  
    void Initialize(SpriteCom* spriteCom, Object3dCom* object3dCom, Camera* camera);
    void Update();
    void Draw();

    bool IsFinish() const { return isFinish_; }
    Choice GetChoice() const { return choice_; }

private:
    bool isFinish_ = false;
    Fade* fade_ = nullptr;
    KeyInput* keyInput_ = nullptr;
    Phase phase_ = Phase::kFadeIn;
    Choice choice_ = Choice::kGame; // 常にゲームへ

    // 一度だけプリロードするためのフラグ
    bool preloaded_ = false;
  
    Object3d* objectModel_ = nullptr;
    Object3d* bombModel_ = nullptr;
    Object3dCom* object3dCom_ = nullptr;
    Camera* camera_ = nullptr;
};
