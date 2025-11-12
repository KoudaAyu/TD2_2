#pragma once

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#endif

class WinApp;
class SrvManager;
class DirectXCom;

class ImGuiManager {
#ifdef USE_IMGUI
private:
  WinApp *winApp_ = nullptr;

  DirectXCom *directXCom_ = nullptr;

  SrvManager *srvManager_ = nullptr;

   int imguiSrvIndex_ = -1;
#endif
public:
  /// <summary>
  /// 初期化
  /// </summary>
  void Initialize(WinApp* winApp, DirectXCom *directXCom, SrvManager *srvManager);
  /// <summary>
  /// 終了
  /// </summary>
  void Finalize();

  /// <summary>
  /// ImGui受付開始
  /// </summary>
  void Begin();
  /// <summary>
  /// ImGui受付終了
  /// </summary>
  void End();
  /// <summary>
  /// 画面への描画
  /// </summary>
  void Draw();
};
