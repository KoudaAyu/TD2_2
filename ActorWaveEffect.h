#pragma once
#define NOMINMAX
#include <Windows.h>
#include <vector>
#include <functional>
#include <cmath>
#include "Vector.h"
class MapChipField;
class Object3d;

class ActorWaveEffect {
public:
    void Initialize(MapChipField* map) { map_ = map; }

    // 任意のObject3dを登録。onApplyでロジック同期（SetPosition/UpdateAABB等）を行う
    void Add(Object3d* obj, std::function<void(const Vector3&)> onApply);

    // TileWaveEffect と同時に始めるだけで同期します（同じリング式・同じタイミング）
    void Start();

    // 更新（TileWaveEffect と同じ tick タイミングで呼んでください）
    void Update(float dt);


    void AddCustom(
        const Vector3& basePos,
        const Vector3& baseScale,                         // ★追加
        std::function<void(const Vector3&)> setTranslate, // 見た目の位置変更
        std::function<void(const Vector3&)> setScale,     // 見た目のスケール変更
        std::function<void(const Vector3&)> onApply       // ロジック同期
    );


    bool IsFinished() const { return !active_; }

    // 見た目パラメータ（TileWaveEffectに合わせて調整可能）
    void SetPerCellDelay(float v) { perCellDelay_ = v; }
    void SetPopDuration(float v) { popDuration_ = v; }
    void SetWaveAmpRatio(float v) { waveAmpRatio_ = v; }
    void SetWaveFreq(float hz) { waveFreq_ = hz; }
    void SetWaveDamp(float v) { waveDamp_ = v; }

private:
    struct Actor {
        Object3d* obj = nullptr;

        std::function<void(const Vector3&)> setTranslate;
        std::function<void(const Vector3&)> setScale;

        std::function<void(const Vector3&)> onApply;

        Vector3 basePos{};
        Vector3 pos{};
        Vector3 baseScale{};   // ★追加：元のスケールを保持する

        bool    visible = false;
        float   anim = 0.0f;
        float   tVisible = 0.0f;
        int     ring = -1;
    };


    static float EaseOutBack(float t, float k = 1.70158f) {
        t = (t < 0.f) ? 0.f : (t > 1.f ? 1.f : t);
        float u = t - 1.0f;
        return 1.0f + (u * u * ((k + 1.0f) * u + k));
    }

    // 位置からグリッド(x,y)を見つける（マップ上にぴったり置いている想定）
    bool FindGridIndexByPosition(const Vector3& p, int& gx, int& gy) const;

    // TileWaveEffect と同じリング式：右上(0)→左下(最大)の斜め走査
    int ComputeRing(int gx, int gy) const {
        const int H = (int)numV_;
        const int W = (int)numH_;
        (void)H;
        return (W - 1 - gx) + gy;
    }

private:
    MapChipField* map_ = nullptr;
    std::vector<Actor> actors_;

    // マップ寸法キャッシュ
    uint32_t numH_ = 0, numV_ = 0;

    // 進行管理
    bool  active_ = false;
    int   currentRing_ = -1;
    int   maxRing_ = 0;
    float time_ = 0.0f;

    // 見た目パラメータ（TileWaveEffectに合わせる）
    float perCellDelay_ = 0.06f;  // 同期させるなら TileWaveEffect と同値に
    float popDuration_ = 0.28f;
    float waveAmpRatio_ = 0.18f;  // タイル高さに対する振幅比
    float waveFreq_ = 2.2f;   // Hz
    float waveDamp_ = 1.9f;   // 減衰
};
