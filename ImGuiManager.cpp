#include "ImGuiManager.h"
#include "DirectXCom.h"
#include "SrvManager.h"
#include "WinApp.h"

/// <summary>
/// 初期化
/// </summary>
void ImGuiManager::Initialize(WinApp *winApp, DirectXCom *directXCom,
                              SrvManager *srvManager) {
#ifdef USE_IMGUI
  directXCom_ = directXCom;
  srvManager_ = srvManager;
  winApp_ = winApp;

  ImGui::CreateContext();
  ImGui::StyleColorsDark();

  ImGui_ImplWin32_Init(winApp_->GetHwnd());

  // ImGui 用SRVを1つ確保（以後このスロットは再利用しない）
  imguiSrvIndex_ = srvManager_->Allocate();

  ImGui_ImplDX12_Init(
      directXCom_->GetDevice(),
      static_cast<int>(directXCom_->GetSwapChainResourcesNum()),
      DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, // ← swapchainのRTVと同じ形式に
      srvManager_->descriptorHeap_.Get(),
      srvManager_->GetCPUDescriptorHandle(imguiSrvIndex_),
      srvManager_->GetGPUDescriptorHandle(imguiSrvIndex_));

#endif
}


/// <summary>
/// 終了
/// </summary>
void ImGuiManager::Finalize() {
#ifdef USE_IMGUI
  // ImGui終了処理
  ImGui_ImplDX12_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();
#endif
}

/// <summary>
/// ImGui受付開始
/// </summary>
void ImGuiManager::Begin() {
#ifdef USE_IMGUI
  // Imguiにここからフレームが始まる趣旨をつたえる
  ImGui_ImplDX12_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();
#endif
}
/// <summary>
/// ImGui受付終了
/// </summary>
void ImGuiManager::End() {
#ifdef USE_IMGUI
  // ImGui内部コマンドを生成する
  ImGui::Render();
#endif
}
/// <summary>
/// 画面への描画
/// </summary>
void ImGuiManager::Draw() {
#ifdef USE_IMGUI
  ID3D12GraphicsCommandList *commandList = directXCom_->GetCommandList();

  ID3D12DescriptorHeap *heaps[] = {srvManager_->descriptorHeap_.Get()};
  commandList->SetDescriptorHeaps(_countof(heaps), heaps);

  ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
#endif
}