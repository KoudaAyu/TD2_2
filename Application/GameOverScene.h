#pragma once

#include "Sprite.h"
#include "SpriteCom.h"

class Camera;
class Object3dCom;

class GameOverScene
{
public:
    GameOverScene() = default;
    ~GameOverScene();

    void Initialize(Object3dCom* object3dCom, SpriteCom* spriteCom);
    void Update();
    void Draw();

    bool IsFinished() const { return finished_; }
    // compatibility with other scenes naming
    bool IsFinish() const { return finished_; }

private:
    SpriteCom* spriteCom_ = nullptr;
    Sprite* gameOverSprite_ = nullptr;
    bool finished_ = false;
};
