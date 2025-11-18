#pragma once

#include"Sprite.h"
#include"SpriteCom.h"

class Fade
{
public:
	Fade() = default;
	~Fade();
	void Initialize(SpriteCom* spriteCom);
	void Update();
	void Draw();

private:
	Sprite* fadeSprite_ = nullptr;
	SpriteCom* spriteCom_ = nullptr;
};