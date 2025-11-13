#pragma once
#include <d3d12.h>
#include <wrl.h>

#include "DirectXCom.h"
#include "Vector.h" // Camera生成ヘルパー用（Vector3）

class Camera;

class Object3dCom {
private:
	//DirectXCom
  DirectXCom *directXCom_;

  HRESULT hr;

  // ルートシグネチャ
  Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
  // グラフィックスパイプラインステート（CCW/CW の2種類）
  Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineFrontCCW_ = nullptr;
  Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineFrontCW_ = nullptr;

  //カメラ
  Camera *defaultCamera_ = nullptr;

public:
  /// <summary>
  /// 初期化
  /// </summary>
  void Initialize(DirectXCom *directXCom);

  /// <summary>
  /// 共通描画設定（デフォルト: Front=CCW）
  /// </summary>
  void ApplyCommonRenderState();

  /// <summary>
  /// 共通描画設定（Frontの向きを選択）
  /// </summary>
  void ApplyCommonRenderState(bool frontCCW) {
    // RootSignatureを設定
    directXCom_->GetCommandList()->SetGraphicsRootSignature(rootSignature_.Get());
    // PSOを選択
    auto pso = frontCCW ? pipelineFrontCCW_.Get() : pipelineFrontCW_.Get();
    directXCom_->GetCommandList()->SetPipelineState(pso);
    directXCom_->GetCommandList()->IASetPrimitiveTopology(
        D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  }

  DirectXCom *GetDirectXCom() { return directXCom_; }

  void SetDefaultCamera(Camera *defaultCamera) {
    defaultCamera_ = defaultCamera;
  }
  Camera *GetDefaultCamera() const { return defaultCamera_; }

  /// <summary>
  /// デフォルトカメラ生成ヘルパー
  /// 生成 -> 位置設定 -> Update -> デフォルト登録 を一括で行う
  /// 呼び出し側で delete してください
  /// </summary>
  /// <param name="translate">初期位置 (既定 {-5 の Z 位置など好みに変更)</param>
  /// <returns>生成された Camera*</returns>
  Camera* CreateDefaultCamera(const Vector3& translate = {0.0f, 0.0f, -5.0f});

  private:
  /// <summary>
  /// ルートシグネチャの作成
  /// </summary>
  void CreateRootSignature();
  /// <summary>
  /// グラフィックスパイプラインの生成
  /// </summary>
  void CreateGraphicsPipeline();
};
