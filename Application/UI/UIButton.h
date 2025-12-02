#pragma once

#include "UIImage.h"
#include <functional>

class UIButton : public UIImage {
public:
  UIButton() = default;
  ~UIButton() = default;

  void Update() override;

  void SetOnClick(const std::function<void()>& cb) { onClick_ = cb; }

private:
  bool pressed_ = false;
  bool hovered_ = false;
  std::function<void()> onClick_;
};
