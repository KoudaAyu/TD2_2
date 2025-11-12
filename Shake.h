#pragma once
#include "Vector.h"

class Shake {
private:
  Vector3 position_ = {0.0f, 0.0f, 0.0f};

  bool isShaking_ = false;

  float shakeTimer_ = 0;

  float min_ = 0.0f;
  float max_ = 0.0f;
  float shakeTime_ = 0.0f;

public:
  /// <summary>
  /// 初期化
  /// </summary>
  void Initialize();

  void Start(const float &min, const float &max, const float& shakeTime);

  /// <summary>
  /// 更新
  /// </summary>
  void Update();
  /// <summary>
  /// getter
  /// </summary>
  /// <returns></returns>
  bool IsShaking() { return isShaking_; }

  Vector3 GetPosition() { return position_; }
};
