#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <string>
#include <vector>
#include "Object3dCom.h"
#include "Object3d.h"
#include "ModelCommon.h"

struct VertexData {
	Vector4 position;
	Vector2 texcoord;
	Vector3 normal;
};

struct Material {
	Vector4 color;
	int32_t enableLighting;
	float padding[3];
	Matrix4x4 uvTransform;
};

struct MaterialData {
	std::string textureFilePath;
	uint32_t textureIndex = 0;
};

struct ModelData {
	std::vector<VertexData> vertices;
	MaterialData material;
};
class Model {
public:

	Model();
	Model(const Model& other);
	~Model();

	void Initialize(ModelCommon* modelCommon, const std::string& directorypath, const std::string& filename);
	void Draw(ID3D12GraphicsCommandList* commandList);


	/// <summary>
	/// objファイルの読み取り
	/// </summary>
	/// <param name="directoryPath"></param>
	/// <param name="filePath"></param>
	/// <returns></returns>
	static ModelData LoadObjFile(const std::string& directoryPath,
		const std::string& filename);

	static MaterialData LoadMaterialTemplateFile(const std::string& directoryPath,
		const std::string& filePath);

	void SetColor(Vector4 color);
	void SetTexture(const std::string& filePath);



private:
	ModelCommon* modelCommon_ = nullptr; // (= ModelCommon 役)

	// モデル固有データ（Object3D から引っ越し）
	ModelData modelData_;
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
	VertexData* vertexData_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
	Material* materialData_ = nullptr;

	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU_{};
};
