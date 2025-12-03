#include "TextureManager.h"
#include "SrvManager.h"

#include <cassert>

using namespace StringUtility;

TextureManager* TextureManager::instance_ = nullptr;
// ImGuiで0番を使用するため、1番から使用
uint32_t TextureManager::kSRVIndexTop = 1;

/// <summary>
/// シングルインスタンスの取得
/// </summary>
/// <returns></returns>
TextureManager* TextureManager::GetInstance()
{
	if (instance_ == nullptr)
	{
		instance_ = new TextureManager;
	}

	return instance_;
}
/// <summary>
/// 終了
/// </summary>
void TextureManager::Finalize()
{
	delete instance_;
	instance_ = nullptr;
}

/// <summary>
/// 初期化
/// </summary>
void TextureManager::Initialize(DirectXCom* directXCom,
	SrvManager* srvManager)
{
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
void TextureManager::LoadTexture(const std::string& filePath)
{
	// 既に読み込み済みならスキップ（キーは渡された文字列そのまま）
	if (textureDatas_.contains(filePath))
	{
		return;
	}

	assert(srvManager_->CanAllocate());

	// パス解決のための候補パスを構築
	std::vector<std::string> candidates;
	candidates.push_back(filePath);

	// 絶対パスでなく、かつ Resources で始まらない場合、プレフィックス付きバージョンを追加
	bool startsWithResources = (filePath.rfind("Resources", 0) == 0);
	bool isAbsoluteWin = (filePath.size() > 1 && std::isalpha(static_cast<unsigned char>(filePath[0])) && filePath[1] == ':');
	bool isAbsoluteUnix = (!filePath.empty() && (filePath[0] == '/' || filePath[0] == '\\'));
	if (!startsWithResources && !isAbsoluteWin && !isAbsoluteUnix) {
		candidates.push_back(std::string("Resources/") + filePath);
	}

	// 候補パスからの読み込みを試行
	DirectX::ScratchImage image{};
	HRESULT hr = E_FAIL;
	std::wstring triedPathW;
	for (const auto& c : candidates)
	{
		std::wstring w = ConvertString(c);
		hr = DirectX::LoadFromWICFile(w.c_str(), DirectX::WIC_FLAGS_DEFAULT_SRGB, nullptr, image);
		if (SUCCEEDED(hr)) {
			triedPathW = w;
			// 元のキーで保存し続けるが、ここでは終了（image にデータあり）
			break;
		}
	}

	assert(SUCCEEDED(hr));

	// ミニマップの作成
	DirectX::ScratchImage mipImages{};
	hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(),
		image.GetMetadata(), DirectX::TEX_FILTER_SRGB,
		0, mipImages);
	assert(SUCCEEDED(hr));

	// マップのキーは呼び出し元の filePath を使用（呼び出しと一致させる）
	TextureData& textureData = textureDatas_[filePath];

	textureData.metadata_ = mipImages.GetMetadata();
	textureData.resource_ =
		directXCom_->CreateTextureResource(textureData.metadata_);

	{
		ComPtr<ID3D12Resource> intermediate =
			directXCom_->UploadTextureData(textureData.resource_, mipImages);
		pendingUploadBuffers_.push_back(intermediate);
	}

	textureData.srvIndex_ = srvManager_->Allocate();
	textureData.srvHandleCPU_ =
		srvManager_->GetCPUDescriptorHandle(textureData.srvIndex_);
	textureData.srvHandleGPU_ =
		srvManager_->GetGPUDescriptorHandle(textureData.srvIndex_);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = textureData.metadata_.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = UINT(textureData.metadata_.mipLevels);

	directXCom_->GetDevice()->CreateShaderResourceView(
		textureData.resource_.Get(), &srvDesc, textureData.srvHandleCPU_);
}

/// <summary>
/// SRVインデックスの開始番号
/// </summary>
/// <param name="filePath"></param>
/// <returns></returns>
uint32_t
TextureManager::GetTextureIndexByFilePath(const std::string& filePath)
{
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
TextureManager::GetSrvHandleGPU(const std::string& filePath)
{
	assert(textureDatas_.contains(filePath));
	return textureDatas_[filePath].srvHandleGPU_;
}

/// <summary>
/// メタデータを取得
/// </summary>
/// <param name="textureIndex"></param>
/// <returns></returns>
const DirectX::TexMetadata& TextureManager::GetMetaData(const std::string& filePath)
{
	assert(textureDatas_.contains(filePath));
	return textureDatas_[filePath].metadata_;
}
/// <summary>
/// SRVインデックスの取得
/// </summary>
/// <param name="filePath"></param>
/// <returns></returns>
uint32_t TextureManager::GetSrvIndex(const std::string& filePath)
{
	assert(textureDatas_.contains(filePath));
	return textureDatas_[filePath].srvIndex_;
}