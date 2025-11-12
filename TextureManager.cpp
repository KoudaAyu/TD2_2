#include "TextureManager.h"
#include "SrvManager.h"

#include <cassert>

using namespace StringUtility;

TextureManager *TextureManager::instance_ = nullptr;
// ImGuiで0番を使用するため、1番から使用
uint32_t TextureManager::kSRVIndexTop = 1;

/// <summary>
/// シングルインスタンスの取得
/// </summary>
/// <returns></returns>
TextureManager *TextureManager::GetInstance() {
  if (instance_ == nullptr) {
    instance_ = new TextureManager;
  }

  return instance_;
}
/// <summary>
/// 終了
/// </summary>
void TextureManager::Finalize() {
  delete instance_;
  instance_ = nullptr;
}

/// <summary>
/// 初期化
/// </summary>
void TextureManager::Initialize(DirectXCom *directXCom,
                                SrvManager *srvManager) {
  directXCom_ = directXCom;
  srvManager_ = srvManager;

  // SRVの数と同数
  textureDatas_.reserve(SrvManager::kMaxSRVCount);
}

/// <summary>
/// テクスチャファイルの読み込み
/// </summary>
/// <param name="filePath"></param>
/// <returns></returns>
void TextureManager::LoadTexture(const std::string &filePath) {
  // 読み込み済みテクスチャを検索
  /*auto it = std::find_if(textureDatas_.begin(), textureDatas_.end(),
                         [&](TextureData &textureData) {
                           return textureData.filePath_ == filePath;
                         });*/

  if (textureDatas_.contains(filePath)) {
    return;
  }

  // テクスチャ枚数上限チェック
  assert(srvManager_->CanAllocate());

  // テクスチャ枚数上限チェック
  // assert(textureDatas_.size() + kSRVIndexTop < SrvManager::kMaxSRVCount);

  // if (it != textureDatas_.end()) {
  //   // 読み込み済みなら早期リターン
  //   return;
  // }

  // テクスチャファイルを読み込んでプログラムで使えるようにする
  DirectX::ScratchImage image{};
  std::wstring filePathW = ConvertString(filePath);
  HRESULT hr = DirectX::LoadFromWICFile(
      filePathW.c_str(), DirectX::WIC_FLAGS_DEFAULT_SRGB, nullptr, image);
  assert(SUCCEEDED(hr));

  // ミニマップの作成
  DirectX::ScratchImage mipImages{};
  hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(),
                                image.GetMetadata(), DirectX::TEX_FILTER_SRGB,
                                0, mipImages);
  assert(SUCCEEDED(hr));

  //// テクスチャデータを追加
  //textureDatas_.resize(textureDatas_.size() + 1);
  // 追加したテクスチャデータの参照を取得
  TextureData &textureData = textureDatas_[filePath];

  /*textureData.filePath_ = filePath;*/
  textureData.metadata_ = mipImages.GetMetadata();
  textureData.resource_ =
      directXCom_->CreateTextureResource(textureData.metadata_);

  {
    ComPtr<ID3D12Resource> intermediate =
        directXCom_->UploadTextureData(textureData.resource_, mipImages);
    pendingUploadBuffers_.push_back(intermediate);
  }

  // テクスチャデータの要素数番号をSRVのインデックスとする
  textureData.srvIndex_ = srvManager_->Allocate();

  textureData.srvHandleCPU_ =
      srvManager_->GetCPUDescriptorHandle(textureData.srvIndex_);
  textureData.srvHandleGPU_ =
      srvManager_->GetGPUDescriptorHandle(textureData.srvIndex_);

  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
  // metaDataを基にSRVの設定
  srvDesc.Format = textureData.metadata_.format;
  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
  srvDesc.Texture2D.MipLevels = UINT(textureData.metadata_.mipLevels);

  //// SRVを生成するDescriptorHeapの場所を決める
  // D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU =
  //     directXCom_->GetSRVCPUDescriptorHnadle(
  //         1); // ImGui が 0 を使っているので 1 番から
  // D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU =
  //     directXCom_->GetSRVGPUDescriptorHnadle(1);

  // D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU2 =
  //     directXCom_->GetSRVCPUDescriptorHnadle(2);
  // D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU2 =
  //     directXCom_->GetSRVGPUDescriptorHnadle(2);

  // SRVの生成
  directXCom_->GetDevice()->CreateShaderResourceView(
      textureData.resource_.Get(), &srvDesc, textureData.srvHandleCPU_);
}

/// <summary>
/// SRVインデックスの開始番号
/// </summary>
/// <param name="filePath"></param>
/// <returns></returns>
uint32_t
TextureManager::GetTextureIndexByFilePath(const std::string &filePath) {
  // 読み込み済みであることを確認
  assert(textureDatas_.contains(filePath));

  // 対応するSRVインデックスを返す
  return textureDatas_.at(filePath).srvIndex_;
}
/// <summary>
/// テクスチャ番号からGPUハンドルを取得
/// </summary>
/// <param name="textureIndex"></param>
/// <returns></returns>
D3D12_GPU_DESCRIPTOR_HANDLE
TextureManager::GetSrvHandleGPU(const std::string &filePath) {
  assert(textureDatas_.contains(filePath));
  return textureDatas_[filePath].srvHandleGPU_;
}

/// <summary>
/// メタデータを取得
/// </summary>
/// <param name="textureIndex"></param>
/// <returns></returns>
const DirectX::TexMetadata& TextureManager::GetMetaData(const std::string &filePath) {
  assert(textureDatas_.contains(filePath));
  return textureDatas_[filePath].metadata_;
}
/// <summary>
/// SRVインデックスの取得
/// </summary>
/// <param name="filePath"></param>
/// <returns></returns>
uint32_t TextureManager::GetSrvIndex(const std::string& filePath) {
  assert(textureDatas_.contains(filePath));
  return textureDatas_[filePath].srvIndex_;
}