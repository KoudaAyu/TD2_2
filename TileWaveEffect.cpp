#include "TileWaveEffect.h"

void TileWaveEffect::Initialize(MapChipField* map, Player* player, std::vector<std::vector<Object3d*>>* walls)
{
    map_ = map;
    player_ = player;
    walls_ = walls;
    if (!map_ || !player_ || !walls_) return;

    const int H = static_cast<int>(map_->GetNumBlockVirtical());
    const int W = static_cast<int>(map_->GetNumBlockHorizontal());

    ringIndex_.assign(H, std::vector<int>(W, -1));
    cells_.assign(H, std::vector<Cell>(W));

    BuildRings();

    // 壁初期化：最初は非表示（描かない）。形だけは正常に保つ。
    for (uint32_t y = 0; y < walls_->size(); ++y) {
        for (uint32_t x = 0; x < (*walls_)[y].size(); ++x) {
            if ((*walls_)[y][x]) {
                cells_[y][x].basePos = (*walls_)[y][x]->GetTranslate();
                cells_[y][x].baseScale = (*walls_)[y][x]->GetScale();
                (*walls_)[y][x]->SetScale({ 0.0f, 0.0f, 0.0f });   // 非表示に
            }
        }
    }



    waveTime_ = 0.0f;
    currentRing_ = -1;
    active_ = true;

    perCellDelay_ = 0.06f;   // ← 広がりをゆっくりに
    popDuration_ = 0.28f;   // ← ニョキっと生えるのもゆっくりに

}

void TileWaveEffect::BuildRings() {
    const int H = (int)map_->GetNumBlockVirtical();
    const int W = (int)map_->GetNumBlockHorizontal();

    maxRing_ = 0;
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            // 右上( x=W-1, y=0 )が最小=0、左下( x=0, y=H-1 )が最大
            int idx = (W - 1 - x) + y;
            ringIndex_[y][x] = idx;
            if (idx > maxRing_) maxRing_ = idx;
        }
    }

    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            cells_[y][x] = Cell{};
        }
    }
}

void TileWaveEffect::ApplyRing(int ring)
{
    if (!walls_ || !map_) return;
    auto* pm = ParticleManager::GetInstance();
    const int H = static_cast<int>(map_->GetNumBlockVirtical());
    const int W = static_cast<int>(map_->GetNumBlockHorizontal());

    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            if (ringIndex_[y][x] != ring) continue;

            if ((*walls_)[y][x]) {
                Cell& cell = cells_[y][x];
                if (!cell.visible) {
                    cell.visible = true;
                    cell.anim = 0.0f;
                    cell.tVisible = 0.0f;

                    // ★ 表示開始時は baseScale を保ちながらYだけ小さく
                    const Vector3& bs = cell.baseScale;
                    (*walls_)[y][x]->SetScale({ bs.x, std::max(0.001f, bs.y * 0.001f), bs.z });

                    // 粒子など…
                }

            }
        }
    }
}

void TileWaveEffect::Update(float dt)
{
    // --- 1) リング出しは active_ 中だけ進める ---
    if (active_) {
        waveTime_ += dt;

        int targetRing = static_cast<int>(std::floor(waveTime_ / perCellDelay_));
        if (targetRing > maxRing_) targetRing = maxRing_;

        
        for (int r = currentRing_; r <= targetRing; ++r) {
            ApplyRing(r);
        }
        currentRing_ = targetRing;
    }

    // --- 2) 可視タイルのアニメは active_ に関係なく最後まで回す ---
    bool animDone = true; // 全部が完了したら true のままになる
    for (uint32_t y = 0; y < walls_->size(); ++y) {
        for (uint32_t x = 0; x < (*walls_)[y].size(); ++x) {
            if (!(*walls_)[y][x]) continue;
            Cell& c = cells_[y][x];
            if (!c.visible) continue;

            // スケール（にょきっ）: baseScale に対して Z/Y をアニメ倍率で反映
            c.anim = std::min(1.0f, c.anim + dt / popDuration_);
            float sZ = EaseOutBack(c.anim);
            Vector3 s = c.baseScale;
            s.y = c.baseScale.y * std::max(0.001f, sZ);
            s.z = c.baseScale.z * std::max(0.001f, sZ);
            (*walls_)[y][x]->SetScale(s);

            // 揺れ（減衰サイン）
            c.tVisible += dt;
            const float h = map_->GetBlockHeight();
            const float amp = h * waveAmpRatio_;
            const float w = 2.0f * 3.1415926535f * waveFreq_;
            float bob = std::sin(w * c.tVisible) * amp * std::exp(-waveDamp_ * c.tVisible);

            Vector3 pos = cells_[y][x].basePos;
            pos.y += bob;
            (*walls_)[y][x]->SetTranslate(pos);

            // ★ 完了時は元スケールへ戻す
            if (c.anim >= 1.0f) {
                (*walls_)[y][x]->SetScale(c.baseScale);
                (*walls_)[y][x]->SetTranslate(cells_[y][x].basePos);
            } else {
                animDone = false; // まだ終わってないやつがある
            }
        }
    }

    // --- 3) 終了条件：リング出し終わり かつ 全可視のアニメ完了 ---
    bool ringsDone = (currentRing_ >= maxRing_);
    if (ringsDone && animDone) {
        active_ = false;
    }
}


void TileWaveEffect::Draw()
{
    if (!walls_) return;
    for (uint32_t y = 0; y < walls_->size(); ++y) {
        for (uint32_t x = 0; x < (*walls_)[y].size(); ++x) {
            if (!(*walls_)[y][x]) continue;
            if (!cells_[y][x].visible) continue; // ★ これが“非表示”の本質

            (*walls_)[y][x]->Update();
            (*walls_)[y][x]->Draw();
        }
    }
}


void TileWaveEffect::Skip()
{
    if (!walls_) return;
    for (uint32_t y = 0; y < walls_->size(); ++y) {
        for (uint32_t x = 0; x < (*walls_)[y].size(); ++x) {
            if (!(*walls_)[y][x]) continue;
            (*walls_)[y][x]->SetScale(cells_[y][x].baseScale); // ★ 元スケールを復元
            (*walls_)[y][x]->SetTranslate(cells_[y][x].basePos); // ★ 追加
            cells_[y][x].visible = true;
        }
    }
    active_ = false;

}
