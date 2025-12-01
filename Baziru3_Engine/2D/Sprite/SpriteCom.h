#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <string>

#include "DirectXCom.h"
#include "Vector.h"

class Sprite;

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

  /// <summary>
  /// Sprite生成ヘルパー
  /// 生成 -> Initialize -> 各種プロパティ設定 を一括実行
  /// 呼び出し側で delete が必要
  /// </summary>
  /// <param name="textureFilePath">テクスチャファイルパス</param>
  /// <param name="position">初期座標 (左上基準)</param>
  /// <param name="scale">初期スケール (Initialize後にテクスチャサイズへ自動調整されるため、上書きしたい場合に指定)</param>
  /// <param name="rotation">Z回転(ラジアン)</param>
  /// <param name="anchorPoint">アンカーポイント</param>
  /// <param name="flipX">左右反転</param>
  /// <param name="flipY">上下反転</param>
  /// <returns>生成された Sprite*</returns>
  Sprite* CreateSprite(const std::string& textureFilePath,
                       const Vector2& position = {0.0f, 0.0f},
                       const Vector2& scale = {0.0f, 0.0f},
                       float rotation = 0.0f,
                       const Vector2& anchorPoint = {0.0f, 0.0f},
                       bool flipX = false,
                       bool flipY = false);
};
