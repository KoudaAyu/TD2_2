#pragma once
#include "externals/DirectXTex/DirectXTex.h"
#include <d3d12.h>
#include <string>
#include <wrl.h>
#include<unordered_map>

#include "DirectXCom.h"

class SrvManager;

class TextureManager {
private:
  static TextureManager *instance_;

  // テクスチャ1枚分のデータ
  struct TextureData {/*
    std::string filePath_;*/
    DirectX::TexMetadata metadata_;
    Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
    uint32_t srvIndex_;
    D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU_;
    D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU_;
  };

  //// テクスチャデータ
  //std::vector<TextureData> textureDatas_;

  // SRVインデックスの開始番号
  static uint32_t kSRVIndexTop;

  DirectXCom *directXCom_ = nullptr;

  std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> pendingUploadBuffers_;

  //SRVマネージャ
  SrvManager *srvManager_ = nullptr;

  //テクスチャデータ
  std::unordered_map<std::string, TextureData> textureDatas_;

private:
  TextureManager() = default;
  ~TextureManager() = default;
  TextureManager(TextureManager &) = delete;
  TextureManager &operator=(TextureManager &) = delete;

public:
  /// <summary>
  /// シングルインスタンスの取得
  /// </summary>
  /// <returns></returns>
  static TextureManager *GetInstance();
  /// <summary>
  /// 終了
  /// </summary>
  void Finalize();

  /// <summary>
  /// 初期化
  /// </summary>
  void Initialize(DirectXCom *directXCom, SrvManager* srvManager);

  /// <summary>
  /// テクスチャファイルの読み込み
  /// </summary>
  /// <param name="filePath"></param>
  /// <returns></returns>
  void LoadTexture(const std::string &filePath);

  /// <summary>
  /// SRVインデックスの開始番号
  /// </summary>
  /// <param name="filePath"></param>
  /// <returns></returns>
  uint32_t GetTextureIndexByFilePath(const std::string &filePath);
  /// <summary>
  /// テクスチャ番号からGPUハンドルを取得
  /// </summary>
  /// <param name="textureIndex"></param>
  /// <returns></returns>
  D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU(const std::string& filePath);

  /// <summary>
  /// メタデータを取得
  /// </summary>
  /// <param name="textureIndex"></param>
  /// <returns></returns>
  const DirectX::TexMetadata &GetMetaData(const std::string& filePath);
  /// <summary>
  /// SRVインデックスの取得
  /// </summary>
  /// <param name="filePath"></param>
  /// <returns></returns>
  uint32_t GetSrvIndex(const std::string &filePath);
};
