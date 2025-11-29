#pragma once

#include "Fade.h"
#include "KeyInput.h"
#include "SpriteCom.h"

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

    void Initialize(SpriteCom* spriteCom);
    void Update();
    void Draw();

    bool IsFinish() const { return isFinish_; }
    Choice GetChoice() const { return choice_; }

private:
    bool isFinish_ = false;
    Fade* fade_ = nullptr;
    KeyInput* keyInput_ = nullptr;
    Phase phase_ = Phase::kFadeIn;
    Choice choice_ = Choice::kGame;
};
