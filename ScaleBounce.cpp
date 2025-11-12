#include "ScaleBounce.h"
#include <algorithm>
#include <cmath>

void ScaleBounce::Play(Object3d* obj, float durationSec, float minScale, float overshoot)
{
    if (!obj) return;

    Play(
        // setter
        [obj](const Vector3& s) { obj->SetScale(s); },
        // getter
        [obj]() -> Vector3 { return obj->GetScale(); },
        durationSec, minScale, overshoot
    );
}

void ScaleBounce::Play(std::function<void(const Vector3&)> setter,
    std::function<Vector3(void)> getter,
    float durationSec, float minScale, float overshoot)
{
    if (!setter || !getter) return;

    Task t{};
    t.setScale = std::move(setter);
    t.getScale = std::move(getter);
    t.base = t.getScale();                 // 現在値を終点（基準）に
    t.elapsed = 0.0f;
    t.duration = std::max(0.001f, durationSec);
    t.minScale = std::clamp(minScale, 0.0f, 1.0f);
    t.overshoot = std::max(0.0f, overshoot);
    t.alive = true;

    // 開始時は minScale 倍にしてから開始
    Vector3 start = { t.base.x * t.minScale, t.base.y * t.minScale, t.base.z * t.minScale };
    t.setScale(start);

    tasks_.push_back(std::move(t));
}

void ScaleBounce::Update(float dt)
{
    if (tasks_.empty()) return;

    for (auto& t : tasks_) {
        if (!t.alive) continue;

        t.elapsed += dt;
        float u = std::clamp(t.elapsed / t.duration, 0.0f, 1.0f);

        // 0→1（オーバーシュートあり）
        float k = EaseOutBack(u, t.overshoot);
        float s = t.minScale + (1.0f - t.minScale) * k;

        Vector3 cur = { t.base.x * s, t.base.y * s, t.base.z * s };
        t.setScale(cur);

        if (u >= 1.0f) {
            t.setScale(t.base); // 最終的にぴったり戻す
            t.alive = false;
        }
    }

    tasks_.erase(std::remove_if(tasks_.begin(), tasks_.end(),
        [](const Task& t) { return !t.alive; }), tasks_.end());
}

float ScaleBounce::EaseOutBack(float t, float overshoot)
{
    float s = 1.70158f * (1.0f + overshoot * 1.5f);
    float u = (t - 1.0f);
    return (u * u * ((s + 1.0f) * u + s) + 1.0f);
}
