#include "DirectXCom.h"
#include "externals/DirectXTex/d3dx12.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include <cassert>
#include <format>
#include <thread>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

using namespace Microsoft::WRL;
using namespace Logger;
using namespace StringUtility;

//const uint32_t DirectXCom::kMaxSRVCount = 512;

/// <summary>
/// デストラクタ
/// </summary>
DirectXCom::~DirectXCom() { if (fenceEvent_) { CloseHandle(fenceEvent_); fenceEvent_ = nullptr; } }
/// <summary>
/// 初期化
/// </summary>
/// <param name="winApp"></param>
void DirectXCom::Initialize(WinApp *winApp) {
  // FPS固定初期化
  InitializeFixFPS();

  // NULLチェック
  assert(winApp);
  // メンバ変数に記録
  winApp_ = winApp;

  CreateFactory();

  // デバイスの初期化
  InitializeDevice();
  // コマンドの初期化
  InitializeCommand();
  // スワップチェーンの生成
  CreateSwapChain();
  // 深度バッファの生成
  depthStencilResource = CreateDepthStencilTextureResource();
  // 各種デスクリプタヒープの生成
  CreateDescriptorHeaps();
  // レンダーターゲットビューの初期化
  InitializeRenderTargetView();
  // 深度ステンシルビューの初期化
  InitializeDepthStencilView();
  // フェンスの生成
  CreateFence();
  // ビューポート矩形の設定
  InitializeViewportRect();
  // シザリング矩形の初期化
  InitializeScissorRect();
  // DXCコンパイラの生成
  CreateDXCCompiler();
  // imguiの初期化
  InitializeImGui();
}

/// <summary>
/// DXGIファクトリーの生成
/// </summary>
void DirectXCom::CreateFactory() {
  // HRESULTはWindoes系のエラーコード
  // 関数が成功したか同課をSUCCEEDEDマクロで判断する
  HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory_));

  assert(SUCCEEDED(hr));
}

/// <summary>
/// デバイスの初期化
/// </summary>
void DirectXCom::InitializeDevice() {

  // 使用するアダプタ用の変数。最初にnullptrを入れる
  Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter = nullptr;

  // いい順にアダプタを頼む
  for (UINT i = 0; GetDxgiFactory()->EnumAdapterByGpuPreference(
                       i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                       IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND;
       ++i) {

    // アダプターの情報を取得する
    DXGI_ADAPTER_DESC3 adapterDesc{};
    hr = useAdapter->GetDesc3(&adapterDesc);
    assert(SUCCEEDED(hr)); // ここで止まった場合一大事

    // ソフトウェアアダプタでなければ採用する
    if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) {
      Log(std::format("Using adapter: {}\n",
                      ConvertString(adapterDesc.Description)));
      break;
    }
    useAdapter =
        nullptr; // ソフトウェアアダプタの場合は見なかったことにするためしないのでnullptr
  }

  // アダプターが見つからなかった場合はエラー
  assert(useAdapter != nullptr);

  // 機能レベルとログの出力用の文字列
  D3D_FEATURE_LEVEL featureLevels[] = {
      D3D_FEATURE_LEVEL_12_2,
      D3D_FEATURE_LEVEL_12_1,
      D3D_FEATURE_LEVEL_12_0,
  };

  const char *featureLevelNames[] = {
      "12.2",
      "12.1",
      "12.0",
  };

  // 機能レベルを順に試していく
  for (size_t i = 0; i < _countof(featureLevels); ++i) {
    // 採用したアダプタでデバイスを作成
    hr = D3D12CreateDevice(useAdapter.Get(),      // アダプタ
                           featureLevels[i],      // 機能レベル
                           IID_PPV_ARGS(&device_) // デバイスのポインタ
    );

    // 指定した昨日レベルでデバイスが生成できたか確認
    if (SUCCEEDED(hr)) {
      // 生成出来たのでログ出力を行う
      Log(std::format("Feature Level: {}\n", featureLevelNames[i]));
      break; // ループを抜ける
    }
  }

  // デバイスの生成に失敗し起動できない
  assert(device_ != nullptr);
  Log(std::format("Complete create D3D12Device!")); // 初期起動完了のLogを出す

#ifdef _DEBUG
  ID3D12InfoQueue *infoQueue = nullptr;
  if (SUCCEEDED(device_->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
    // 重大なエラーの時に止まる
    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);

    // エラーの時に止まる
    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);

    // 警告時に止まる
    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

    // 抑制するメッセージのID
    D3D12_MESSAGE_ID denyIds[] = {
        D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE};

    D3D12_MESSAGE_SEVERITY serverities[] = {D3D12_MESSAGE_SEVERITY_INFO};
    D3D12_INFO_QUEUE_FILTER filter{};
    filter.DenyList.NumIDs = _countof(denyIds); // 抑制するメッセージの数
    filter.DenyList.pIDList = denyIds;          // 抑制するメッセージのID
    filter.DenyList.NumSeverities =
        _countof(serverities); // 抑制するメッセージの重要度の数
    filter.DenyList.pSeverityList = serverities; // 抑制するメッセージの重要度

    infoQueue->PushStorageFilter(&filter); // フィルターを適用する

    // 解放
    infoQueue->Release();
  }
