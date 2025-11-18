#include "Fade.h"

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
}

void Fade::Draw()
{
	if (fadeSprite_) {
		fadeSprite_->Draw();
	}
}
