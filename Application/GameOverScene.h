#pragma once

#include "Camera.h"
#include "Object3dCom.h"
#include "Fade.h"

class SpriteCom;
class Sprite;
class Object3d; // forward declaration for Object3d pointer
class KeyInput; // forward declaration for KeyInput

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
    // Background sprite (BackScreen.png)
    Sprite* background_ = nullptr;
    Sprite* overlay_ = nullptr; // optional visual

    // GameOver text model
    Object3d* gameOverModel_ = nullptr;
    // SPACE text model shown below game over
    Object3d* spaceModel_ = nullptr;

    // input
    KeyInput* keyInput_ = nullptr;

    // simple animation timer
    float animTime_ = 0.0f;
};
