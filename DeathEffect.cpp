#include "DeathEffect.h"
#include "ParticleManager.h"

void DeathEffect::Start(MapChipField* map, const Player* player)
{
    cells_.clear();
    index_ = 0;
    timer_ = 0.0f;
    burstFired_ = false;
    alive_ = 0.0f;
    finished_ = false;
    active_ = true;

    if (!map || !player) { active_ = false; finished_ = true; return; }

    const int H = static_cast<int>(map->GetNumBlockVirtical());

    // ★壁を含まない「ヘビ本体（頭→尾）」を使う関数がある前提
    //   もし未実装なら、GetOccupiedGridPositions() を使って頭→尾になるように並べてください。
    std::vector<std::pair<int, int>> grids;
    if constexpr (true) {
        
    }
    if (grids.empty()) {
        // フォールバック：既存の関数を使う（順序は必要に応じて反転してね）
        grids = player->GetOccupiedGridPositions();
        // ここで不要なら「壁セル」を除外するロジックを加えるとより堅牢
    }
    if (grids.empty()) { active_ = false; finished_ = true; return; }

    auto pushCell = [&](int gx, int gyPlayer) {
        const int gyMap = H - 1 - gyPlayer; // ★Y反転：プレイヤー座標(下原点)→マップ(上原点)
        cells_.push_back(map->GetMapChipPositionByIndex(gx, gyMap));
        };

    if (order_ == Order::HeadToTail) {
        for (auto& g : grids) pushCell(g.first, g.second);          // 頭→尾
    } else {
        for (auto it = grids.rbegin(); it != grids.rend(); ++it)     // 尾→頭
            pushCell(it->first, it->second);
    }
}

void DeathEffect::Update(float dt)
{
    if (!active_ || finished_) return;

    if (mode_ == Mode::BurstAll) {
        if (!burstFired_) {
            // ★全セルで一斉に発生
            for (const auto& p : cells_) {
                ParticleManager::GetInstance()->EmitBurst8(
                    "player", p,
                    /*scale*/ 0.08f,
                    /*speedMin*/ 0.28f,
                    /*speedMax*/ 0.40f
                );
            }
            burstFired_ = true;
        }
        alive_ += dt;
        if (alive_ >= linger_) { finished_ = true; active_ = false; }
        return;
    }

    // （おまけ）逐次破壊：未使用ならそのままでOK
    timer_ += dt;
    while (timer_ >= interval_ && index_ < static_cast<int>(cells_.size())) {
        timer_ -= interval_;
        ParticleManager::GetInstance()->EmitBurst8(
            "player", cells_[index_],
            0.08f, 0.25f, 0.35f
        );
        ++index_;
    }
    if (index_ >= static_cast<int>(cells_.size())) {
        finished_ = true; active_ = false;
    }
}
