#pragma once

#include <memory>
#include <string>
#include "Vector.h"


class SpriteCom;

class UIElement {
public:
  UIElement() = default;
  virtual ~UIElement() = default;

  virtual void Initialize(SpriteCom* spriteCom, const std::string& textureFilePath) = 0;
  virtual void Update() = 0;
  virtual void Draw() = 0;

  virtual void SetPosition(const Vector2& pos) = 0;
  virtual Vector2 GetPosition() const = 0;
  virtual void SetScale(const Vector2& scale) = 0;
  virtual Vector2 GetScale() const = 0;

  virtual bool HitTest(const Vector2& point) const = 0;

  // mouse events
  virtual void OnMouseDown(const Vector2& /*point*/) {}
  virtual void OnMouseUp(const Vector2& /*point*/) {}
  virtual void OnMouseMove(const Vector2& /*point*/) {}

  void SetVisible(bool v) { visible_ = v; }
  bool IsVisible() const { return visible_; }

  void SetZOrder(int z) { zOrder_ = z; }
  int GetZOrder() const { return zOrder_; }

private:
  bool visible_ = true;
  int zOrder_ = 0;
};
