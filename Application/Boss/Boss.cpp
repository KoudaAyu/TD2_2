#include "Boss.h"
#include "HeadPart.h"
#include "TurretPart.h"
#include "BodyPart.h"
#include "Player.h"

#include <algorithm>
#include "SpriteCom.h"
#include "Sprite.h"
#include <filesystem>

Boss::Boss() {}

Boss::~Boss()
{
    for (EnemyBullet* b : bullets_)
    {
        delete b;
    }
    bullets_.clear();

    // スプライト解放
    for (auto s : phaseSprites_)
    {
        if (s) delete s;
    }
}

Vector3 Boss::GetPlayerWorldTranslate() const
{
    if (player_) return player_->GetWorldTranslate();
    return {0.0f, 0.0f, 0.0f};
}


static std::string FindNumberTexturePath(const std::string& fileName)
{
    // 候補パス（実行時カレントやプロジェクト構成に応じてチェック）
    const std::vector<std::string> candidates = {
        std::string("Resources/number/") + fileName,
        fileName
    };

    for (const auto& c : candidates)
    {
        if (std::filesystem::exists(std::filesystem::path(c)))
        {
            return c;
        }
    }

    return std::string(); // 見つからない
}


void Boss::Initialize(Object3d* model, Camera* camera, const Vector3 pos, Object3dCom* object3dCom, SpriteCom* spriteCom)
{
    model_ = model;
    camera_ = camera;
    object3dCom_ = object3dCom;

    // SpriteCom を保持（デバッグ表示用）
    spriteCom_ = spriteCom;

    worldTransform_.Initialize();
    spawnTarget_ = pos;
    spawnStart_ = { pos.x, pos.y + 8.0f, pos.z + 40.0f };
    worldTransform_.SetTranslate(spawnTarget_);

    if (model_)
    {
        model_->ApplyState(worldTransform_, camera_, true);
    }

    const int gridSizeX = 3;
    const int gridSizeY = 3;
    const float spacing = 1.2f;

    float offsetX = -(gridSizeX - 1) * 0.5f * spacing;
    float offsetY = -(gridSizeY - 1) * 0.5f * spacing;

    for (int y = 0; y < gridSizeY; ++y)
    {
        for (int x = 0; x < gridSizeX; ++x)
        {
            Vector3 partPos = { offsetX + x * spacing, offsetY + y * spacing, 0.0f };
            parts_.push_back(std::make_unique<BodyPart>());
            parts_.back()->Initialize(this, model_, partPos);
        }
    }

    const int layers = 2;
    for (int l = 1; l < layers; ++l)
    {
        float layerHeight = 0.9f * l;
        for (int y = 0; y < gridSizeY; ++y)
        {
            for (int x = 0; x < gridSizeX; ++x)
            {
                Vector3 partPos = { offsetX + x * spacing, offsetY + y * spacing, layerHeight };
                parts_.push_back(std::make_unique<BodyPart>());
                parts_.back()->Initialize(this, model_, partPos);
            }
        }
    }

    for (size_t i = 0; i < parts_.size(); ++i)
    {
        BossPart* p = parts_[i].get();
        if (!p) continue;
        Vector3 targetLocal = p->GetWorldTranslate();
        Vector3 bossPos = worldTransform_.GetTranslate();
        Vector3 localTarget = { targetLocal.x - bossPos.x, targetLocal.y - bossPos.y, targetLocal.z - bossPos.z };

        float scatterScale = 4.0f;
        Vector3 startLocal = { localTarget.x * scatterScale, localTarget.y * scatterScale + 6.0f, localTarget.z + 12.0f };

        int dur = spawnDuration_;
        p->StartSpawn(startLocal, dur);
    }

    isActive_ = true;

    
    phase_ = Phase::Spawn;
    spawnTimer_ = 0;

    // 初期 HP 設定（最大 HP にリセット）
    hp_ = maxHP_;

    // ヒットカウント初期化
    hitCount_ = 0;
    hitCooldownTimer_ = 0;

    // デバッグ用スプライト作成 
    if (spriteCom_)
    {
        // 1.png ～ 5.png を読み込み
        for (int i = 0; i < 5; ++i)
        {
            std::string fileName = std::to_string(i + 1) + ".png";
            std::string path = FindNumberTexturePath(fileName);
            if (path.empty())
            {
                // 見つからなければ作成をスキップ（assert を回避）
                phaseSprites_[i] = nullptr;
                continue;
            }

            Sprite* s = spriteCom_->CreateSprite(path, phaseSpritePosition_, phaseSpriteScale_, 0.0f, {0.0f,0.0f}, false, false);
            phaseSprites_[i] = s;
            // 初期は非表示にするため alpha を 0 に（Sprite の色にアクセス）
            Vector4 c = s->GetColor();
            c.w = 0.0f; // alpha
            s->SetColor(c);
            s->Update();
        }
    }
}