#endif
}

/// <summary>
/// コマンドの初期化
/// </summary>
void DirectXCom::InitializeCommand() {

  // コマンドキューの生成
  D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
  hr = device_->CreateCommandQueue(&commandQueueDesc,
                                   IID_PPV_ARGS(&commandQueue));

  // コマンドキューの生成に失敗した場合はエラー
  assert(SUCCEEDED(hr));

  // コマンドアロケーターを生成する
  hr = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                       IID_PPV_ARGS(&commandAllocator));

  // コマンドアロケーターの生成に失敗した場合はエラー
  assert(SUCCEEDED(hr));

  // コマンドリストの生成
  hr = device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                  commandAllocator.Get(), nullptr,
                                  IID_PPV_ARGS(&commandList));

  // コマンドリストの生成に失敗した場合はエラー
  assert(SUCCEEDED(hr));
}

/// <summary>
/// スワップチェーンの初期化
/// </summary>
void DirectXCom::CreateSwapChain() {
  swapChainDesc_.Width = winApp_->GetClientWidth();   // ウィンドウの幅
  swapChainDesc_.Height = winApp_->GetClientHeight(); // ウィンドウの高さ
  swapChainDesc_.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 色の形式
  swapChainDesc_.SampleDesc.Count = 1;                // マルチサンプルしない
  swapChainDesc_.BufferUsage =
      DXGI_USAGE_RENDER_TARGET_OUTPUT; // レンダリングターゲットとして使用
  swapChainDesc_.BufferCount = 2;      // ダブルバッファリング
  swapChainDesc_.SwapEffect =
      DXGI_SWAP_EFFECT_FLIP_DISCARD; // モニターに映ったら描画を破棄

  // コマンドキュー、ウィンドウハンドル、設定を渡して生成する
  hr = GetDxgiFactory()->CreateSwapChainForHwnd(
      commandQueue.Get(), winApp_->GetHwnd(), &swapChainDesc_, nullptr, nullptr,
      reinterpret_cast<IDXGISwapChain1 **>(swapChain_.GetAddressOf()));
  // スワップチェーンの生成に失敗した場合はエラー
  assert(SUCCEEDED(hr));
}

