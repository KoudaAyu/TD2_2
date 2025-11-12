#pragma once
#include "Matrix4x4.h"
#include "Transform.h"
#include "Vector.h"

#include <cstdint>
#include <d3d12.h>
#include <string>
#include <wrl.h>

class SpriteCom;

class Sprite {
public:
  struct VertexData {
    Vector4 position;
    Vector2 texcoord;
    Vector3 normal;
  };

  struct Material {
    Vector4 color;
    int32_t enableLighting;
    float padding[3];      // パディングを追加して16バイト境界に揃える
    Matrix4x4 uvTransform; // UV変換行列
  };

  struct DirectionalLight {
    Vector4 color;
    Vector3 direction;
    float intensity;
  };

private:
  SpriteCom *spriteCom_ = nullptr;

  // テクスチャ番号
  uint32_t textureIndex_ = 0;

  /// ===============================
  /// 頂点リソース
  /// ===============================

  // バッファリソース
  Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_ = nullptr;
  Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_ = nullptr;

  // バッファリソース内のデータを指すポインタ
  VertexData *vertexData_ = nullptr;
  uint32_t *indexData_ = nullptr;

  // バッファリソースの使い道を補足するバッファビュー
  D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
  D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

  // テクスチャ
  D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU_{};

  Transform transform_ = {
      {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};

  // 座標
  Vector2 position_ = {0.0f, 0.0f};
  // 回転
  float rotation_ = 0.0f;
  // 拡縮
  Vector2 scale_ = {640.0f, 360.0f};
  // アンカーポイント
  Vector2 anchorPoint_ = {0.0f, 0.0f};
  // 左右反転
  bool isFlipX_ = false;
  // 上下反転
  bool isFlipY_ = false;
  // テクスチャの左上座標
  Vector2 textureLeftTop_ = {0.0f, 0.0f};
  // テクスチャ切り出しサイズ
  Vector2 textureSize_ = {100.0f, 100.0f};

  std::string textureFilePath_;

  /// ===============================
  /// マテリアルリソース
  /// ===============================

  // バッファリソース
  Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_ = nullptr;
  // バッファリソース内のデータを指すポインタ
  Material *materialData_ = nullptr;

  Microsoft::WRL::ComPtr<ID3D12Resource> directionalLight_ = nullptr;

  /// ===============================
  /// 座標変換行列リソース
  /// ===============================

  // バッファリソース
  Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource_ =
      nullptr;
  // バッファリソース内のデータを指すポインタ
  TransformationMatrix *transformationMatrixData_ = nullptr;

public:
  /// <summary>
  /// 初期化
  /// </summary>
  /// <param name="spriteCom"></param>
  void Initialize(SpriteCom *spriteCom, std::string textureFilePath);
  /// <summary>
  /// 更新
  /// </summary>
  void Update();
  /// <summary>
  /// 描画
  /// </summary>
  void Draw();

  void SetTextureSrvHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle) {
    textureSrvHandleGPU_ = handle;
  }

  D3D12_GPU_DESCRIPTOR_HANDLE GetTextureSrvHandle() const {
    return textureSrvHandleGPU_;
  }

  /// <summary>
  /// 拡縮
  /// </summary>
  /// <returns></returns>
  const Vector2 &GetScale() const { return scale_; }
  void SetScale(const Vector2 &scale) { scale_ = scale; }
  /// <summary>
  /// 回転用
  /// </summary>
  /// <returns></returns>
  float GetRotation() const { return rotation_; }
  void SetRotation(float rotation) { rotation_ = rotation; }
  /// <summary>
  /// 移動用
  /// </summary>
  /// <returns></returns>
  const Vector2 GetPosition() const { return position_; }
  void SetPosition(const Vector2 &position) { position_ = position; }
  /// <summary>
  /// 色用
  /// </summary>
  /// <returns></returns>
  const Vector4 &GetColor() const { return materialData_->color; }
  void SetColor(const Vector4 &color) { materialData_->color = color; }
  /// <summary>
  /// アンカーポイント用
  /// </summary>
  /// <returns></returns>
  const Vector2 &GetAnchorPoint() const { return anchorPoint_; }
  void SetAnchorPoint(const Vector2 &anchorPoint) {
    anchorPoint_ = anchorPoint;
  }
  /// <summary>
  /// 左右反転用
  /// </summary>
  /// <returns></returns>
  bool GetFlipX() const { return isFlipX_; }
  void SetFlipX(bool isFlipX) { isFlipX_ = isFlipX; }
  /// <summary>
  /// 上下反転用
  /// </summary>
  /// <returns></returns>
  bool GetFlipY() const { return isFlipY_; }
  void SetFlipY(bool isFlipY) { isFlipY_ = isFlipY; }
  /// <summary>
  /// テクスチャの左上座標
  /// </summary>
  /// <returns></returns>
  const Vector2 &GetTextureLeftTop() { return textureLeftTop_; }
  void SetTextureLeftTop(const Vector2 &textureLeftTop) {
    textureLeftTop_ = textureLeftTop;
  }
  /// <summary>
  /// テクスチャ切り出しサイズ
  /// </summary>
  /// <returns></returns>
  const Vector2 &GetTextureSize() { return textureSize_; }
  void SetTextureSize(const Vector2 &textureSize) {
    textureSize_ = textureSize;
  }

private:
  /// <summary>
  /// テクスチャサイズをイメージに合わせる
  /// </summary>
  void AdjustTextureSize();
};
