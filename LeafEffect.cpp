#include "LeafEffect.h"
#include "ModelManager.h"
#include "Object3dCom.h"
#include <cmath>
#include <cstdlib>

LeafEffect::~LeafEffect() {
  for (auto &leaf : leaves_) {
    delete leaf.leaf;
    leaf.leaf = nullptr;
  }
  leaves_.clear();
}

void LeafEffect::Initialize(Object3dCom *object3dCom) {
  object3dCom_ = object3dCom;

  ModelManager *modelManager = ModelManager::GetInstance();
  modelManager->LoadModel("darkGreenLeaf.obj");
  modelManager->LoadModel("lightGreenLeaf.obj");
  modelDark_ = modelManager->FindModel("darkGreenLeaf.obj");
  modelLight_ = modelManager->FindModel("lightGreenLeaf.obj");

  for (int i = 0; i < 2; ++i) {
    SpawnLeaf();
  }
  spawnCooldown_ = Frand(spawnMin_, spawnMax_);
}

void LeafEffect::Update() {
  spawnCooldown_ -= 0.016f;
  if (spawnCooldown_ <= 0.0f) {
    if (rand() % 10 < 7) {
      SpawnLeaf();
    }
    spawnCooldown_ = Frand(spawnMin_, spawnMax_);
  }

  for (auto &leaf : leaves_) {
    if (!leaf.leaf) {
      continue;
    }

    if (!leaf.active) {
      leaf.wait -= 0.016f;
      if (leaf.wait <= 0.0f) {
        Vector3 p{Frand(40.0f, 45.0f), RandSpawnY(), Frand(4.0f, 9.0f)};
        leaf.leaf->SetTranslate(p);
        leaf.baseY = p.y;
        leaf.vx = Frand(0.03f, 0.09f);
        leaf.amp = Frand(1.2f, 2.6f);
        leaf.hz = Frand(0.18f, 0.55f);
        leaf.t = Frand(0.0f, 1000.0f);
        leaf.ry = Frand(0.004f, 0.012f);
        leaf.rz = Frand(-0.008f, 0.008f);
        leaf.active = true;
      }
      continue;
    }

    auto p = leaf.leaf->GetTranslate();
    float wind = leaf.vx + std::sin(leaf.t * 0.8f) * 0.015f;
    p.x -= wind;

    leaf.t += 0.016f;
    float mainWave = std::sin(leaf.t * 6.28318f * leaf.hz) * leaf.amp;
    float subWave = std::sin(leaf.t * 0.6f + p.x * 0.2f) * (leaf.amp * 0.35f);
    p.y = leaf.baseY + mainWave + subWave;
    p.z += std::sin(leaf.t * 0.9f + p.x * 0.15f) * 0.015f;

    if (p.x < -28.0f) {
      leaf.active = false;
      leaf.wait = Frand(0.4f, 1.6f);
    }

    Vector3 r = leaf.leaf->GetRotate();
    r.y += leaf.ry;
    r.z += leaf.rz;

    leaf.leaf->SetTranslate(p);
    leaf.leaf->SetRotate(r);
    leaf.leaf->Update();
  }
}

void LeafEffect::Draw() {
  for (auto &leaf : leaves_) {
    if (leaf.leaf && leaf.active) {
      leaf.leaf->Draw();
    }
  }
}

void LeafEffect::SpawnLeaf() {
  if ((int)leaves_.size() >= maxLeaves_) {
    return;
  }

  Model *leafModel = (rand() % 2 == 0) ? modelDark_ : modelLight_;
  if (!leafModel) {
    return;
  }

  Vector3 pos{Frand(40.0f, 45.0f), RandSpawnY(), Frand(4.0f, 9.0f)};
  Vector3 sca{Frand(0.34f, 0.44f), Frand(0.34f, 0.44f), Frand(0.34f, 0.44f)};

  Object3d *o = new Object3d();
  o->Initialize(object3dCom_);
  o->SetModel(leafModel);
  o->SetTranslate(pos);
  o->SetScale(sca);

  Leaf leaf;
  leaf.leaf = o;
  leaf.active = true;
  leaf.vx = Frand(0.03f, 0.09f);
  leaf.baseY = pos.y;
  leaf.t = Frand(0.0f, 1000.0f);
  leaf.amp = Frand(1.2f, 2.6f);
  leaf.hz = Frand(0.18f, 0.55f);
  leaf.ry = Frand(0.004f, 0.012f);
  leaf.rz = Frand(-0.008f, 0.008f);
  leaf.wait = 0.0f;

  leaves_.push_back(leaf);
}

float LeafEffect::Frand(float a, float b) {
  return a + (b - a) * (float)rand() / (float)RAND_MAX;
}

float LeafEffect::RandSpawnY() {
  float r = Frand(0.0f, 1.0f);
  if (r < 0.20f) {
    return Frand(-1.0f, 4.0f); // ★ 旧: (-4.0f, 1.0f) → 上へ+2
  }
  else if (r < 0.80f) {
    return Frand(4.0f, 15.0f); // ★ 旧: (1.0f, 12.0f) → 上へ+2
  }
  else {
    return Frand(15.0f, 19.0f); // ★ 旧: (12.0f, 16.0f) → 上へ+2
  }
}
