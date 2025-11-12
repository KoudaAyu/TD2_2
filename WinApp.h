#pragma once
#include "externals/imgui/imgui_impl_win32.h"
#include <Windows.h>
#include <cstdint>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
using Microsoft::WRL::ComPtr;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd,
                                                             UINT msg,
                                                             WPARAM wparam,
                                                             LPARAM lParam);

class WinApp {
public:
  // クライアント領域のサイズ
  static const int32_t kClientWidth = 1280;
  static const int32_t kClientHeight = 720;

private:

  HWND hwnd_ = nullptr;

  // ウィンドウ関係
  WNDCLASS wc{};

public:
  // ウィンドウプロシージャ
  static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam,
                                     LPARAM lparam);

  // 初期化
  void Initialize();
  // 更新
  void Update();
  //終了
  void Finalize();

  // メッセージの処理
  bool ProcessMessage();

  //ゲッター
  int32_t GetClientWidth() { return kClientWidth; }
  int32_t GetClientHeight() { return kClientHeight; }
  HWND GetHwnd() const { return hwnd_; }
  HINSTANCE GetHInstance() const { return wc.hInstance; }
};
