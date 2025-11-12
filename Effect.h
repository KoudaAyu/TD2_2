#pragma once
#include <vector>
#include <algorithm>
#include "Vector.h"

class ParticleManager;

// 単一クラスに「インスタンス管理」「再生API」「更新」を内包
class Effect {
public:
    // 1) 再生API（リンゴ取得の演出：粒バースト＋スケールぷにょ）
    //    scalePtr: 演出対象のScaleへのポインタ（例：&player.worldTransform_.scale_）
    static void PlayApplePickup(ParticleManager* pm, const Vector3& worldPos, Vector3* scalePtr,
        float tGrow = 0.08f, float tShrink = 0.12f, float peak = 1.18f);

    // 2) 毎フレーム全エフェクト更新（どこか1か所で1行だけ呼べばOK）
    static void Tick(float dt);

private:
    enum class Type { ApplePickup };

    struct Instance {
        Type type{};
        ParticleManager* pm = nullptr;
        Vector3 pos{};
        Vector3* scalePtr = nullptr; // 触るのはScaleのみ
        Vector3 baseScale{ 1,1,1 };

        // 時間パラメータ
        float tGrow = 0.08f;
        float tShrink = 0.12f;
        float peak = 1.18f;

        float elapsed = 0.0f;
        bool  finished = false;

        void Start();           // 生成直後の一度きり処理（パーティクル等）
        void Update(float dt);  // 種別ごとの本体更新
    };

    static std::vector<Instance>& Instances();

    // ちょいイージング
    static float Lerp(float a, float b, float t) { return a + (b - a) * t; }
    static float EaseOutQuad(float t) { return 1.0f - (1.0f - t) * (1.0f - t); }
    static float EaseInOutQuad(float t) {
        return (t < 0.5f) ? 2.0f * t * t : 1.0f - ((-2.0f * t + 2.0f) * (-2.0f * t + 2.0f)) / 2.0f;
    }
};
