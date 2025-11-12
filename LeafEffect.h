#pragma once
#include "Object3d.h"
#include "Vector.h"
#include <string>
#include <vector>

class Object3dCom;
class Model;

class LeafEffect {
public:
  LeafEffect() = default;
  ~LeafEffect();

  void Initialize(Object3dCom *object3dCom);
  void Update();
  void Draw();

private:
  struct Leaf {
    Object3d *leaf = nullptr;
    bool active = false;
    float vx = 0.0f;
    float baseY = 0.0f;
    float t = 0.0f;
    float amp = 0.0f;
    float hz = 0.0f;
    float ry = 0.0f;
    float rz = 0.0f;
    float wait = 0.0f;
  };

  Object3dCom *object3dCom_ = nullptr;
  Model *modelDark_ = nullptr;
  Model *modelLight_ = nullptr;
  std::vector<Leaf> leaves_;

  float spawnCooldown_ = 0.0f;
  const float spawnMin_ = 0.9f;
  const float spawnMax_ = 2.2f;
  const int maxLeaves_ = 12;

private:
  void SpawnLeaf();
  static float Frand(float a, float b);
  static float RandSpawnY();
};
