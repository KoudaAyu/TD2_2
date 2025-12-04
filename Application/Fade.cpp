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

	// 完了したらアクティブ状態を解除（ただし counter_ は維持して IsFinished() が true を返すようにする）
	if (counter_ >= duration_) {
		status_ = State::kNone;
	}
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

	if (counter_ >= duration_) {
		status_ = State::kNone;
	}
}

void Fade::Stop()
{
	// stop the fade by setting status to none and resetting the counter
	status_ = State::kNone;
	counter_ = 0.0f;
}

bool Fade::IsFinished() const
{
	return counter_ >= duration_;
}

bool Fade::IsActive() const
{
	return status_ != State::kNone;
}

Fade::State Fade::GetState() const
{
	return status_;
}
