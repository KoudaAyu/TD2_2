#pragma once
#include <map>        // std::map
#include <string>     // std::string
#include <memory>     // std::unique_ptr

class Model;
class ModelCommon;
class DirectXCom;

class ModelManager
{
public:
    // 初期化
    void Initialize(DirectXCom* dxCommon);
    // シングルトンインスタンスの取得
    static ModelManager* GetInstance();
    // 終了
    void Finalize();
    // モデルファイルの読み込み
    void LoadModel(const std::string& filePath);
    // モデルの検索
    Model* FindModel(const std::string& filePath);
	
   
private:
	static ModelManager* instance;

    // シングルトンなので、コンストラクタなどをprivateにする
    ModelManager() = default;
    ~ModelManager() = default;
    ModelManager(const ModelManager&) = delete;
    const ModelManager& operator=(const ModelManager&) = delete;

    // モデルデータ
    std::map<std::string, std::unique_ptr<Model>> models;

    ModelCommon* modelCommon = nullptr;
};

