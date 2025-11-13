#include "WinApp.h"
#include "externals/imgui/imgui_impl_win32.h"

// FPS固定機能のsleep精度をあげる
#pragma comment(lib, "winmm.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd,
                                                             UINT msg,
                                                             WPARAM wparam,
                                                             LPARAM lParam);

// ウィンドウプロシージャ
LRESULT CALLBACK WinApp::WindowProc(HWND hwnd, UINT msg, WPARAM wparam,
                                    LPARAM lparam) {
  if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
    return true;
  }

  // メッセージに応じてゲーム固有の処理を行う
  switch (msg) {
    // ウィンドウが破棄された
  case WM_DESTROY:
    // アプリケーションを終了する
    PostQuitMessage(0);
    return 0;
  }

  // 標準のメッセージ処理を行う
  return DefWindowProc(hwnd, msg, wparam, lparam);
}

// 初期化
void WinApp::Initialize() {

  // システムタイマーの分解能をあげる
  timeBeginPeriod(1);

  // ウィンドウプロシージャ
  wc.lpfnWndProc = WindowProc;

  // ウィンドウクラス名
  wc.lpszClassName = L"MyWindowClass";

  // インスタンスハンドル
  wc.hInstance = GetModuleHandle(nullptr);

  // カーソル
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

  // ウィンドウクラスを登録する
  RegisterClass(&wc);

  // ウィンドウサイズを表す構造体にクライアント領域を入れる
  RECT wrc = {0, 0, kClientWidth, kClientHeight};

  // クライアント領域をもとに実際のサイズにwrcを変更してもらう
  AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, FALSE);

  // ウィンドウの生成
  hwnd_ = CreateWindow(wc.lpszClassName,             // ウィンドウクラス名
                       L"window name",            // ウィンドウタイトル
                       WS_OVERLAPPEDWINDOW,          // ウィンドウスタイル
                       CW_USEDEFAULT, CW_USEDEFAULT, // 位置
                       wrc.right - wrc.left, wrc.bottom - wrc.top, // サイズ
                       nullptr,      // 親ウィンドウハンドル
                       nullptr,      // メニューハンドル
                       wc.hInstance, // インスタンスハンドル
                       nullptr       // 追加のパラメータ
  );

#ifdef _DEBUG
  Microsoft::WRL::ComPtr<ID3D12Debug1> debugController = nullptr;

  if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
    // デバックレイヤーを有効化する
    debugController->EnableDebugLayer();
    // 更にGPU側でもチェックを行うようにする
    debugController->SetEnableGPUBasedValidation(TRUE);
  }
#endif
  // ウィンドウを表示する
  ShowWindow(hwnd_, SW_SHOW);
}
// 更新
void WinApp::Update() {}
// 終了
void WinApp::Finalize() {
  CloseWindow(hwnd_);
  CoUninitialize();
}
// メッセージの処理
bool WinApp::ProcessMessage() {
  MSG msg{};

  // Windowに目セージが来ていたら最優先で処理される
  if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
    TranslateMessage(&msg); // メッセージを変換
    DispatchMessage(&msg);  // メッセージをウィンドウプロシージャに送る
  }

  if (msg.message == WM_QUIT) {
    return true;
  }

  return false;
}