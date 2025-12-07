#include "GameOverScene.h"
#include "SpriteCom.h"
#include "Sprite.h"
#include "Object3dCom.h"
#include "KeyInput.h"
#include <cassert>

extern KeyInput* keyInput; // from main.cpp

GameOverScene::~GameOverScene()
{
    if (gameOverSprite_) { delete gameOverSprite_; gameOverSprite_ = nullptr; }
}

void GameOverScene::Initialize(Object3dCom* /*object3dCom*/, SpriteCom* spriteCom)
{
    spriteCom_ = spriteCom;
    if (!spriteCom_) return;

    // load texture externally; assume application already loaded textures via TextureManager elsewhere
    gameOverSprite_ = new Sprite();
    gameOverSprite_->Initialize(spriteCom_, "Resources/gameover.png");

    // center on screen
    gameOverSprite_->SetPosition({ 640.0f, 360.0f });
}

void GameOverScene::Update()
{
    if (!gameOverSprite_) return;
    gameOverSprite_->Update();

    // SPACE or A returns to Title
    if (!keyInput) return;
    if (keyInput->TriggerKey(DIK_SPACE) || keyInput->TriggerPadButton(XINPUT_GAMEPAD_A))
    {
        finished_ = true;
    }
}

void GameOverScene::Draw()
{
    if (gameOverSprite_) gameOverSprite_->Draw();
}
