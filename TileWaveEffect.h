#pragma once
#define NOMINMAX
#include <Windows.h>
#include <vector>
#include <cmath>
#include "Vector.h"
#include "MapChipField.h"
#include "Object3d.h"
#include "Player.h"
#include "ParticleManager.h"

class TileWaveEffect {
public:
    // 初期化（マップ・プレイヤー・壁リスト参照を渡す）
    void Initialize(MapChipField* map, Player* player, std::vector<std::vector<Object3d*>>* walls);

    // 更新（毎フレーム呼ぶ）
    void Update(float dt);

    // 描画（visibleな壁だけ描画）
    void Draw();

    // 即完了（スキップ機能）
    void Skip();

    // 終了確認
    bool IsFinished() const { return !active_; }

private:
    // 補助構造体
    struct Cell {
        bool  visible = false;
        bool  particleDone = false;
        float anim = 0.0f;     // 0→1 (スケール用)
        float tVisible = 0.0f; // ★ 追加：見え始めてからの経過秒（揺れ用）
        Vector3 basePos{};
        Vector3 baseScale{};   // ★ 追加：元のスケールを保持
    };

    // 内部処理
    void BuildRings();
    void ApplyRing(int ring);
    static float EaseOutBack(float t, float k = 1.70158f) {
        float u = t - 1.0f;
        return 1.0f + (u * u * ((k + 1.0f) * u + k));
    }

private:
    // 参照
    MapChipField* map_ = nullptr;
    Player* player_ = nullptr;
    std::vector<std::vector<Object3d*>>* walls_ = nullptr;

    // 演出管理
    std::vector<std::vector<int>> ringIndex_; // 距離リング
    std::vector<std::vector<Cell>> cells_;    // 各タイル状態
    int maxRing_ = 0;
    int currentRing_ = -1;
    float waveTime_ = 0.0f;

    // パラメータ
    float perCellDelay_ = 0.035f;  // 1セルあたり遅延
    float popDuration_ = 0.15f;    // ニョキっと生える時間
    bool active_ = false;

    // パラメータをメンバに
    float waveAmpRatio_ = 0.18f; // タイル高さに対する振幅比
    float waveFreq_ = 2.2f;  // [Hz]（1秒あたりの揺れ回数）
    float waveDamp_ = 1.9f;  // 減衰（大きいほど早く収束）
};
