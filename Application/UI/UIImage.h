#pragma once

#include "UIElement.h"
#include "Sprite.h"
#include <string>
#include <memory>

class UIImage : public UIElement {
public:
  UIImage() = default;
  ~UIImage() = default;

  void Initialize(SpriteCom* spriteCom, const std::string& textureFilePath) override;
  void Update() override;
  void Draw() override;

  void SetPosition(const Vector2& pos) override;
  Vector2 GetPosition() const override;
  void SetScale(const Vector2& scale) override;
  Vector2 GetScale() const override;

  bool HitTest(const Vector2& point) const override;

  void SetTextureRect(float left, float top, float width, float height);

  void SetColor(const Vector4& color);

  // Anchor point (0..1)
  void SetAnchor(const Vector2& anchor);
  Vector2 GetAnchor() const;

private:
  Sprite sprite_; // use Sprite from engine include
  SpriteCom* spriteCom_ = nullptr;
};
