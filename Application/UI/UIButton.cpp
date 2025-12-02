#include "UIButton.h"
#include "../../Baziru3_Engine/Base/Window/WinApp.h"
#include "../../Baziru3_Engine/IO/KeyInput/KeyInput.h"

// simple mouse helper
static Vector2 GetMousePosInClient() {
  POINT p;
  GetCursorPos(&p);
  HWND hwnd = GetActiveWindow();
  ScreenToClient(hwnd, &p);
  return { (float)p.x, (float)p.y };
}

void UIButton::Update() {
  if (!IsVisible()) return;

  Vector2 mpos = GetMousePosInClient();
  bool hit = HitTest(mpos);

  bool mouseDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

  if (hit) {
    if (!hovered_) {
      hovered_ = true;
    }
    if (mouseDown) {
      if (!pressed_) {
        pressed_ = true;
        OnMouseDown(mpos);
      }
    }
    else {
      if (pressed_) {
        // released while over button -> click
        pressed_ = false;
        OnMouseUp(mpos);
        if (onClick_) onClick_();
      }
    }
  }
  else {
    hovered_ = false;
    if (!mouseDown && pressed_) {
      pressed_ = false;
      OnMouseUp(mpos);
    }
  }

  UIImage::Update();
}