/// <summary>
/// 深度バッファの生成
/// </summary>
/// <returns></returns>
ComPtr<ID3D12Resource> DirectXCom::CreateDepthStencilTextureResource() {
  // 生成するResourceの設定
  D3D12_RESOURCE_DESC resourceDesc{};
  resourceDesc.Width = winApp_->GetClientWidth();   // Textureの幅
  resourceDesc.Height = winApp_->GetClientHeight(); // textureの高さ
  resourceDesc.MipLevels = 1;                       // mipmapの数
  resourceDesc.DepthOrArraySize = 1; // 奥行き or 配列Textureの配列数
  resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // TextureのFormat
  resourceDesc.SampleDesc.Count = 1; // サンプリングカウント。1固定
  resourceDesc.Dimension =
      D3D12_RESOURCE_DIMENSION_TEXTURE2D; // Textureの次元数。普段使っているのは2次元
  resourceDesc.Flags =
      D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // DepthStencilとして使う通知

  // 2. 利用するHeapの設定
  D3D12_HEAP_PROPERTIES heapProperties{};
  heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // VRAM上に作る

  // 深度値のクリア設定
  D3D12_CLEAR_VALUE depthClearValue{};
  depthClearValue.DepthStencil.Depth = 1.0f; // 1.0f(最大値)でクリア
  depthClearValue.Format =
      DXGI_FORMAT_D24_UNORM_S8_UINT; // フォーマット。Resourceと合わせる

  // 3. Resourceを生成する
  ComPtr<ID3D12Resource> resource = nullptr;
  HRESULT hr = device_->CreateCommittedResource(
      &heapProperties,                  // Heapの設定
      D3D12_HEAP_FLAG_NONE,             // Heapの特殊な設定
      &resourceDesc,                    // Resourceの設定
      D3D12_RESOURCE_STATE_DEPTH_WRITE, // 深度値を書き込む状態にしておく
      &depthClearValue,                 // Clear最適値。
      IID_PPV_ARGS(&resource));         // 作成するResourceポインタへのポインタ
  assert(SUCCEEDED(hr));
  return resource;
}

/// <summary>
/// デスクリプタヒープの生成
/// </summary>
void DirectXCom::CreateDescriptorHeaps() {
  // DescriptorSizeを取得しておく
  /*descriptorSizeSRV_ = device_->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);*/
  descriptorSizeRTV_ =
      device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  descriptorSizeDSV_ =
      device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

  // RTV用のヒープでディスクリプタの数は2。RTVはShader内でふれるものではないため、ShaderVisibleはfalse
  rtvDescriptorHeap_ =
      CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
  // SRV用のヒープでディスクリプタの数は128。SRTはShader内で触れるものなので、ShaderVisibleはtrue
  /*srvDescriptorHeap_ = CreateDescriptorHeap(
      D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kMaxSRVCount, true);*/

  // DSV用のヒープでディスクリプタの数は1。DSVはShader内で触れるものではないため、ShaderVisibleはfalse
  dsvDescriptorHeap_ =
      CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);
}

/// <summary>
/// デスクリプタヒープの生成
/// </summary>
/// <param name="heapType"></param>
/// <param name="numDescriptors"></param>
/// <param name="shaderVisible"></param>
/// <returns></returns>
ComPtr<ID3D12DescriptorHeap>
DirectXCom::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType,
                                 UINT numDescriptors, bool shaderVisible) {
  ComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;
  D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
  descriptorHeapDesc.Type = heapType;
  descriptorHeapDesc.NumDescriptors = numDescriptors;
  descriptorHeapDesc.Flags = shaderVisible
                                 ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
                                 : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  HRESULT hr = device_->CreateDescriptorHeap(&descriptorHeapDesc,
                                             IID_PPV_ARGS(&descriptorHeap));
  assert(SUCCEEDED(hr));
  return descriptorHeap;
}

/// <summary>
/// レンダーターゲットビューの初期化
/// </summary>
void DirectXCom::InitializeRenderTargetView() {
  // --- バックバッファ取得 ---
  for (UINT i = 0; i < swapChainResources_.size(); ++i) {
    hr = swapChain_->GetBuffer(i, IID_PPV_ARGS(&swapChainResources_[i]));
    assert(SUCCEEDED(hr));
  }

  // --- RTV 設定 ---
  rtvDesc_.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
  rtvDesc_.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

  // 先頭ハンドル＆インクリメント幅
  const D3D12_CPU_DESCRIPTOR_HANDLE base =
      rtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
  const UINT rtvInc =
      device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

  // rtvHandles_ のサイズをバックバッファ数に合わせる（vector の場合）
  // rtvHandles_.resize(static_cast<size_t>(swapChainResources_.size()));

  // --- RTV 作成（バックバッファ数分）---
  for (UINT i = 0; i < swapChainResources_.size(); ++i) {
    // i番目のハンドル = 先頭 + i * インクリメント
    rtvHandles_[i].ptr = base.ptr + static_cast<SIZE_T>(i) * rtvInc;

    device_->CreateRenderTargetView(swapChainResources_[i].Get(), &rtvDesc_,
                                    rtvHandles_[i]);
  }
}

