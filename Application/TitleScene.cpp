#include "TitleScene.h"

TitleScene::~TitleScene()
{
	delete fade_;
}

void TitleScene::Initialize(SpriteCom* spriteCom)
{
	fade_ = new Fade();
	fade_->Initialize(spriteCom);

	keyInput_ = KeyInput::GetInstance();
}

void TitleScene::Update()
{
	fade_->Update();

	if(keyInput_->TriggerKey(DIK_SPACE))
	{
		isFinish_ = true;
	}
}

void TitleScene::Draw()
{
	fade_->Draw();
}
