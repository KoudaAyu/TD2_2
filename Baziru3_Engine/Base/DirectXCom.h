#pragma once

#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi.h>
#include <dxgi1_6.h>

// DXCの初期化
#include <dxcapi.h>
#pragma comment(lib, "dxcompiler.lib")

#include <array>
#include <wrl.h>

#include "Logger.h"
#include "StringUtility.h"
#include "WinApp.h"
#include "externals/DirectXTex/DirectXTex.h"
#include <chrono>

class DirectXCom {
public:
  ~DirectXCom();

  void Initialize(WinApp *winApp);

  void CreateFactory();
  HRESULT GetHr() const { return hr; }
  ID3D12Device *GetDevice() const { return device_.Get(); }
  ID3D12GraphicsCommandList *GetCommandList() const {
    return commandList.Get();
  }

  Microsoft::WRL::ComPtr<IDXGIFactory7> GetDxgiFactory() const {
    return dxgiFactory_;
  }

  uint32_t GetClientWidth() { return winApp_->kClientWidth; }
  uint32_t GetClientHeight() { return winApp_->kClientHeight; }

  size_t GetSwapChainResourcesNum() const { return swapChainResources_.size(); }

  ///// <summary>
  ///// SRVのCPUデスクリプタハンドルを取得
  ///// </summary>
  ///// <param name="index"></param>
  ///// <returns></returns>
  //D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUDescriptorHnadle(uint32_t index);
  ///// <summary>
  ///// SRVのGPUデスクリプタハンドルを取得
  ///// </summary>
  ///// <param name="index"></param>
  ///// <returns></returns>
  //D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUDescriptorHnadle(uint32_t index);

  /// <summary>
  /// シェーダーのコンパイル
  /// </summary>
  /// <param name="filePath"></param>
  /// <param name="profile"></param>
  /// <returns></returns>
  Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(
      // CompilerするShaderファイルへのパス
      const std::wstring &filePath,
      // Compilerに使用するProfile
      const wchar_t *profile);

  /// <summary>
  /// バッファリソースの生成
  /// </summary>
  /// <param name="sizeInBytes"></param>
  /// <returns></returns>
  Microsoft::WRL::ComPtr<ID3D12Resource>
  CreateBufferResource(size_t sizeInBytes);
  /// <summary>
  /// テクスチャリソースの生成
  /// </summary>
  /// <param name="metadata"></param>
  /// <returns></returns>
  Microsoft::WRL::ComPtr<ID3D12Resource>
  CreateTextureResource(const DirectX::TexMetadata &metadata);
  /// <summary>
  /// テクスチャデータの転送
  /// </summary>
  /// <param name="texture"></param>
  /// <param name="mipImages"></param>
  /// <returns></returns>
  [[nodiscard]]
  Microsoft::WRL::ComPtr<ID3D12Resource>
  UploadTextureData(Microsoft::WRL::ComPtr<ID3D12Resource> &texture,
                    const DirectX::ScratchImage &mipImages);
  /*/// <summary>
  /// テクスチャファイルの読み込み
  /// </summary>
  /// <param name="filePath"></param>
  /// <returns></returns>
  static DirectX::ScratchImage LoadTexture(const std::string &filePath);*/

  /// <summary>
  ///  深度バッファの生成
  /// </summary>
  /// <returns></returns>
  Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTextureResource();

  /// <summary>
  /// デスクリプタヒープの生成
  /// </summary>
  /// <param name="heapType"></param>
  /// <param name="numDescriptors"></param>
  /// <param name="shaderVisible"></param>
  /// <returns></returns>
  ComPtr<ID3D12DescriptorHeap>
  CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors,
                       bool shaderVisible);

  /// <summary>
  /// 描画前処理
  /// </summary>
  void PreDraw();
  /// <summary>
  /// 描画後処理
  /// </summary>
  void PostDraw();

