// Fade.cpp
#include "Fade.h"
#include "TextureManager.h"

Fade::~Fade() { delete sprite_; }

void Fade::Initialize(SpriteCom *spriteCom) {
  sprite_ = new Sprite();

  TextureManager::GetInstance()->LoadTexture("Resources/white.png");
  sprite_->Initialize(spriteCom, "Resources/white.png"); // 白い1x1テクスチャを使用
  sprite_->SetScale({1280.0f, 720.0f});           // 画面いっぱい
  sprite_->SetPosition({0.0f, 0.0f});
  sprite_->SetColor({0.0f, 0.0f, 0.0f, 0.0f}); // 黒色、透明
}

void Fade::Start(FadeState state, float duration) {
  state_ = state;
  duration_ = duration;
  timer_ = 0.0f;
  alpha_ = (state == FadeState::In) ? 1.0f : 0.0f;
}

void Fade::Update() {
  if (state_ == FadeState::None || state_ == FadeState::End) {
    return;
  }

  timer_++;
  float t = timer_ / duration_;
  if (t >= 1.0f) {
    t = 1.0f;
    state_ = FadeState::End;
  }

  if (state_ == FadeState::In) {
    alpha_ = 1.0f - t; // 黒→透明
  } else if (state_ == FadeState::Out) {
    alpha_ = t; // 透明→黒
  }

  sprite_->SetColor({0.0f, 0.0f, 0.0f, alpha_});
  sprite_->Update();
}

void Fade::Draw() {
  if (state_ == FadeState::None)
    return;
  sprite_->Draw();
}
