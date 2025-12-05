#pragma once

#include <string>

class Camera;
class Object3dCom;
class SpriteCom;
class Sprite;

class ClearScene
{
public:
    ClearScene() = default;
    ~ClearScene();

    void Initialize(Camera* camera, Object3dCom* object3dCom, SpriteCom* spriteCom);
    void Update();
    void Draw();

    bool IsFinish() const { return isFinish_; }

private:
    bool isFinish_ = false;
    Camera* camera_ = nullptr;
    Object3dCom* object3dCom_ = nullptr;
    SpriteCom* spriteCom_ = nullptr;

    Sprite* clearSprite_ = nullptr;
};
