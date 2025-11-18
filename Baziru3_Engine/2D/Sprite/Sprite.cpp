#include "Sprite.h"
#include "SpriteCom.h"
#include "TextureManager.h"

// 初期化
void Sprite::Initialize(SpriteCom *spriteCom, std::string textureFilePath) {

  spriteCom_ = spriteCom;

  textureFilePath_ = textureFilePath;

  // 必要なテクスチャを事前にロードしておく（以降のメタデータ/ハンドル取得で assert 回避）
  TextureManager::GetInstance()->LoadTexture(textureFilePath_);

  /// ==================================
  /// 頂点リソース
  /// ==================================
  vertexResource_ =
      spriteCom_->GetDirectXCom()->CreateBufferResource(sizeof(VertexData) * 6);
  // リソースの先頭のアドレスから使う
  vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
  // 使用するリソースのサイズは頂点6つ分のサイズ
  vertexBufferView_.SizeInBytes = sizeof(VertexData) * 6;
  // 1頂点当たりのサイズ
  vertexBufferView_.StrideInBytes = sizeof(VertexData);

  indexResource_ =
      spriteCom_->GetDirectXCom()->CreateBufferResource(sizeof(uint32_t) * 6);

  // リソースの先頭アドレスから使う
  indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
  // 使用するリソースのサイズは頂点6つ分のサイズ
  indexBufferView_.SizeInBytes = sizeof(uint32_t) * 6;
  // インデックスはuint32_tとする
  indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

  // インデックスリソースにデータを書き込む
  indexResource_->Map(0, nullptr, reinterpret_cast<void **>(&indexData_));
  indexData_[0] = 0; // 左下
  indexData_[1] = 1; // 左上
  indexData_[2] = 2; // 右下
  indexData_[3] = 2; // 右下
  indexData_[4] = 1; // 左上
  indexData_[5] = 3; // 右上

  indexResource_->Unmap(0, nullptr);

  // Sprite用
  vertexResource_->Map(0, nullptr, reinterpret_cast<void **>(&vertexData_));
  // 一枚目の三角形
  vertexData_[0].position = {0.0f, 360.0f, 0.0f, 1.0f};   // 左下
  vertexData_[1].position = {0.0f, 0.0f, 0.0f, 1.0f};     // 左上
  vertexData_[2].position = {640.0f, 360.0f, 0.0f, 1.0f}; // 右下
  vertexData_[3].position = {640.0f, 0.0f, 0.0f, 1.0f};   // 右上

  vertexData_[0].texcoord = {0.0f, 1.0f};
  vertexData_[1].texcoord = {0.0f, 0.0f};
  vertexData_[2].texcoord = {1.0f, 1.0f};
  vertexData_[3].texcoord = {1.0f, 0.0f};

  transform_.SetScale({ scale_.x, scale_.y, 1.0f });

  /// ==================================
  /// マテリアルリソース
  /// ==================================

  // マテリアル用のリソースを作る
  materialResource_ =
      spriteCom_->GetDirectXCom()->CreateBufferResource(sizeof(Material));
  // 書き込む為のアドレス取得
  materialResource_->Map(0, nullptr, reinterpret_cast<void **>(&materialData_));
  // データを設定（赤色 RGBA: 1,0,0,1）
  Vector4 temp{};
  temp.x = 1.0f;
  temp.y = 1.0f;
  temp.z = 1.0f;
  temp.w = 1.0f;
  materialData_->color = temp;
  materialData_->enableLighting = false;
  materialResource_->Unmap(0, nullptr);

  directionalLight_ = spriteCom_->GetDirectXCom()->CreateBufferResource(
      sizeof(DirectionalLight));

  // MapしてGPUリソースのCPU側の書き込み可能ポインタを取得する
  DirectionalLight *directionalLightData = nullptr;
  directionalLight_->Map(0, nullptr,
                         reinterpret_cast<void **>(&directionalLightData));

  // directionalLightDataに値を書き込む
  directionalLightData->color = {1.0f, 1.0f, 1.0f, 1.0f};
  directionalLightData->direction = {0.0f, -1.0f, 0.0f};
  directionalLightData->intensity = 1.0f;

  // 書き込み完了後はUnmapを呼ぶ
  directionalLight_->Unmap(0, nullptr);

  materialResource_ =
      spriteCom_->GetDirectXCom()->CreateBufferResource(sizeof(Material));

  materialResource_->Map(0, nullptr, reinterpret_cast<void **>(&materialData_));
  materialData_->color = {1.0f, 1.0f, 1.0f,
                          1.0f}; // 白（テクスチャ色をそのまま出す用）
  materialData_->enableLighting = false;
  materialData_->uvTransform = MakeIdentity4x4();

  /// ==================================
  /// 座標変換行列リソース
  /// ==================================

  // Sprite用のTransformationMatrix用のリソースを作る
  transformationMatrixResource_ =
      spriteCom_->GetDirectXCom()->CreateBufferResource(
          sizeof(TransformationMatrix));
  // 書き込むためのアドレス取得
  transformationMatrixResource_->Map(
      0, nullptr, reinterpret_cast<void **>(&transformationMatrixData_));
  // 単位行列を書き込んでおく
  transformationMatrixData_->WVP = MakeIdentity4x4();
  transformationMatrixData_->World = MakeIdentity4x4();

  // テクスチャインデックス取得（LoadTexture済み）
  textureIndex_ =
      TextureManager::GetInstance()->GetTextureIndexByFilePath(textureFilePath_);

  AdjustTextureSize();
}
/// <summary>
/// 更新
/// </summary>
void Sprite::Update() {

  // 拡縮、回転、移動
  transform_ = {{scale_.x, scale_.y, 1.0f},
                {0.0f, 0.0, rotation_},
                {position_.x, position_.y, 0.0f}};

  // アンカーポイント
  float left = 0.0f - anchorPoint_.x;
  float right = 1.0f - anchorPoint_.x;
  float top = 0.0f - anchorPoint_.y;
  float bottom = 1.0f - anchorPoint_.y;

  // 左右反転
  if (isFlipX_) {
    left = -left;
    right = -right;
  }
  // 上下反転
  if (isFlipY_) {
    top = -top;
    bottom = -bottom;
  }

  const DirectX::TexMetadata &metadata =
      TextureManager::GetInstance()->GetMetaData(textureFilePath_);

  float tex_left = textureLeftTop_.x / metadata.width;
  float tex_right = (textureLeftTop_.x + textureSize_.x) / metadata.width;
  float tex_top = textureLeftTop_.y / metadata.height;
  float tex_bottom = (textureLeftTop_.y + textureSize_.y) / metadata.height;

  // 左下
  vertexData_[0].position = {left, bottom, 0.0f, 1.0f};
  vertexData_[0].texcoord = {tex_left, tex_bottom};
  vertexData_[0].normal = {0.0f, 0.0f, -1.0f};
  // 左上
  vertexData_[1].position = {left, top, 0.0f, 1.0f};
  vertexData_[1].texcoord = {tex_left, tex_top};
  vertexData_[1].normal = {0.0f, 0.0f, -1.0f};
  // 右下
  vertexData_[2].position = {right, bottom, 0.0f, 1.0f};
  vertexData_[2].texcoord = {tex_right, tex_bottom};
  vertexData_[2].normal = {0.0f, 0.0f, -1.0f};
  // 右上
  vertexData_[3].position = {right, top, 0.0f, 1.0f};
  vertexData_[3].texcoord = {tex_right, tex_top};
  vertexData_[3].normal = {0.0f, 0.0f, -1.0f};

  Matrix4x4 viewMatrix = MakeIdentity4x4();

  // Sprite用のworldViewProjectMatrix
  Matrix4x4 worldMatrixSprite = MakeAffineMatrix(
      transform_.GetScale(), transform_.GetRotate(), transform_.GetTranslate());
  Matrix4x4 viewMatrixSprite = MakeIdentity4x4();
  Matrix4x4 projectionMatrixSprite = MakeOrthographicMatrix(
      0.0f, 0.0f, float(spriteCom_->GetDirectXCom()->GetClientWidth()),
      float(spriteCom_->GetDirectXCom()->GetClientHeight()), 0.0f, 100.0f);
  Matrix4x4 worldViewProjectionmatrixSprite =
      Multiply(worldMatrixSprite, Multiply(viewMatrix, projectionMatrixSprite));
  transformationMatrixData_->WVP = worldViewProjectionmatrixSprite;
  transformationMatrixData_->World = worldMatrixSprite;

  Transform uvTransform_ = {
      {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};

  // UVTransform用
  Matrix4x4 uvTransformMatrix = MakeScaleMatrix(uvTransform_.GetScale());
  uvTransformMatrix =
      Multiply(uvTransformMatrix, MakeRotateZMatrix(uvTransform_.GetRotate().z));
  uvTransformMatrix =
      Multiply(uvTransformMatrix, MakeTranslateMatrix(uvTransform_.GetTranslate()));
  materialData_->uvTransform = uvTransformMatrix;
}
/// <summary>
/// 描画
/// </summary>
void Sprite::Draw() {
  // 必要なPSO/RootSignatureを確実にセット
  spriteCom_->ApplyCommonRenderState();

  spriteCom_->GetDirectXCom()->GetCommandList()->IASetVertexBuffers(
      0, 1, &vertexBufferView_);
  spriteCom_->GetDirectXCom()->GetCommandList()->IASetIndexBuffer(
      &indexBufferView_);
  spriteCom_->GetDirectXCom()->GetCommandList()->IASetPrimitiveTopology(
      D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  spriteCom_->GetDirectXCom()
      ->GetCommandList()
      ->SetGraphicsRootConstantBufferView(
          0, materialResource_->GetGPUVirtualAddress());
  spriteCom_->GetDirectXCom()
      ->GetCommandList()
      ->SetGraphicsRootConstantBufferView(
          1, transformationMatrixResource_->GetGPUVirtualAddress());
  spriteCom_->GetDirectXCom()
      ->GetCommandList()
      ->SetGraphicsRootConstantBufferView(
          3, directionalLight_->GetGPUVirtualAddress());

  // 事前に設定されたSRVハンドルがある場合はそれを使う
  D3D12_GPU_DESCRIPTOR_HANDLE handle = textureSrvHandleGPU_.ptr != 0
                                           ? textureSrvHandleGPU_
                                           : TextureManager::GetInstance()->GetSrvHandleGPU(textureFilePath_);
  spriteCom_->GetDirectXCom()->GetCommandList()->SetGraphicsRootDescriptorTable(
      2, handle);

  spriteCom_->GetDirectXCom()->GetCommandList()->DrawIndexedInstanced(6, 1, 0,
                                                                      0, 0);
}

/// <summary>
/// テクスチャサイズをイメージに合わせる
/// </summary>
void Sprite::AdjustTextureSize() {
  // テクスチャメタデータを取得
  const DirectX::TexMetadata &metadata =
      TextureManager::GetInstance()->GetMetaData(textureFilePath_);
  textureSize_.x = static_cast<float>(metadata.width);
  textureSize_.y = static_cast<float>(metadata.height);
  // 画像サイズをテクスチャサイズに合わせる
  scale_ = textureSize_;
}