///// <summary>
///// SRVのCPUデスクリプタハンドルを取得
///// </summary>
///// <param name="index"></param>
///// <returns></returns>
//D3D12_CPU_DESCRIPTOR_HANDLE
//DirectXCom::GetSRVCPUDescriptorHnadle(uint32_t index) {
//  return GetCPUDescriptorHandle(srvDescriptorHeap_, descriptorSizeSRV_, index);
//}
///// <summary>
///// SRVのGPUデスクリプタハンドルを取得
///// </summary>
///// <param name="index"></param>
///// <returns></returns>
//D3D12_GPU_DESCRIPTOR_HANDLE
//DirectXCom::GetSRVGPUDescriptorHnadle(uint32_t index) {
//  return GetGPUDescriptorHandle(srvDescriptorHeap_, descriptorSizeSRV_, index);
//}

/// <summary>
/// CPUでスクリプタハンドルを取得
/// </summary>
/// <param name="descriptorHeap"></param>
/// <param name="descriptorSize"></param>
/// <param name="index"></param>
/// <returns></returns>
D3D12_CPU_DESCRIPTOR_HANDLE DirectXCom::GetCPUDescriptorHandle(
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> &descriptorHeap,
    uint32_t descriptorSize, uint32_t index) {
  D3D12_CPU_DESCRIPTOR_HANDLE handleCPU =
      descriptorHeap->GetCPUDescriptorHandleForHeapStart();
  handleCPU.ptr += (descriptorSize * index);
  return handleCPU;
}

/// <summary>
/// GPUでスクリプタハンドルを取得
/// </summary>
/// <param name="descriptorHeap"></param>
/// <param name="descriptorSize"></param>
/// <param name="index"></param>
/// <returns></returns>
D3D12_GPU_DESCRIPTOR_HANDLE DirectXCom::GetGPUDescriptorHandle(
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> &descriptorHeap,
    uint32_t descriptorSize, uint32_t index) {
  D3D12_GPU_DESCRIPTOR_HANDLE handleGPU =
      descriptorHeap->GetGPUDescriptorHandleForHeapStart();
  handleGPU.ptr += (descriptorSize * index);
  return handleGPU;
}

/// <summary>
/// 深度ステンシルビューの初期化
/// </summary>
void DirectXCom::InitializeDepthStencilView() {
  // DSVの設定
  D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
  dsvDesc.Format =
      DXGI_FORMAT_D24_UNORM_S8_UINT; // Format。基本的にはResourceに合わせる
  dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D; // 2dTexture
  // DSVHeapの先頭にDSVを作る
  device_->CreateDepthStencilView(
      depthStencilResource.Get(), &dsvDesc,
      dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart());
}

/// <summary>
/// フェンスの生成
/// </summary>
void DirectXCom::CreateFence() {
  // 初期値0でFenceを作る
  fence = nullptr;
  fenceValue_ = 0;
  hr = device_->CreateFence(fenceValue_, D3D12_FENCE_FLAG_NONE,
                            IID_PPV_ARGS(&fence));
  // フェンスの生成に失敗した場合はエラー
  assert(SUCCEEDED(hr));

  fenceEvent_ = CreateEvent(NULL, FALSE, FALSE, NULL);
  // フェンスイベントの生成に失敗した場合はエラー
  assert(fenceEvent_ != nullptr);
}

