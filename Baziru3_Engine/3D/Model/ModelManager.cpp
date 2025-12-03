#include "ModelManager.h"
#include "ModelCommon.h"
#include "Model.h"

ModelManager* ModelManager::instance = nullptr;

void ModelManager::Initialize(DirectXCom* dxCommon)
{
    modelCommon = new ModelCommon;
    modelCommon->Initialize(dxCommon);
}

// GetInstance
ModelManager* ModelManager::GetInstance() {
    if (instance == nullptr) {
        instance = new ModelManager;
    }
    return instance;
}

// Finalize
void ModelManager::Finalize() {
    delete instance;
    instance = nullptr;
}

void ModelManager::LoadModel(const std::string& filePath)
{
    // 読み込み済みのモデル検索
    if (models.contains(filePath)) {
        // すでに読み込み済み
        return;
    }

    // 渡されたパスからディレクトリとファイル名を抽出する
    // 例) "Resources/Enemy/Enemy.obj" -> directory: "Resources/Enemy", filename: "Enemy.obj"
    std::string directory = "Resources"; // デフォルトディレクトリ
    std::string filename = filePath;

    // スラッシュまたはバックスラッシュを考慮して末尾の区切り位置を探す
    size_t posSlash = filePath.find_last_of("/");
    size_t posBack = filePath.find_last_of("\\");
    size_t pos = std::string::npos;
    if (posSlash != std::string::npos && posBack != std::string::npos) {
        pos = (posSlash > posBack) ? posSlash : posBack;
    } else if (posSlash != std::string::npos) {
        pos = posSlash;
    } else if (posBack != std::string::npos) {
        pos = posBack;
    }

    if (pos != std::string::npos) {
        directory = filePath.substr(0, pos);
        filename = filePath.substr(pos + 1);
    }

    // モデルの生成とファイル読み込み、初期化
    std::unique_ptr<Model> model = std::make_unique<Model>();
    model->Initialize(modelCommon, directory, filename);
    // モデルをmapコンテナに格納する（キーは元の filePath を使用）
    models.insert(std::make_pair(filePath, std::move(model)));
}

Model* ModelManager::FindModel(const std::string& filePath)
{
    // 読み込み済みモデルを検索
    if (models.contains(filePath)) {
        // 読み込みモデルを戻り値としてreturn
        return models.at(filePath).get();
    }
    // ファイル名一致なし
    return nullptr;
}

Model* ModelManager::LoadAndGetModel(const std::string& filePath)
{
    LoadModel(filePath);
    return FindModel(filePath);
}

