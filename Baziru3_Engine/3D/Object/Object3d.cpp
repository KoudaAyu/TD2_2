#include "Object3d.h"
#include "Object3dCom.h"
#include"Camera.h"

#include "ModelManager.h"
#include "TextureManager.h"
#include <cassert>
#include <fstream>
#include <sstream>



Object3d::~Object3d()
{
}

/// <summary>
/// 初期化
/// </summary>
void Object3d::Initialize(Object3dCom *object3dCom) {
  object3dCom_ = object3dCom;

  /// ==================================
  /// マテリアルリソース
  /// ==================================

  // 平行光源
  directionalLightResource_ =
      object3dCom_->GetDirectXCom()->CreateBufferResource(
          sizeof(DirectionalLight));

  // MapしてGPUリソースのCPU側の書き込み可能ポインタを取得する
  directionalLightResource_->Map(0, nullptr,
                                 reinterpret_cast<void **>(&directionalLight_));

  // directionalLightDataに値を書き込む
  directionalLight_->color = {1.0f, 1.0f, 1.0f, 1.0f};
  directionalLight_->direction = {0.0f, -1.0f, 0.0f};
  directionalLight_->intensity = 1.0f;

  // 書き込み完了後はUnmapを呼ぶ
  directionalLightResource_->Unmap(0, nullptr);

  /// ==================================
  /// 座標変換行列リソース
  /// ==================================

  // object3d用のTransformationMatrix用のリソースを作る
  transformationMatrixResource_ =
      object3dCom_->GetDirectXCom()->CreateBufferResource(
          sizeof(TransformationMatrix));
  // 書き込むためのアドレス取得
  transformationMatrixResource_->Map(
      0, nullptr, reinterpret_cast<void **>(&transformationMatrixData_));
  // 単位行列を書き込んでおく
  transformationMatrixData_->WVP = MakeIdentity4x4();
  transformationMatrixData_->World = MakeIdentity4x4();

  // Tranform変数
  transform_ = {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};

  camera_ = object3dCom_->GetDefaultCamera();

  /*cameraTransform_ = {
          {1.0f, 1.0f, 1.0f}, {0.3f, 0.0f, 0.0f}, {0.0f, 4.0f, -10.0f} };

  aspectRatio_ =
          static_cast<float>(object3dCom_->GetDirectXCom()->GetClientWidth()) /
          static_cast<float>(object3dCom_->GetDirectXCom()->GetClientHeight());*/
}
/// <summary>
/// 更新
/// </summary>
void Object3d::Update() {

  transform_.rotation_.y += 0.00f;
  if (transform_.rotation_.y > DirectX::XM_2PI) {
    transform_.rotation_.y -= DirectX::XM_2PI; // 角度を一周分でラップ
  }

  Matrix4x4 worldMatrix = MakeAffineMatrix(transform_.scale_, transform_.rotation_,
                                           transform_.translation_);
  // Matrix4x4 cameraMatrix =
  //	MakeAffineMatrix(cameraTransform_.scale, cameraTransform_.rotate,
  //		cameraTransform_.translate);
  // Matrix4x4 viewMatrix = Inverse(cameraMatrix);
  // Matrix4x4 projectionMatrix =
  //	MakePerspectiveFovMatrix(fovY_, aspectRatio_, nearZ_, farZ_);
  //// WVPMatrixを作る
  // Matrix4x4 worldViewProjectMatrix = Multiply(
  //	worldMatrix, Multiply(viewMatrix,
  //		projectionMatrix));

  Matrix4x4 worldViewProjectionMatrix;

  if (camera_) {
    const Matrix4x4 &viewProjectionMatrix = camera_->GetViewProjectionMatrix();
    worldViewProjectionMatrix =
        Multiply(worldMatrix, viewProjectionMatrix);
  } else {
    worldViewProjectionMatrix = worldMatrix;
  }

  transformationMatrixData_->WVP = worldViewProjectionMatrix;
  transformationMatrixData_->World = worldMatrix;
}
/// <summary>
/// 描画
/// </summary>
void Object3d::Draw() {
  // 3D用のPSO/RootSignatureを毎回再セット（Sprite描画の影響を受けないように）
  object3dCom_->ApplyCommonRenderState();

  // VBだけ（model 用の vertexBufferView を使う）
  object3dCom_->GetDirectXCom()->GetCommandList()->IASetPrimitiveTopology(
      D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  // CBV / SRV / Light
  object3dCom_->GetDirectXCom()
      ->GetCommandList()
      ->SetGraphicsRootConstantBufferView(
          1, transformationMatrixResource_->GetGPUVirtualAddress());
  object3dCom_->GetDirectXCom()
      ->GetCommandList()
      ->SetGraphicsRootConstantBufferView(
          3, directionalLightResource_->GetGPUVirtualAddress());

  // 3Dモデルが割り当てられていれば描画
  if (model_) {
    model_->Draw(object3dCom_->GetDirectXCom()->GetCommandList());
  }
}

void Object3d::SetModel(const std::string &filePath) {
  // モデルを検索してセットする
  model_ = ModelManager::GetInstance()->FindModel(filePath);
}

void Object3d::ApplyState(const Transform& t, Camera* cam, bool immediateUpdate)
{
    transform_ = t;
    camera_ = cam;
    if (immediateUpdate) { Update(); }

}

void Object3d::SetColor(const Vector4& color)
{
    this->color = color;
    if (directionalLight_)
    {
        directionalLight_->color = color;
    }
}

// 静的ファクトリ
Object3d* Object3d::Create(Object3dCom* object3dCom,
    const std::string& modelPath,
    const Transform& transform,
    Camera* camera)
{
    assert(object3dCom);
    Object3d* obj = new Object3d();
    obj->Initialize(object3dCom);

    // モデル取得（未読み込みなら読み込む）
    Model* model = ModelManager::GetInstance()->LoadAndGetModel(modelPath);
    assert(model);
    obj->SetModel(model);

    // Camera 指定なければデフォルト
    if (!camera) { camera = object3dCom->GetDefaultCamera(); }
    obj->ApplyState(transform, camera, true);

    return obj;
}