/// <summary>
/// ビューポート矩形の設定
/// </summary>
void DirectXCom::InitializeViewportRect() {
  // クライアント領域のサイズと一緒にして画面全体に表示
  viewport_.Width = static_cast<float>(winApp_->GetClientWidth());
  viewport_.Height = static_cast<float>(winApp_->GetClientHeight());
  viewport_.TopLeftX = 0.0f; // 左上のX座標
  viewport_.TopLeftY = 0.0f; // 左上のY座標
  viewport_.MinDepth = 0.0f; // 最小の深度
  viewport_.MaxDepth = 1.0f; // 最大の深度
}
/// <summary>
/// シザリング矩形の初期化
/// </summary>
void DirectXCom::InitializeScissorRect() {

  // 基本的にビューポートと同じ矩形が構成されるようにする
  scissorRect_.left = 0;                            // 左上のX座標
  scissorRect_.right = winApp_->GetClientWidth();   // 右下のX座標
  scissorRect_.top = 0;                             // 左上のY座標
  scissorRect_.bottom = winApp_->GetClientHeight(); // 右下のY座標
}

/// <summary>
/// DXCコンパイラの生成
/// </summary>
void DirectXCom::CreateDXCCompiler() {
  // dxcCompilerを初期化
  dxcUtils_ = nullptr;
  dxcCompiler_ = nullptr;
  hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils_));
  assert(SUCCEEDED(hr));
  hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler_));
  assert(SUCCEEDED(hr));

  // 現時点ではincludeしないが、includeに対応する為の設定を行う
  includeHandler_ = nullptr;
  hr = dxcUtils_->CreateDefaultIncludeHandler(&includeHandler_);
  assert(SUCCEEDED(hr));
}

/// <summary>
/// ImGuiの初期化
/// </summary>
void DirectXCom::InitializeImGui() {
 /* IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  ImGui_ImplWin32_Init(winApp_->GetHwnd());
  ImGui_ImplDX12_Init(device_.Get(), swapChainDesc_.BufferCount,
                      rtvDesc_.Format, srvDescriptorHeap_.Get(),
                      srvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart(),
                      srvDescriptorHeap_->GetGPUDescriptorHandleForHeapStart());*/
}

