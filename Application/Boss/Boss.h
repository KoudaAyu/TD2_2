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

    void Move();

public:
 
    struct Motion {
        virtual ~Motion() {}
        virtual void Start(Boss* owner) = 0;
        virtual void Update(Boss* owner, float dt) = 0;
        virtual bool IsFinished() const = 0;
    };

   
    Object3d* GetModel() const { return model_; }
    Object3d* GetLaserModel() const { return laserModel_; }
    Camera* GetCamera() const { return camera_; }
    Vector3 GetWorldTranslatePublic() const { return worldTransform_.GetTranslate(); }
    float GetLaserWidth() const { return laserWidth_; }
    float GetLaserMaxLength() const { return laserMaxLength_; }

    void SetPlayer(Player* player);

    const std::list<EnemyBullet*>& GetBullets() const { return bullets_; }
    bool IsActive() const { return isActive_; }

    void RegisterBullet(EnemyBullet* b) { bullets_.push_back(b); }

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
    enum class Phase { Spawn, Phase1, Phase2, Phase2_5, Phase3, Phase4, Phase5, Leave };
    Phase GetPhase() const { return phase_; }

    // Start a fancy visual/sound/particle transition when phases change
    void StartPhaseTransition();

    // Query whether boss is currently performing a phase transition (used to temporarily disable player actions)
    bool IsInPhaseTransition() const { return inPhaseTransition_; }

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
    int maxHP_ = 100; // 最大HP（デフォルト）
    int hp_ = 100; // 現在のHP

    // フェーズ境界（比率）。降順で指定。0.0f ~ 1.0f の範囲で設定。
    // 例: {1.0f, 0.8f, 0.6f, 0.4f, 0.2f} の場合、HP が 80% を下回ると Phase2 に遷移
    std::array<float, 6> hpPhaseThresholds_ = { 1.0f, 0.85f, 0.7f, 0.55f, 0.4f, 0.2f };

    // 現在のフェーズ
    Phase phase_ = Phase::Spawn;

    // ヒット回数（1ヒットで次のフェーズへ進行させるため）
    int hitCount_ = 0; // 初期は 0
    const int hitsPerFull = 6; // 6 ヒットでボス撃破（フェーズ5 到達で切替）

    // ヒットの短期無敵（多重カウント防止）
    int hitCooldownTimer_ = 0; // フレームカウント
    static constexpr int kHitCooldownFrames = 8; // 8フレームの無敵

    // フェーズ更新用（内部使用）
    void UpdatePhaseByHP();


    // --- デバッグ用スプライト ---
    // Phase1~Phase5 のときに画面に対応する番号画像を重ねて描画する
    SpriteCom* spriteCom_ = nullptr; // Sprite作成用コンポーネント
    std::array<Sprite*, 6> phaseSprites_ = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
    // スプライトの描画位置・スケールは簡単に変更できるようにメンバ化
    Vector2 phaseSpritePosition_ = { 10.0f, 10.0f }; // 画面左上に表示
    Vector2 phaseSpriteScale_ = { 64.0f, 64.0f };   // 表示サイズ

    void UpdatePhase1();
    int ShootTimer_ = 0;           // フレームカウンタ
    int ShootInterval_ = 30;       // 発射間隔（フレーム）
    float phase1BulletSpeed_ = 0.6f;     // 弾速
    int phase1BulletsPerShot_ = 1;       // 1 回につき発射する弾数（将来拡張用）

  
    void UpdatePhase2();
    void UpdatePhase2_5();
    int phase2ShootInterval_ = 25;
    int phase2BulletsPerShot_ = 1; //弾をいくつ発射するか
    float phase2BulletSpeed_ = 0.7f;
    float phase2TurnRate_ = 0.06f; // 追従の曲がりやすさ
    int phase2BulletLifeFrames_ = 60; // 追従を何フレームするか

    float cameraShakeCooldown_ = 0.0f; 
    static constexpr float kCameraShakeCooldownSeconds = 0.25f; 

    
   
    float phase1MoveSpeed_ = 0.01f;
    // 上下に揺れるボビング振幅と周波数
    float phase1BobAmplitude_ = 0.12f; 
    float phase1BobFrequency_ = 0.4f;
    // 内部経過時間（秒）
    float phase1Time_ = 0.0f;

    // --- 追加: Phase1 の左右上下運動（軌道）パラメータ ---
    // X/Y 軸の軌道半径
    float phase1OrbitRadiusX_ = 1.4f; 
    float phase1OrbitRadiusY_ = 0.5f; 
    // X/Y 軸の軌道速度（角速度的な意味合い）
    float phase1OrbitSpeedX_ = 0.12f; 
    float phase1OrbitSpeedY_ = 0.15f; 

    // 目標オフセット（軌道で計算される）
    Vector3 phase1TargetOffset_ = { 0.0f, 0.0f, 0.0f };

    // --- Phase4 レーザー攻撃 ---
    void UpdatePhase4();
    Object3d* laserModel_ = nullptr; // レーザー表示用モデル（弾と同じ元モデルを複製）
    bool laserActive_ = false;
    int laserTimer_ = 0;
    int laserChargeFrames_ = 120;    // チャージ時間 
    int laserFireFrames_ = 120;     // 発射時間
    int laserCooldownFrames_ = 90;  // クールダウン
    float laserMaxLength_ = 40.0f;  // 最大長さ(Z方向)
    float laserWidth_ = 0.6f;       // 幅(X,Y方向スケール)
    Vector4 laserChargeColor_ = {1.0f, 0.3f, 0.3f, 0.7f};
    Vector4 laserFireColor_ = {1.0f, 0.9f, 0.2f, 1.0f};
    // プレイヤー判定のパディング（プレイヤーの見た目に合わせて幅を加算）
    float laserPlayerHitPaddingXY_ = 0.4f; // X/Y方向の追加半径
    // Z方向の判定補正（深さ方向の猶予）
    float laserPlayerHitPaddingZ_ = 0.6f; // Z方向の追加半径
    // プレイヤーAABB半径（モデルに依存しない簡易当たり判定用）
    float playerHitHalfSizeXY_ = 0.4f;
    float playerHitHalfSizeZ_ = 0.4f;

    // Phase4 turrets (mass-produced enemies that fire beams)
    struct Phase4Turret {
        Object3d* obj = nullptr;
        int fireTimer = 0;
        int fireInterval = 36; // frames
        bool active = true;
    };
    std::vector<Phase4Turret> phase4Turrets_;
    int phase4TurretCount_ = 4;
    float phase4TurretRadius_ = 3.2f;
    float phase4TurretScale_ = 0.5f;

    // --- Phase3: 周回ドローン + 狭角連射 ---
    void UpdatePhase3();

    struct Drone {
        float angle = 0.0f;        
        float radius = 2.4f;      
        float orbitSpeed = 0.06f;  
        int shootTimer = 0;        
        int shootInterval = 45;    
        bool active = true;
    };

    std::vector<Drone> drones_;
    int phase3DroneCount_ = 4;
    int phase3Timer_ = 0;
    float phase3BulletSpeed_ = 0.9f;

   
    std::vector<Object3d*> droneObjs_;
    float droneModelScale_ = 0.45f;


    void UpdatePhase5();
    int phase5BurstTimer_ = 0;           // フレームカウント
    int phase5BurstInterval_ = 30;       // 間隔（フレーム） -- 増やして間隔を開ける
    int phase5BulletsPerBurst_ = 6;      // 1 バーストあたりの弾数 -- 少なめに (レーン数として利用)
    float phase5BulletSpeed_ = 0.85f;    // 弾速 -- 少し遅く
    float phase5SpinRate_ = 0.06f;       // バースト毎の角度進行量 (未使用だが残す)
    float phase5Angle_ = 0.0f;           // 現在の回転角

 
    float phase5LaneSpacing_ = 1.2f;     // レーン間隔
    bool phase5SweepMode_ = false;       // スイープモード有無（横に流す）
    float phase5SweepSpeed_ = 0.18f;     // スイープ時の横速度
    int phase5BurstCount_ = 0;           // バースト発生回数

    std::unique_ptr<Motion> currentMotion_ = nullptr;
    Phase lastPhase_ = Phase::Spawn;

    void StartLaserPreMotion();

    // phase transition internal flag and timer
    bool inPhaseTransition_ = false;
    int phaseTransitionTimer_ = 0;
    int phaseTransitionDuration_ = 30; // frames to block player input by default

    // --- 新規: レーザーを派手にするためのパラメータ ---
    // 横スイープ量（角度）
    float laserSweepAngle_ = 0.0f;
    float laserSweepSpeed_ = 0.12f;        // スイープ速度
    float laserSweepAmplitude_ = 1.8f;     // X方向の振幅
    // プレイヤー方向へのゆっくり追従
    float laserAimX_ = 0.0f;               // 内部保持用のXオフセット
    float laserAimLerpSpeed_ = 0.06f;      // 追従のLerp速度

    // 発射中にレーザーから子弾を出すためのタイマー
    int laserBulletInterval_ = 8;          // フレーム間隔
    int laserBulletTimer_ = 0;
    float phase4BulletSpeed_ = 1.2f;       // 発射される子弾の速度
};
