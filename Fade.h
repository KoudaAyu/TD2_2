// Fade.h
#pragma once
#include "Sprite.h"

enum class FadeState
{
	None,
	In,  // フェードイン中（暗→明）
	Out, // フェードアウト中（明→暗）
	End
};

class Fade
{
public:
	Fade() = default;
	~Fade();

	void Initialize(SpriteCom* spriteCom);
	void Start(FadeState state, float duration);
	void Update();
	void Draw();

	bool IsEnd() const { return state_ == FadeState::End; }

private:
	Sprite* sprite_ = nullptr;
	FadeState state_ = FadeState::None;
	float timer_ = 0.0f;
	float duration_ = 60.0f; // デフォルト60フレーム
	float alpha_ = 0.0f;
};