/// <summary>
/// 描画前処理
/// </summary>
void DirectXCom::PreDraw() {
  // これから書き込むバックバッファのインデックスを取得する
  UINT backBufferIndex = swapChain_->GetCurrentBackBufferIndex();

  // 今回のバリアはTransition
  barrier_.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  // Noneにしておく
  barrier_.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  // バリアを張る対象のリソース。現在のバックバッファに対し行う
  barrier_.Transition.pResource = swapChainResources_[backBufferIndex].Get();
  // 遷移前(現在)のResourceState
  barrier_.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
  // 遷移後のResourceState
  barrier_.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
  // TransitionBarrierを張る
  commandList->ResourceBarrier(1, &barrier_);

  // 描画先のRTVとDSVを設定する
  D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle =
      dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
  commandList->OMSetRenderTargets(1, &rtvHandles_[backBufferIndex], false,
                                  &dsvHandle);
  ///exeの色
  float clearColor[] = {1.0f,1.0f,1.0f, 1.0f}; // RGBAの値。青っぽい色
  commandList->ClearRenderTargetView(rtvHandles_[backBufferIndex], clearColor,
                                     0, nullptr);

  commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0,
                                     0, nullptr);

  // 描画用のDescriptorHeapの設定
  /*Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap[] = {
      srvDescriptorHeap_.Get()};
  commandList->SetDescriptorHeaps(1, descriptorHeap->GetAddressOf());*/

  // コマンドを積む
  commandList->RSSetViewports(1, &viewport_);       // ビューポートを設定
  commandList->RSSetScissorRects(1, &scissorRect_); // シザー矩形を設定
}
/// <summary>
/// 描画後処理
/// </summary>
void DirectXCom::PostDraw() {
  // バックバッファの番号を取得
  UINT backBufferIndex = swapChain_->GetCurrentBackBufferIndex();

  // 画面に描く処理は終わり画面に映すので、状態を遷移
  // RenderTargetからPresentにする
  barrier_.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
  barrier_.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
  // TransitionBarrierを張る
  commandList->ResourceBarrier(1, &barrier_);

  // コマンドリストの内容を下記率させる。すべてのコマンドを積んでからCloseする
  hr = commandList->Close();
  // コマンドリストのCloseに失敗した場合はエラー
  assert(SUCCEEDED(hr));

  // GUPにコマンドリストの実行を行わせる
  ID3D12CommandList *commandLists[] = {commandList.Get()};
  commandQueue->ExecuteCommandLists(1, commandLists);
  // GUPとOSに画面の交換を要求する
  swapChain_->Present(1, 0);

  // Fenceの値を更新
  fenceValue_++;
  // GPUがここまでたどり着いたときに、Fenceの値を指定した値に代入するようにSignalを送る
  commandQueue->Signal(fence.Get(), fenceValue_);

  // Fenceの値が指定したSignalの値にたどり着いているか確認する
  // GetCompletedValueの初期値はFence作成時に渡した初期値
  if (fence->GetCompletedValue() < fenceValue_) {
    // 指定したSignalにたどり着いていないので、たどり着くまで待つようにイベントを設定する
    fence->SetEventOnCompletion(fenceValue_, fenceEvent_);

    UpdateFixFPS();

    // イベントを待つ
    WaitForSingleObject(fenceEvent_, INFINITE);
  }

  // 次フレーム用のコマンドリストを用意
  hr = commandAllocator->Reset();
  // コマンドアロケーターのリセットに失敗した場合はエラー
  assert(SUCCEEDED(hr));
  // コマンドリストをリセットする
  hr = commandList->Reset(commandAllocator.Get(), nullptr);
  // コマンドリストのリセットに失敗した場合はエラー
  assert(SUCCEEDED(hr));
}

/// <summary>
/// シェーダーのコンパイル
/// </summary>
/// <param name="filePath"></param>
/// <param name="profile"></param>
/// <returns></returns>
ComPtr<IDxcBlob> DirectXCom::CompileShader(
    // CompilerするShaderファイルへのパス
    const std::wstring &filePath,
    // Compilerに使用するProfile
    const wchar_t *profile) {
  // これからシェーダーをコンパイルする旨をログに出す
  Logger::Log(ConvertString(std::format(
      L"Begin CompileShader, path{},profile:{}\n", filePath, profile)));
  // hlslファイルを読み込む
  ComPtr<IDxcBlobEncoding> shaderScore = nullptr;
  HRESULT hr = dxcUtils_->LoadFile(filePath.c_str(), nullptr, &shaderScore);
  // ファイルの読み込みに失敗した場合はエラー
  assert(SUCCEEDED(hr));
  // 読み込んだファイルの内容を設定する
  DxcBuffer shaderSourceBuffer;
  shaderSourceBuffer.Ptr = shaderScore->GetBufferPointer();
  shaderSourceBuffer.Size = shaderScore->GetBufferSize();
  shaderSourceBuffer.Encoding =
      DXC_CP_UTF8; // UTF8の文字コードである事を通知する

  LPCWSTR arguments[] = {
      filePath.c_str(), // コンパイルするファイルのパス
      L"-E",
      L"main", // エントリーポイントの指定。基本的にmain以外にはしない
      L"-T",
      profile, // ShaderProfileの設定
      L"-Zi",
      L"Qembed_debug",
      L"-Od",  // 最適化を行わない
      L"-Zpr", // メモリレイアウトは行優先
  };

  // 実際にShaderをコンパイルする
  ComPtr<IDxcResult> shaderResult = nullptr;
  hr = dxcCompiler_->Compile(
      &shaderSourceBuffer,        // コンパイルするシェーダーの内容
      arguments,                  // コンパイル時の引数
      _countof(arguments),        // 引数の数
      includeHandler_.Get(),      // includeハンドラ
      IID_PPV_ARGS(&shaderResult) // 結果を受け取るポインタ
  );

  // 警告やエラーがあった場合はログに出力し停止する
  ComPtr<IDxcBlobUtf8> shaderError = nullptr;

#pragma warning(push)
#pragma warning(disable : 6387) // C6387 警告を抑制
  shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
#pragma warning(pop)

  if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
    Logger::Log(shaderError->GetStringPointer());

    // 警告やエラーがあった場合は、Shaderのコンパイルに失敗したとする
    assert(SUCCEEDED(hr));
  }

  // コンパイルの結果から実行用のバイナリ部分を取得
  ComPtr<IDxcBlob> shaderBlob = nullptr;
  hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob),
                               nullptr);
  // Shaderのコンパイルに失敗した場合はエラー
  assert(SUCCEEDED(hr));

  // Shaderのコンパイルに成功したので、ログに出力する
  Logger::Log(ConvertString(std::format(
      L"Complete CompileShader, path{},profile:{}\n", filePath, profile)));

  return shaderBlob; // コンパイルしたShaderのバイナリを返す
}