private:
  /// <summary>
  ///  デバイスの初期化
  /// </summary>
  void InitializeDevice();
  /// <summary>
  /// コマンドの初期化
  /// </summary>
  void InitializeCommand();
  /// <summary>
  ///  スワップチェーンの初期化
  /// </summary>
  void CreateSwapChain();

  /// <summary>
  /// 各種デスクリプタヒープを生成
  /// </summary>
  void CreateDescriptorHeaps();

  /// <summary>
  /// レンダーターゲットビューの初期化
  /// </summary>
  void InitializeRenderTargetView();

  /// <summary>
  /// CPUでスクリプタハンドルを取得
  /// </summary>
  /// <param name="descriptorHeap"></param>
  /// <param name="descriptorSize"></param>
  /// <param name="index"></param>
  /// <returns></returns>
  static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(
      Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> &descriptorHeap,
      uint32_t descriptorSize, uint32_t index);

  /// <summary>
  /// GPUでスクリプタハンドルを取得
  /// </summary>
  /// <param name="descriptorHeap"></param>
  /// <param name="descriptorSize"></param>
  /// <param name="index"></param>
  /// <returns></returns>
  static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(
      Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> &descriptorHeap,
      uint32_t descriptorSize, uint32_t index);

  /// <summary>
  /// 深度ステンシルビューの初期化
  /// </summary>
  void InitializeDepthStencilView();

  /// <summary>
  /// フェンスの生成
  /// </summary>
  void CreateFence();
  /// <summary>
  ///  ビューポート矩形の設定
  /// </summary>
  void InitializeViewportRect();
  /// <summary>
  /// シザリング矩形の初期化
  /// </summary>
  void InitializeScissorRect();
  /// <summary>
  /// DXCコンパイラの生成
  /// </summary>
  void CreateDXCCompiler();
  /// <summary>
  /// imguiの初期化
  /// </summary>
  void InitializeImGui();

  /// <summary>
  /// FPS固定初期化
  /// </summary>
  void InitializeFixFPS();
  /// <summary>
  /// FPS固定更新
  /// </summary>
  void UpdateFixFPS();

private:
  Microsoft::WRL::ComPtr<ID3D12Debug1> debugController_ = nullptr;
  Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_ = nullptr;
  Microsoft::WRL::ComPtr<ID3D12Device> device_ = nullptr;

  // コマンドキュー
  Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue = nullptr;
  // コマンドアロケータ
  Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator = nullptr;
  // コマンドリスト
  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList = nullptr;

  // スワップチェーン
  Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_ = nullptr;
  // スワップチェーンを生成する
  DXGI_SWAP_CHAIN_DESC1 swapChainDesc_{};

  // 深度バッファリソース
  Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource = nullptr;

  D3D12_RENDER_TARGET_VIEW_DESC rtvDesc_{};
  std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 2> rtvHandles_{};

  // デスクリプタヒープサイズ
  /*uint32_t descriptorSizeSRV_;*/
  uint32_t descriptorSizeRTV_;
  uint32_t descriptorSizeDSV_;

  // デスクリプタヒープ
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap_ = nullptr;
  /*Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap_ = nullptr;*/
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap_ = nullptr;

  // スワップチェーンリソース
  std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, 2> swapChainResources_;

  // フェンス
  Microsoft::WRL::ComPtr<ID3D12Fence> fence = nullptr;

  // ビューポート矩形
  D3D12_VIEWPORT viewport_{};
  // シザー矩形
  D3D12_RECT scissorRect_{};

  // DXC
  Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils_ = nullptr;
  Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler_ = nullptr;
  Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler_ = nullptr;

  // TransitionBarrierの設定
  D3D12_RESOURCE_BARRIER barrier_{};

  // フェンス
  uint64_t fenceValue_ = 0;
  HANDLE fenceEvent_ = nullptr;

  // 記録時間(FPS固定用)
  std::chrono::steady_clock::time_point refrence_;

  // WindowsAPI
  WinApp *winApp_ = nullptr;

  HRESULT hr;

public:
  //// 最大SRV数（最大テクスチャ枚数）
  //static const uint32_t kMaxSRVCount;
};