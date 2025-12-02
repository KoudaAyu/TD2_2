#pragma once
#include "Sprite.h"
#include "SpriteCom.h"

#include <string>

class UI {
public:
  UI() = default;

  // 初期化: SpriteCom とテクスチャパスを渡す
  void Initialize(SpriteCom *spriteCom, const std::string &textureFilePath);

  // 更新と描画
  void Update();
  void Draw();

  // プロパティ操作（Sprite をラップ）
  void SetPosition(const Vector2 &pos) { sprite_.SetPosition(pos); }
  Vector2 GetPosition() const { return sprite_.GetPosition(); }

  void SetScale(const Vector2 &s) { sprite_.SetScale(s); }
  const Vector2 &GetScale() const { return sprite_.GetScale(); }

  void SetRotation(float r) { sprite_.SetRotation(r); }
  float GetRotation() const { return sprite_.GetRotation(); }

  void SetColor(const Vector4 &c) { sprite_.SetColor(c); }

  void SetAnchor(const Vector2 &a) { sprite_.SetAnchorPoint(a); }

  void SetFlip(bool flipX, bool flipY) {
    sprite_.SetFlipX(flipX);
    sprite_.SetFlipY(flipY);
  }

  // テクスチャの切り出し指定（左上座標とサイズ）
  void SetTextureRect(float left, float top, float width, float height);

  void SetVisible(bool v) { visible_ = v; }
  bool IsVisible() const { return visible_; }

  void SetTextureSrvHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle) {
    sprite_.SetTextureSrvHandle(handle);
  }

private:
  Sprite sprite_;
  bool visible_ = true;
};
