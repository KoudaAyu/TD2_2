#include "UI.h"
#include "TextureManager.h"

void UI::Initialize(SpriteCom *spriteCom, const std::string &textureFilePath) {
  sprite_.Initialize(spriteCom, textureFilePath);
}

void UI::Update() {
  if (!visible_) return;
  sprite_.Update();
}

void UI::Draw() {
  if (!visible_) return;
  sprite_.Draw();
}

void UI::SetTextureRect(float left, float top, float width, float height) {
  sprite_.SetTextureLeftTop({left, top});
  sprite_.SetTextureSize({width, height});
  // テクスチャサイズを再調整したい場合は追加処理
}
