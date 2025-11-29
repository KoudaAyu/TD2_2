#pragma once

#include "GameScene.h"
#include "Fade.h"

class TutorialScene
{
public:
    TutorialScene() = default;
    ~TutorialScene();

    void Initialize(Camera* camera, Object3dCom* object3dCom, SpriteCom* spriteCom);
    void Update();
    void Draw();

    bool IsFinish() const { return isFinish_; }

private:
    GameScene* gameScene_ = nullptr;
    Fade* fade_ = nullptr; // optional overlay
    bool isFinish_ = false;
};
