#include "ActorWaveEffect.h"
#include "MapChipField.h"
#include "Object3d.h"

void ActorWaveEffect::Add(Object3d* obj, std::function<void(const Vector3&)> onApply)
{
    if (!obj) return;
    Actor a{};
    a.obj = obj;
    a.basePos = obj->GetTranslate();
    a.pos = a.basePos;
    a.baseScale = obj->GetScale();            // ★追加：元スケールを取得
    a.onApply = std::move(onApply);
    actors_.push_back(std::move(a));
}


bool ActorWaveEffect::FindGridIndexByPosition(const Vector3& p, int& gx, int& gy) const
{
    // マップに“ぴったり”置いている（= GetMapChipPositionByIndex と一致）前提で探索
    for (uint32_t y = 0; y < numV_; ++y) {
        for (uint32_t x = 0; x < numH_; ++x) {
            Vector3 c = map_->GetMapChipPositionByIndex(x, y);
            if (c.x == p.x && c.y == p.y && c.z == p.z) {
                gx = (int)x; gy = (int)y;
                return true;
            }
        }
    }
    return false;
}

void ActorWaveEffect::Start()
{
    if (!map_ || actors_.empty()) return;

    numH_ = map_->GetNumBlockHorizontal();
    numV_ = map_->GetNumBlockVirtical();

    maxRing_ = 0;
    for (auto& a : actors_) {
        int gx = -1, gy = -1;
        if (!FindGridIndexByPosition(a.basePos, gx, gy)) {
            a.ring = (int)(numH_ + numV_);
        } else {
            a.ring = ComputeRing(gx, gy);
            if (a.ring > maxRing_) maxRing_ = a.ring;
        }

        // 初期は非表示（元スケール×極小）
        a.visible = false;
        a.anim = 0.0f;
        a.tVisible = 0.0f;
        a.pos = a.basePos;

        const Vector3 tiny{ 0.001f, 0.001f, 0.001f };
        const Vector3 s0{
            a.baseScale.x * tiny.x,
            a.baseScale.y * tiny.y,
            a.baseScale.z * tiny.z
        };

        if (a.obj) {
            a.obj->SetScale(s0);
            a.obj->SetTranslate(a.pos);
        } else {
            if (a.setScale)     a.setScale(s0);
            if (a.setTranslate) a.setTranslate(a.pos);
        }

        // ロジックも初期位置に同期したい場合は残す（不要なら消してOK）
        if (a.onApply) a.onApply(a.pos);
    }

    time_ = 0.0f;
    currentRing_ = -1;
    active_ = true;
}


void ActorWaveEffect::Update(float dt)
{
    if (!active_) return;

    time_ += dt;
    int targetRing = (int)std::floor(time_ / perCellDelay_);
    if (targetRing > maxRing_) targetRing = maxRing_;

    // 新たに可視化するリング
    for (int r = currentRing_; r <= targetRing; ++r) {
        if (r < 0) continue;
        for (auto& a : actors_) {
            if (a.ring != r) continue;
            if (!a.visible) {
                a.visible = true;
                a.anim = 0.0f;
                a.tVisible = 0.0f;
                // にょきっ開始（元スケール×{1, tiny, 1}）
                const Vector3 startMul{ 1.0f, 0.001f, 1.0f };
                const Vector3 sStart{
                    a.baseScale.x * startMul.x,
                    a.baseScale.y * startMul.y,
                    a.baseScale.z * startMul.z
                };
                if (a.obj) a.obj->SetScale(sStart);
                else if (a.setScale) a.setScale(sStart);
            }
        }
    }
    currentRing_ = targetRing;

    // アニメ進行（にょきっ＋減衰バウンド）
    bool animLeft = false;
    const float h = map_->GetBlockHeight();
    const float amp = h * waveAmpRatio_;
    const float w = 2.0f * 3.1415926535f * waveFreq_;

    for (auto& a : actors_) {
        if (!a.visible) continue;

        // スケール係数（0→1）
        a.anim = std::min(1.0f, a.anim + dt / popDuration_);
        float s = std::max(0.001f, EaseOutBack(a.anim));
        Vector3 scaled = { a.baseScale.x * s, a.baseScale.y * s, a.baseScale.z * s };

        // Yボビング（可視化からの経過時間で計算）
        a.tVisible += dt;
        float bob = std::sin(w * a.tVisible) * amp * std::exp(-waveDamp_ * a.tVisible);

        // 位置適用
        a.pos = a.basePos;
        a.pos.y += bob;

        // 適用（Object3d or アダプタ）
        if (a.obj) {
            a.obj->SetScale(scaled);
            a.obj->SetTranslate(a.pos);
        } else {
            if (a.setScale)     a.setScale(scaled);
            if (a.setTranslate) a.setTranslate(a.pos);
        }
        if (a.onApply) a.onApply(a.pos);

        // まだアニメ中かどうか
        if (a.anim < 1.0f || std::fabs(bob) > 0.002f) {
            animLeft = true;
        }
    }

    // 全リング出し & 全アニメ完了で終了
    if (currentRing_ >= maxRing_ && !animLeft) {
        for (auto& a : actors_) {
            // ★ 最終的に「元スケール」に戻す（←ここが重要）
            if (a.obj) {
                a.obj->SetScale(a.baseScale);
                a.obj->SetTranslate(a.basePos);
            } else {
                if (a.setScale)     a.setScale(a.baseScale);
                if (a.setTranslate) a.setTranslate(a.basePos);
            }
            if (a.onApply) a.onApply(a.basePos);
        }
        active_ = false;
    }
}

void ActorWaveEffect::AddCustom(
    const Vector3& basePos,
    const Vector3& baseScale,
    std::function<void(const Vector3&)> setTranslate,
    std::function<void(const Vector3&)> setScale,
    std::function<void(const Vector3&)> onApply)
{
    Actor a{};
    a.obj = nullptr;
    a.basePos = basePos;
    a.pos = basePos;
    a.baseScale = baseScale;                 // ★記録
    a.setTranslate = std::move(setTranslate);
    a.setScale = std::move(setScale);
    a.onApply = std::move(onApply);
    actors_.push_back(std::move(a));
}
