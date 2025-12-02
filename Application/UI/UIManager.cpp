#include "UIManager.h"
#include <algorithm>

void UIManager::Remove(const std::shared_ptr<UIElement>& element) {
  elements_.erase(std::remove(elements_.begin(), elements_.end(), element), elements_.end());
}

void UIManager::UpdateAll() {
  // sort by z-order ascending so higher z draw later
  std::sort(elements_.begin(), elements_.end(), [](const std::shared_ptr<UIElement>& a, const std::shared_ptr<UIElement>& b) {
    return a->GetZOrder() < b->GetZOrder();
  });

  for (auto& e : elements_) {
    e->Update();
  }
}

void UIManager::DrawAll() {
  for (auto& e : elements_) {
    e->Draw();
  }
}
