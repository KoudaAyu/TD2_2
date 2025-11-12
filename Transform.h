#pragma once
#include "Vector.h"

struct Transform {
  Vector3 scale_;
  Vector3 rotation_;
  Vector3 translation_;
  Matrix4x4 matWorld_{};

  inline void TransferMatrix()
  {
      matWorld_ = MakeAffineMatrix(scale_, rotation_, translation_);
  }
};
