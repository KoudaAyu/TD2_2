#include "Fade.h"

#include<algorithm>

Fade::~Fade()
{
}

void Fade::Initialize(SpriteCom* spriteCom)
{ 

	// SpriteCom を注入して保持
	spriteCom_ = spriteCom;
	assert(spriteCom_ && "SpriteCom is null. Call Fade::Initialize with a valid SpriteCom*.");

	fadeSprite_ = spriteCom_->CreateSprite("Resources/white.png",
		{ 0.0f,0.0f },
		{ 1280.0f,720.0f },
		0.0f,
		{ 0.0f,0.0f },
		false,
		false);
}

void Fade::Update()
{
	switch (status_)
	{
		case State::kNone:
			// 何もしない
			break;
		case State::kFadeIn:
			FadeIn();
			break;
		case State::kFadeOut:
			FadeOut();
			break;
	}
}

void Fade::Draw()
{
	if (fadeSprite_) {
		fadeSprite_->Draw();
	}
}

void Fade::Start(State state, float duration)
{
	status_ = state;
	duration_ = duration;
	counter_ = 0.0f;
}

void Fade::FadeIn()
{
	// 時間経過
	counter_ += 1.0f / 60.0f;
	// 終了判定
	if (counter_ >= duration_) {
		counter_ = duration_;
	}
	
	float t = duration_ > 0.0f ? std::clamp(counter_ / duration_, 0.0f, 1.0f) : 1.0f;
	fadeSprite_->SetColor(Vector4(0, 0, 0, 1.0f - t));
}

void Fade::FadeOut()
{	
	// 時間経過
	counter_ += 1.0f / 60.0f;

	//フェード継続時間に達したら処理終了
	if(counter_ >= duration_)
	{
		counter_ = duration_;
	}

	float t = duration_ > 0.0f ? std::clamp(counter_ / duration_, 0.0f, 1.0f) : 1.0f;
	fadeSprite_->SetColor(Vector4(0, 0, 0, t));
}

void Fade::Stop()
{
	status_ = State::kNone;
	if (status_ == State::kNone)
	{
		return;
	}
}

bool Fade::IsFinished() const
{
	switch (status_)
	{
	case State::kFadeIn:
	case State::kFadeOut:
		if(counter_ >= duration_)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	return false;
}
