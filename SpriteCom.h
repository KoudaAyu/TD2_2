#pragma once
#include <d3d12.h>
#include <wrl.h>

#include "DirectXCom.h"

class SpriteCom {
private:
  DirectXCom *directXCom_;

  HRESULT hr;

  // ルートシグネチャ
  Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
  // グラフィックスパイプラインステート
  Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicPipelineState_ = nullptr;

public:
  DirectXCom *GetDirectXCom() const { return directXCom_; }

private:
  /// <summary>
  /// ルートシグネチャの作成
  /// </summary>
  void CreateRootSignature();
  /// <summary>
  /// グラフィックスパイプラインの生成
  /// </summary>
  void CreateGraphicsPipeline();

public:
  /// <summary>
  /// 初期化
  /// </summary>
  /// <param name="directXCom"></param>
  void Initialize(DirectXCom *directXCom);

  /// <summary>
  /// 共通描画設定
  /// </summary>
  void ApplyCommonRenderState();
};
