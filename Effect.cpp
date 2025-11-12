#include "Effect.h"
#include "Effect.h"
#include "ParticleManager.h"
#include <cmath>

std::vector<Effect::Instance>& Effect::Instances() {
    static std::vector<Instance> s;
    return s;
}

void Effect::PlayApplePickup(ParticleManager* pm, const Vector3& worldPos, Vector3* scalePtr,
    float tGrow, float tShrink, float peak)
{
    Instance inst{};
    inst.type = Type::ApplePickup;
    inst.pm = pm;
    inst.pos = worldPos;
    inst.scalePtr = scalePtr;
    inst.tGrow = tGrow;
    inst.tShrink = tShrink;
    inst.peak = peak;
    if (inst.scalePtr) inst.baseScale = *inst.scalePtr;

    auto& list = Instances();
    list.emplace_back(inst);
    list.back().Start(); // 生成直後の一回処理（粒バーストなど）
}

void Effect::Tick(float dt) {
    auto& list = Instances();
    for (auto& e : list) e.Update(dt);
    list.erase(std::remove_if(list.begin(), list.end(),
        [](const Instance& e) { return e.finished; }),
        list.end());
}

void Effect::Instance::Start() {
    if (type == Type::ApplePickup && pm) {
        // ▼ここをあなたのPMに合わせて置き換え
        // 例1: 8方向XY即時バーストAPIがあるならそれを使う
        // pm->Emit8DirectionsXY("appleSpark", pos, 8);

        // 例2: 単純な即時Emitしか無いなら、とりあえず複数回
        const int N = 16; // 粒の数はお好みで
        for (int i = 0; i < N; ++i) {
            //pm->Emit("default", pos, 1);


        }
    }
}

void Effect::Instance::Update(float dt) {
    elapsed += dt;

    if (type == Type::ApplePickup) {
        float s = 1.0f;
        if (elapsed < tGrow) {
            float u = elapsed / tGrow;
            s = Lerp(1.0f, peak, Effect::EaseOutQuad(u));            // ふくらむ
        } else if (elapsed < tGrow + tShrink) {
            float u = (elapsed - tGrow) / tShrink;
            float under = 0.98f;                                     // 少し潜って戻る
            float mid = Lerp(peak, under, Effect::EaseInOutQuad(std::min(u * 1.2f, 1.0f)));
            float back = Lerp(mid, 1.0f, std::min(std::max(u * 1.1f - 0.1f, 0.0f), 1.0f));
            s = back;
        } else {
            s = 1.0f;
            finished = true;
        }

        if (scalePtr) {
            scalePtr->x = baseScale.x * s;
            scalePtr->y = baseScale.y * s;
            scalePtr->z = baseScale.z * s;
            if (finished) *scalePtr = baseScale; // 念のため原状復帰
        }
    }
}
