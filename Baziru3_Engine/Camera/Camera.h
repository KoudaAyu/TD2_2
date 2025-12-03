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

	//水平方向視野角
	float fovY_ = 0.45f;
	//アスペクト比
	float aspectRatio_ = 1.0f;
	//ニアクリップ距離
	float nearZ_ = 0.1f;
	//ファークリップ距離
	float farZ_;

	// 回転
	Vector3 rotation_ = { 0.0f, 0.0f, 0.0f };
	// 移動
	Vector3 translation_ = { 0.0f, 0.0f, 0.0f };

	// Camera shake
	bool isShaking_ = false;
	float shakeAmplitude_ = 0.0f; // 最大揺れ幅
	float shakeDuration_ = 0.0f;  // 揺れ継続時間（秒）
	float shakeTimer_ = 0.0f;     // 残り時間（秒）
	float shakeTimeElapsed_ = 0.0f; // 経過時間（秒）
	Vector3 shakeOffset_ = { 0.0f, 0.0f, 0.0f };

	float shakeStartCooldown_ = 0.0f; 

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	Camera();

	/// <summary>
	/// 初期化処理（ウィンドウサイズ等に依存する行列生成）
	/// </summary>
	void Initialize();

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

	/// <summary>
	/// デバッグカメラなど外部カメラから行列を強制適用
	/// </summary>
	/// <param name="view"></param>
	/// <param name="projection"></param>
	void OverrideViewProjection(const Matrix4x4& view, const Matrix4x4& projection) {
		viewMatrix_ = view;
		projectionMatrix_ = projection;
		viewProjectionMatrix_ = Multiply(viewMatrix_, projectionMatrix_);
	}


	
	/// <summary>
	/// 揺れを開始する
	/// </summary>
	/// <param name="amplitude">最大揺れ幅(ワールド単位)</param>
	/// <param name="duration">duration: 継続時間(秒)</param>
	void StartShake(float amplitude, float duration);
	/// <summary>
	/// デフォルトの揺れ幅を設定
	/// </summary>
	void SetShakeAmplitude(float amplitude) { shakeAmplitude_ = amplitude; }
	/// <summary>
	/// デフォルトの揺れ継続時間を設定
	/// </summary>
	void SetShakeDuration(float duration) { shakeDuration_ = duration; }
};
