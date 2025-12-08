#pragma once

#include "Camera.h"
#include "Object3dCom.h"
#include "Fade.h"

class SpriteCom;
class Sprite;
class GameOverScene
{
public:
    enum class Phase { kFadeIn, kMain, kFadeOut };

    GameOverScene() = default;
    ~GameOverScene();

    void Initialize(Camera* camera, Object3dCom* object3dCom, SpriteCom* spriteCom);
    void Update();
    void Draw();

    bool IsFinish() const { return isFinish_; }

private:
    bool isFinish_ = false;
    Phase phase_ = Phase::kFadeIn;

    Camera* camera_ = nullptr;
    Object3dCom* object3dCom_ = nullptr;
    SpriteCom* spriteCom_ = nullptr;

    Fade* fade_ = nullptr;
    Sprite* overlay_ = nullptr; // optional visual
};
