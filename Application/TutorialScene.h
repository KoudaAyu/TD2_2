#pragma once

#include "Fade.h"

// forward declarations
class Camera;
class Object3dCom;
class SpriteCom;

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
    // フェードの状態遷移を管理するフェーズ
    enum class Phase
    {
        kFadeIn, // フェードイン（シーン開始時）
        kMain,   // メイン処理（未使用）
        kFadeOut // フェードアウト（シーン終了へ）
    };

    Fade* fade_ = nullptr; 
    bool isFinish_ = false;

    // 現在のフェーズ（デフォルトでフェードインにする）
    Phase phase_ = Phase::kFadeIn;
};
