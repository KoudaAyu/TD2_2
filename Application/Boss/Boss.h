#pragma once

#include <vector>
#include <list>
#include <memory>
#include <cstdint>
#include <cassert>
#include <array>

#include "Camera.h"
#include "EnemyBullet.h"
#include "Object3d.h"
#include "Object3dCom.h"
#include "Transform.h"
#include "BossPart.h"

class Player;
class SpriteCom;
class Sprite;

class Boss
{
public:
    Boss();
    ~Boss();

    // SpriteCom* を受け取るように変更（フェーズ表示用のスプライトを作成するため）
    void Initialize(Object3d* model, Camera* camera, const Vector3 pos, Object3dCom* object3dCom, SpriteCom* spriteCom);
    void Update();
    void Draw();

    void OnCollision();

    // プレイヤーの弾やバリアに当たったときの処理（デバッグ: フェーズ進行）
    void OnHit();

    /// <summary>
    /// 弾を発射する
    /// </summary>
    void Shoot();

public:

    void SetPlayer(Player* player) { player_ = player; }

    const std::list<EnemyBullet*>& GetBullets() const { return bullets_; }
    bool IsActive() const { return isActive_; }

    void RegisterBullet(EnemyBullet* b) { bullets_.push_back(b); }

    Camera* GetCamera() const { return camera_; }
    Object3dCom* GetObject3dCom() const { return object3dCom_; }
    Player* GetPlayer() const { return player_; }

    // ワールド変換の取得（部位がボス位置に追従するため）
    const Matrix4x4& GetWorldMatrix() const { return worldTransform_.GetWorldMatrix(); }
    Vector3 GetWorldTranslate() const { return worldTransform_.GetTranslate(); }

    // プレイヤーのワールド座標を返すラッパー（parts が Player を直接参照しなくて済む）
    Vector3 GetPlayerWorldTranslate() const;

    // --- HP / フェーズ制御に関する API ---
    // 最大HP を設定（外部から調整しやすくするため）
    void SetMaxHP(int maxHP) { maxHP_ = maxHP; if (hp_ > maxHP_) hp_ = maxHP_; }
    // 現在HP を取得
    int GetHP() const { return hp_; }
    // ダメージを与える（フェーズ判定は Update 内で自動的に行われる）
    void ApplyDamage(int dmg) { hp_ -= dmg; if (hp_ < 0) hp_ = 0; }

    // 現在のフェーズを取得
    enum class Phase { Spawn, Phase1, Phase2, Phase3, Phase4, Phase5, Leave };
    Phase GetPhase() const { return phase_; }

private:
    std::vector<std::unique_ptr<BossPart>> parts_;
    std::list<EnemyBullet*> bullets_;

    Transform worldTransform_ = {};
    Object3d* model_ = nullptr;
    Object3dCom* object3dCom_ = nullptr;
    Camera* camera_ = nullptr;
    Player* player_ = nullptr;

    bool isActive_ = true;

    // 生成（スポーン）関連
    Vector3 spawnStart_ = { 0.0f, 0.0f, 0.0f };
    Vector3 spawnTarget_ = { 0.0f, 0.0f, 0.0f };
    int spawnTimer_ = 0;
    int spawnDuration_ = 180;

    // --- HP / フェーズ関連 ---
    // 最大HP（簡単に変更可能）
    int maxHP_ = 100; // 最大HP（デフォルト）
    // 現在HP
    int hp_ = 100; // 現在のHP

    // フェーズ境界（比率）。降順で指定。0.0f ~ 1.0f の範囲で設定。
    // 例: {1.0f, 0.8f, 0.6f, 0.4f, 0.2f} の場合、HP が 80% を下回ると Phase2 に遷移
    std::array<float, 5> hpPhaseThresholds_ = { 1.0f, 0.8f, 0.6f, 0.4f, 0.2f };

    // 現在のフェーズ
    Phase phase_ = Phase::Spawn;

    // ヒット回数（1ヒットで次のフェーズへ進行させるため）
    int hitCount_ = 0; // 初期は 0
    const int hitsPerFull = 5; // 5 ヒットでボス撃破（フェーズ5 到達で切替）

    // ヒットの短期無敵（多重カウント防止）
    int hitCooldownTimer_ = 0; // フレームカウント
    static constexpr int kHitCooldownFrames = 8; // 8フレームの無敵

    // フェーズ更新用（内部使用）
    void UpdatePhaseByHP();


    // --- デバッグ用スプライト ---
    // Phase1~Phase5 のときに画面に対応する番号画像を重ねて描画する
    SpriteCom* spriteCom_ = nullptr; // Sprite作成用コンポーネント
    std::array<Sprite*, 5> phaseSprites_ = { nullptr, nullptr, nullptr, nullptr, nullptr };
    // スプライトの描画位置・スケールは簡単に変更できるようにメンバ化
    Vector2 phaseSpritePosition_ = { 10.0f, 10.0f }; // 画面左上に表示
    Vector2 phaseSpriteScale_ = { 64.0f, 64.0f };   // 表示サイズ

    void UpdatePhase1();
    int ShootTimer_ = 0;           // フレームカウンタ
    int ShootInterval_ = 30;       // 発射間隔（フレーム）
    float phase1BulletSpeed_ = 0.6f;     // 弾速
    int phase1BulletsPerShot_ = 1;       // 1 回につき発射する弾数（将来拡張用）

    float cameraShakeCooldown_ = 0.0f; 
    static constexpr float kCameraShakeCooldownSeconds = 0.25f; 
};
