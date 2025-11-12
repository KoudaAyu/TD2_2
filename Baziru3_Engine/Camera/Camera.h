#pragma once
#include "Matrix4x4.h"
#include "Transform.h"
class Camera
{
private:
	Transform transform_;
	Matrix4x4 worldMatrix_;
	Matrix4x4 viewMatrix_;
	Matrix4x4 projectionMatrix_;
	Matrix4x4 viewProjectionMatrix_;

	//float fovY = 0.45f; // 資料通り
    //float aspectRatio = static_cast<float>(winApp->GetClientWidth()) /
    //                    static_cast<float>(winApp->GetClientHeight());
    //float nearZ = 0.1f;
    //float farZ = 100.0f;

	//水平方向視野角
	float fovY_ = 0.45f;
	//アスペクト比
	float aspectRatio_ = 1.0f;
	//ニアクリップ距離
	float nearZ_ = 0.1f;
	//ファークリップ距離
	float farZ_ = 100.0f;

	// 回転
	Vector3 rotation_ = { 0.0f, 0.0f, 0.0f };
	// 移動
	Vector3 translation_ = { 0.0f, 0.0f, 0.0f };

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	Camera();

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// worldMatrixのgetter
	/// </summary>
	/// <returns></returns>
	const Matrix4x4& GetWorldMatrix() const { return worldMatrix_; }
	/// <summary>
	/// viewMatrixのgetter
	/// </summary>
	/// <returns></returns>
	const Matrix4x4& GetViewMatrix() const { return viewMatrix_; }
	/// <summary>
	/// projectionMatrixのgetter
	/// </summary>
	/// <returns></returns>
	const Matrix4x4& GetProjectionMatrix() const { return projectionMatrix_; }
	/// <summary>
	/// viewProjectionMatrixのgetter
	/// </summary>
	/// <returns></returns>
	const Matrix4x4& GetViewProjectionMatrix() const { return viewProjectionMatrix_; }
	/// <summary>
	/// 回転のgetter
	/// </summary>
	/// <returns></returns>
	const Vector3& GetRotate() const { return rotation_; }
	/// <summary>
	/// 移動のgetter
	/// </summary>
	/// <returns></returns>
	const Vector3& GetTranslate() const { return translation_; }

	/// <summary>
	/// 回転のsetter
	/// </summary>
	/// <param name="rotation"></param>
	void SetRotate(const Vector3& rotation) { rotation_ = rotation; }
	/// <summary>
	/// 移動のsetter
	/// </summary>
	/// <param name="translation"></param>
	void SetTranslate(const Vector3& translation) { translation_ = translation; }
	/// <summary>
	/// 水平方向視野角のsetter
	/// </summary>
	/// <param name="fovY"></param>
	void SetFovY(const float fovY) { fovY_ = fovY; }
	/// <summary>
	/// アスペクト比のsetter
	/// </summary>
	/// <param name="aspectRatio"></param>
	void SetAspectRatio(const float aspectRatio) { aspectRatio_ = aspectRatio; }
	/// <summary>
	/// ニアクリップ距離のsetter
	/// </summary>
	/// <param name="nearZ"></param>
	void SetNearClip(const float nearZ) { nearZ_ = nearZ; }
	/// <summary>
	/// ファークリップ距離のsetter
	/// </summary>
	/// <param name="farZ"></param>
	void SetFarClip(const float farZ) { farZ_ = farZ; }
};