/// <summary>
/// バッファリソースの生成
/// </summary>
/// <param name="sizeInBytes"></param>
/// <returns></returns>
ComPtr<ID3D12Resource> DirectXCom::CreateBufferResource(size_t sizeInBytes) {
  // 頂点リソース用のヒープの設定
  D3D12_HEAP_PROPERTIES uploadHeapProperties{};
  uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD; // アップロード用のヒープ

  // 頂点リソースの設定（今回は汎用的なバッファとして設定）
  D3D12_RESOURCE_DESC resourceDesc{};
  resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; // バッファ
  resourceDesc.Width = sizeInBytes;                         // 指定されたサイズ
  // バッファの場合はこれらを1にする決まり
  resourceDesc.Height = 1;
  resourceDesc.DepthOrArraySize = 1;
  resourceDesc.MipLevels = 1;
  resourceDesc.SampleDesc.Count = 1;
  resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; // 行優先

  ComPtr<ID3D12Resource> bufferResource = nullptr;
  HRESULT hr = device_->CreateCommittedResource(
      &uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ, // データ書き込み用なのでREAD
      nullptr, IID_PPV_ARGS(&bufferResource));
  assert(SUCCEEDED(hr)); // 失敗したらassertで止める

  return bufferResource;
}

/// <summary>
/// テクスチャリソースの生成
/// </summary>
/// <param name="metadata"></param>
/// <returns></returns>
ComPtr<ID3D12Resource>
DirectXCom::CreateTextureResource(const DirectX::TexMetadata &metadata) {
  // 1. metadataを基にResourceの設定
  D3D12_RESOURCE_DESC resourceDesc{};
  resourceDesc.Width = UINT(metadata.width);           // Textureの幅
  resourceDesc.Height = UINT(metadata.height);         // Textureの高さ
  resourceDesc.MipLevels = UINT16(metadata.mipLevels); // mipmapの数
  resourceDesc.DepthOrArraySize =
      UINT16(metadata.arraySize);        // 奥行き or 配列Textureの配列数
  resourceDesc.Format = metadata.format; // TextureのFormat
  resourceDesc.SampleDesc.Count = 1;     // サンプリングカウント。1固定
  resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(
      metadata.dimension); // Textureの次元数。普段使っているのは2次元

  // 2. 利用するHeapの設定
  D3D12_HEAP_PROPERTIES heapProperties{};
  heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // 細かい設定を行う
  // heapProperties.CPUPageProperty =
  // D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;//WriteBackポリシーでCPUアクセス可能
  // heapProperties.MemoryPoolPreference =
  // D3D12_MEMORY_POOL_L0;//プロセッサの近くに配列

  // 3. Resourceを生成する
  Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
  HRESULT hr = device_->CreateCommittedResource(
      &heapProperties,                // Heapの設定
      D3D12_HEAP_FLAG_NONE,           // Heapの特殊な設定
      &resourceDesc,                  // Resourceの設定
      D3D12_RESOURCE_STATE_COPY_DEST, // 初回のResourceState。Textureは基本読むだけ
      nullptr,                        // Clear最適値。使わないのでnullptr
      IID_PPV_ARGS(&resource));       // 作成するResourceポインタへのポインタ
  assert(SUCCEEDED(hr));
  return resource;
}

