#pragma once
#define NOMINMAX
#include <Windows.h>
#include <vector>
#include <functional>
#include "Vector.h"
#include "Object3d.h"

class ScaleBounce {
public:
    // Object3d 版：obj->GetScale()/SetScale() を内部で呼ぶのでポインタ不要
    void Play(Object3d* obj,
        float durationSec = 0.30f,
        float minScale = 0.0f,
        float overshoot = 0.18f);

    // （汎用版：必要になったとき用）
    void Play(std::function<void(const Vector3&)> setter,
        std::function<Vector3(void)> getter,
        float durationSec = 0.30f,
        float minScale = 0.0f,
        float overshoot = 0.18f);

    void Update(float dt);

private:
    struct Task {
        std::function<void(const Vector3&)> setScale;
        std::function<Vector3(void)>        getScale;
        Vector3 base;   // 終点（元のスケール）
        float elapsed;
        float duration;
        float minScale;    // 0.0~1.0
        float overshoot;   // 強さ
        bool  alive;
    };

    static float EaseOutBack(float t, float overshoot);
    std::vector<Task> tasks_;
};
