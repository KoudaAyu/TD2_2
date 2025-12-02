#pragma once

#include <vector>
#include <memory>
#include "UIElement.h"

class UIManager {
public:
  UIManager() = default;
  ~UIManager() = default;

  void Add(const std::shared_ptr<UIElement>& element) { elements_.push_back(element); }
  void Remove(const std::shared_ptr<UIElement>& element);

  void UpdateAll();
  void DrawAll();

private:
  std::vector<std::shared_ptr<UIElement>> elements_;
};