/// <summary>
/// テクスチャデータの転送
/// </summary>
/// <param name="texture"></param>
/// <param name="mipImages"></param>
/// <returns></returns>
[[nodiscard]]
ComPtr<ID3D12Resource>
DirectXCom::UploadTextureData(ComPtr<ID3D12Resource> &texture,
                              const DirectX::ScratchImage &mipImages) {
  std::vector<D3D12_SUBRESOURCE_DATA> subresources;
  DirectX::PrepareUpload(device_.Get(), mipImages.GetImages(),
                         mipImages.GetImageCount(), mipImages.GetMetadata(),
                         subresources);
  uint64_t intermediateSize =
      GetRequiredIntermediateSize(texture.Get(), 0, UINT(subresources.size()));
  ComPtr<ID3D12Resource> intermediateResource =
      CreateBufferResource(intermediateSize);
  UpdateSubresources(commandList.Get(), texture.Get(),
                     intermediateResource.Get(), 0, 0,
                     UINT(subresources.size()), subresources.data());
  // textureへの転送後は利用できるよう、D3D12_RESOURCE_STATE_DESTからD3D12_RESOURCE_STATE_GENERIC_READへResourceStateを変更する
  D3D12_RESOURCE_BARRIER barrier{};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  barrier.Transition.pResource = texture.Get();
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
  commandList->ResourceBarrier(1, &barrier);
  return intermediateResource;
}

///// <summary>
///// テクスチャファイルの読み込み
///// </summary>
///// <param name="filePath"></param>
///// <returns></returns>
// DirectX::ScratchImage DirectXCom::LoadTexture(const std::string &filePath) {
//   // テクスチャファイルを読み込んでプログラムで使えるようにする
//   DirectX::ScratchImage image{};
//   std::wstring filePathW = ConvertString(filePath);
//   HRESULT hr = DirectX::LoadFromWICFile(
//       filePathW.c_str(), DirectX::WIC_FLAGS_DEFAULT_SRGB, nullptr, image);
//   assert(SUCCEEDED(hr));
//
//   // ミニマップの作成
//   DirectX::ScratchImage mipImages{};
//   hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(),
//                                 image.GetMetadata(),
//                                 DirectX::TEX_FILTER_SRGB, 0, mipImages);
//   assert(SUCCEEDED(hr));
//
//   // ミニマップ付きのデータを返す
//   return mipImages;
// }

/// <summary>
/// FPS固定初期化
/// </summary>
void DirectXCom::InitializeFixFPS() {
  // 現在時間を記録する
  refrence_ = std::chrono::steady_clock::now();
}
/// <summary>
/// FPS固定更新
/// </summary>
void DirectXCom::UpdateFixFPS() {
  // 1/60秒ぴったりの時間
  const std::chrono::microseconds kMinTime(uint64_t(1000000.0f / 60.0f));
  // 1/60秒よりわずかに短い時間
  const std::chrono::microseconds kMinCheckTime(uint64_t(1000000.0f / 65.0f));

  // 現在時間を取得
  std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
  // 前回記録からの経過時間を取得する
  std::chrono::microseconds elapsed =
      std::chrono::duration_cast<std::chrono::microseconds>(now - refrence_);

  // 1/60秒(わずかに短い時間)経っていない場合
  if (elapsed < kMinCheckTime) {
    // 1/60秒経過するまで微小なスリープを繰り返す
    while (std::chrono::steady_clock::now() - refrence_ < kMinTime) {
      // 1マイクロ秒スリープ
      std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
  }

  // 現在の時間を記録する
  refrence_ = std::chrono::steady_clock::now();
}