void Boss::OnHit()
{
    // Spawn/Leave 時や無敵中はカウントしない
    if (phase_ == Phase::Spawn || phase_ == Phase::Leave) return;
    if (hitCooldownTimer_ > 0) return; // 無敵フレーム中は無視

    ++hitCount_;

    if (hitCount_ > hitsPerFull) hitCount_ = hitsPerFull;

    // 1ヒットで Phase2 に見せたい場合はインデックスずらしを行う
    int clampedIndex = (std::min)(hitCount_, hitsPerFull - 1);
    Phase newPhase = static_cast<Phase>(static_cast<int>(Phase::Phase1) + clampedIndex);
    phase_ = newPhase;

    // カメラ揺れ
    if (camera_) camera_->StartShake(0.2f, 0.2f);

    // スプライト更新
    if (spriteCom_)
    {
        int currentPhaseIndex = (phase_ >= Phase::Phase1 && phase_ <= Phase::Phase5) ?
            static_cast<int>(phase_) - static_cast<int>(Phase::Phase1) : 0;
        for (int i = 0; i < 5; ++i)
        {
            Sprite* s = phaseSprites_[i];
            if (!s) continue;
            Vector4 col = s->GetColor();
            col.w = (currentPhaseIndex == i) ? 1.0f : 0.0f;
            s->SetColor(col);
            s->Update();
        }
    }

    // 無敵タイマーをリセットして短時間多重カウントを防止
    hitCooldownTimer_ = kHitCooldownFrames;

    // ヒットが上限に達したらボスを撃破扱いにする（シーン遷移のトリガ）
    if (hitCount_ >= hitsPerFull)
    {
        // ここで部位を破壊する演出を追加しても良いが、簡易的に衝突処理を呼びボスを無効化する
        OnCollision();
    }
}

void Boss::Update()
{
    if (!isActive_)
    {
        for (EnemyBullet* b : bullets_)
        {
            if (b) b->Update();
        }
        return;
    }

    // ヒット無敵タイマー更新
    if (hitCooldownTimer_ > 0) --hitCooldownTimer_;

    if (phase_ == Phase::Spawn)
    {
        ++spawnTimer_;
        float t = (spawnDuration_ <= 0) ? 1.0f : (float)spawnTimer_ / (float)spawnDuration_;
        if (t > 1.0f) t = 1.0f;

       
        for (auto& p : parts_)
        {
            if (p) p->UpdateSpawn(t);
        }

        if (spawnTimer_ >= spawnDuration_)
        {
            phase_ = Phase::Phase1; // スポーン後はフェーズ1 から開始
          
            if (camera_)
            {
                camera_->StartShake(0.6f, 0.6f);
            }
        }
    }

    // フェーズ更新
    UpdatePhaseByHP();

    // デバッグ用: フェーズに合わせてスプライトの表示切替
    if (spriteCom_)
    {
        for (int i = 0; i < 5; ++i)
        {
            Sprite* s = phaseSprites_[i];
            if (!s) continue;
            Vector4 c = s->GetColor();
            int currentPhaseIndex = 0;
            if (phase_ >= Phase::Phase1 && phase_ <= Phase::Phase5)
            {
                currentPhaseIndex = static_cast<int>(phase_) - static_cast<int>(Phase::Phase1);
            }
            c.w = (currentPhaseIndex == i) ? 1.0f : 0.0f;
            s->SetColor(c);
            s->Update();
        }
    }

    for (auto& p : parts_)
    {
        if (p) p->Update();
    }

    for (EnemyBullet* b : bullets_)
    {
        if (b) b->Update();
    }

    worldTransform_.TransferMatrix();
    if (model_) model_->ApplyState(worldTransform_, camera_, true);

    bool allDestroyed = true;
    for (auto& p : parts_)
    {
        if (p && !p->IsDestroyed())
        {
            allDestroyed = false;
            break;
        }
    }
    if (allDestroyed)
    {
        isActive_ = false;
    }
}

void Boss::Draw()
{
    for (EnemyBullet* b : bullets_)
    {
        if (b) b->Draw();
    }


    for (auto& p : parts_)
    {
        if (p) p->Draw();
    }

    // デバッグ用スプライトの描画（Phase1~Phase5 のみ表示）
    if (spriteCom_)
    {
        for (int i = 0; i < 5; ++i)
        {
            Sprite* s = phaseSprites_[i];
            if (!s) continue;
            s->Draw();
        }
    }
}

void Boss::OnCollision()
{
    worldTransform_.SetTranslate({ 10000.0f, 10000.0f, 10000.0f });
    for (EnemyBullet* b : bullets_)
    {
        if (b) b->OnCollision();
    }
    isActive_ = false;
}

// HP に基づいてフェーズを更新する
void Boss::UpdatePhaseByHP()
{
    // Spawn / Leave 中は遷移を行わない
    if (phase_ == Phase::Spawn || phase_ == Phase::Leave) return;

    // まずヒット数ベースの優先処理：ヒットでフェーズを進める設計になっている場合はこちらを優先
    if (hitCount_ > 0)
    {
        int clampedIndex = (std::min)(hitCount_, hitsPerFull - 1);
        Phase newPhase = static_cast<Phase>(static_cast<int>(Phase::Phase1) + clampedIndex);
        if (newPhase != phase_)
        {
            phase_ = newPhase;
            if (camera_) camera_->StartShake(0.3f, 0.3f);
        }
        return; // ヒットベース優先なので HP による更新は行わない
    }

    if (maxHP_ <= 0) return;

    float hpRatio = static_cast<float>(hp_) / static_cast<float>(maxHP_);

    // hpPhaseThresholds_ 配列は {1.0, 0.8, 0.6, 0.4, 0.2} のように 5 段階を保持
    // フェーズは Phase1 ~ Phase5 と対応する。hpRatio がしきい値を下回ったら次のフェーズへ。

    Phase newPhase = Phase::Phase1;

    for (size_t i = 0; i < hpPhaseThresholds_.size(); ++i)
    {
        float threshold = hpPhaseThresholds_[i];
        if (hpRatio <= threshold)
        {
            newPhase = static_cast<Phase>(static_cast<int>(Phase::Phase1) + static_cast<int>(i));
        }
    }

    if (newPhase != phase_)
    {
        phase_ = newPhase;
        if (camera_)
        {
            camera_->StartShake(0.3f, 0.3f);
        }
    }
}
