#include "UIImage.h"
#include "TextureManager.h"

void UIImage::Initialize(SpriteCom* spriteCom, const std::string& textureFilePath) {
  spriteCom_ = spriteCom;
  sprite_.Initialize(spriteCom, textureFilePath);
}

void UIImage::Update() {
  if (!IsVisible()) return;
  sprite_.Update();
}

void UIImage::Draw() {
  if (!IsVisible()) return;
  sprite_.Draw();
}

void UIImage::SetPosition(const Vector2& pos) { sprite_.SetPosition(pos); }
Vector2 UIImage::GetPosition() const { return sprite_.GetPosition(); }
void UIImage::SetScale(const Vector2& scale) { sprite_.SetScale(scale); }
Vector2 UIImage::GetScale() const { return sprite_.GetScale(); }

bool UIImage::HitTest(const Vector2& point) const {
  Vector2 pos = sprite_.GetPosition();
  Vector2 scale = sprite_.GetScale();
  Vector2 anchor = sprite_.GetAnchorPoint();

  Vector2 topLeft = { pos.x - anchor.x * scale.x, pos.y - anchor.y * scale.y };
  Vector2 bottomRight = { topLeft.x + scale.x, topLeft.y + scale.y };

  return point.x >= topLeft.x && point.x <= bottomRight.x && point.y >= topLeft.y && point.y <= bottomRight.y;
}

void UIImage::SetTextureRect(float left, float top, float width, float height) {
  sprite_.SetTextureLeftTop({left, top});
  sprite_.SetTextureSize({width, height});
}

void UIImage::SetColor(const Vector4& color) {
  sprite_.SetColor(color);
}

void UIImage::SetAnchor(const Vector2& anchor) { sprite_.SetAnchorPoint(anchor); }
Vector2 UIImage::GetAnchor() const { return sprite_.GetAnchorPoint(); }
