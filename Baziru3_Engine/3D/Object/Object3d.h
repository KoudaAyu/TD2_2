#pragma once
#include <cstdint>
#include <d3d12.h>
#include <string>
#include <vector>
#include <wrl.h>

#include "Matrix4x4.h"
#include "Transform.h"
#include "Vector.h"
#include "Model.h"
class Model;
class Object3dCom;
class Camera;

class Object3d
{
public:

	struct DirectionalLight
	{
		Vector4 color;
		Vector3 direction;
		float intensity;
	};

private:
	Object3dCom* object3dCom_ = nullptr;
	Model* model_ = nullptr;

	Transform transform_;

	Camera* camera_ = nullptr;

	Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

	/*Transform cameraTransform_;

	float fovY_ = 0.45f;
	float aspectRatio_ = 1.0f;
	float nearZ_ = 0.1f;
	float farZ_ = 100.0f;*/

	/// ==================================
	/// マテリアルリソース
	/// ==================================

	// バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource_ = nullptr;
	// バッファリソース内のデータを指すポインタ
	DirectionalLight* directionalLight_ = nullptr;

	/// ===============================
	/// 座標変換行列リソース
	/// ===============================

	// バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource_ = nullptr;
	// バッファリソース内のデータを指すポインタ
	TransformationMatrix* transformationMatrixData_ = nullptr;

public:

	Object3d() = default;
	~Object3d();

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(Object3dCom* object3dCom);
	/// <summary>
	/// 更新
	/// </summary>
	void Update();
	/// <summary>
	/// 描画
	/// </summary>
	void Draw();
	// モデルの設定
	void SetModel(const std::string& filePath);
	// セッター
	void SetModel(Model* model) { model_ = model; }

	void SetCamera(Camera* camera) { camera_ = camera; }

	void SetScale(const Vector3& scale) { transform_.scale_ = scale; }
	void SetRotate(const Vector3& rotate) { transform_.rotation_ = rotate; }
	void SetTranslate(const Vector3& translate) { transform_.translation_ = translate; }    

	void SetTransform(const Transform& transform) { transform_ = transform; }

	/// <summary>
	/// 指定TransformとCameraを一括適用し 必要なら即時にUpdate()で行列をGPUへ反映する
	/// </summary>
	/// <param name="t">Transform</param>
	/// <param name="cam">Camera</param>
	/// <param name="immediateUpdate">true: 即座にUpdate()実行 / false: 次フレームの通常Updateで反映</param>
	void ApplyState(const Transform& t, Camera* cam, bool immediateUpdate = true);


public:
	// --- getter ---
	const Vector3& GetScale() const { return transform_.scale_; }
	const Vector3& GetRotate() const { return transform_.rotation_; }
	const Vector3& GetTranslate() const { return transform_.translation_; }
	Model* GetModel() const { return model_; }
	void SetColor(const Vector4& color);
};
