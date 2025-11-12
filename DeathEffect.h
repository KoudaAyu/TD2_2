#pragma once
#include <vector>
#include "Vector.h"
#include "MapChipField.h"
#include "Player.h"
#include "SpriteCom.h"

// プレイヤー死亡時の「一気にバラ撒き」粒子演出
class DeathEffect {
public:
    enum class Mode { Sequential, BurstAll };   // 今回は BurstAll を既定
    enum class Order { HeadToTail, TailToHead }; // 指定は一応残す（Burstでも使える）

    // 既定：BurstAll / HeadToTail
    DeathEffect() = default;

    // 設定
    void SetMode(Mode  m) { mode_ = m; }
    void SetOrder(Order o) { order_ = o; }

    // 開始（死亡検知直後に呼ぶ）
    void Start(MapChipField* map, const Player* player);

    // 更新（ゲームオーバー中も1/60で回す）
    void Update(float dt);

    // 必要ならオーバーレイ描画（今は未使用）
    void Draw(SpriteCom* /*spriteCom*/) {}

    bool IsFinished() const { return finished_; }
    bool IsActive()   const { return active_; }

private:
    // 体の各セル（ワールド座標）
    std::vector<Vector3> cells_;

    // 逐次用（残しておく）
    int   index_ = 0;
    float interval_ = 0.04f;
    float timer_ = 0.0f;

    // BurstAll 用
    bool  burstFired_ = false;    // 1回だけ全セル発生
    float linger_ = 0.35f;    // 余韻（秒）
    float alive_ = 0.0f;

    bool  active_ = false;
    bool  finished_ = false;

    Mode  mode_ = Mode::BurstAll;          // ★既定：一気に
    Order order_ = Order::HeadToTail;       // ★既定：頭→尾
};
