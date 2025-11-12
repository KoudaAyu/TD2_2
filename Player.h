#pragma once
#include "AABB.h"
#include "Camera.h"
#include "KeyInput.h"
#include "MapChipField.h"
#include "Model.h"
#include"SoundManager.h"
#include "Transform.h"
#include <deque>
#include "ParticleManager.h"

enum class LRDirection { Left, Right, Unknown };
enum class UDDirection { Up, Down, Unknown };
class Player
{
public:

	Player() = default;
	~Player();

	void Initialize(Object3d* model, Camera* camera, const Vector3 pos, Object3dCom* object3dCom);
	void Update();
	void Draw();

	void Grow();           // 体を伸ばす
	void RemoveLastPart(); // 体を1つ減らす

	// 頭AABBと体AABBを含めて更新
	void UpdateAABB();

	// 爆弾関係
	void EatBomb();         // 爆弾を食べた時の処理
	void UpdateBomb();      // 爆弾進行の更新
	void DetachBombParts(); // 切り離し処理

	bool IsOccupyingGrid(int gridX, int gridY) const;

	std::vector<std::pair<int, int>> GetOccupiedGridPositions() const;
	std::vector<std::pair<int, int>> GetWallGridPositions() const;

	// 一度だけ切り離し入力（SPACE/A）を無視する（ポーズ復帰対策）
	void SuppressCutOnce() { suppressCutOnce_ = true; }

	// 追加: 移動ロックの制御と切り離し検出
	void SetMovementLocked(bool v) { movementLocked_ = v; }
	bool IsMovementLocked() const { return movementLocked_; }
	bool HasDetachedThisFrame() const { return detachedThisFrame_; }
	void ClearDetachedThisFrame() { detachedThisFrame_ = false; }

	// 追加: 爆弾進行速度の設定/取得（1ステップあたりの秒数）
	void SetBombStepTime(float seconds) { bombStepTime_ = (seconds > 0.0f) ? seconds : bombStepTime_; }
	float GetBombStepTime() const { return bombStepTime_; }

public:
	const Vector3 GetPosition() const { return worldTransform_.translation_; }
	Vector3 SetPosition(const Vector3& position);

	// 従来互換: 頭AABBを返す
	const AABB& GetAABB() const { return playerAABB; }

	// 新規: 頭と体のAABBアクセス
	const AABB& GetHeadAABB() const { return headAABB_; }
	const std::vector<AABB>& GetBodyAABBs() const { return bodyAABBs_; }

	// 当たり判定ヘルパー
	bool IsHeadCollidingWith(const AABB& other) const;
	bool IsBodyCollidingWith(const AABB& other) const;

	void SetIsAlive(bool isAlive) { isAlive_ = isAlive; }
	const bool GetIsAlive() const { return isAlive_; }
	//const Vector3 GetScale() const { return worldTransform_.scale_; }
	Vector3* GetScalePtr() { return &worldTransform_.scale_; }

	Object3d* GetObject3d() const { return model_; }

	bool CheckSelfCollisionGrid() const;
	bool CheckHeadCollisionWithDetachedWalls() const;

	// --- 追加：設置済みの壁数を取得 ---
	int GetPlacedWallCount() const { return static_cast<int>(wallTransforms_.size()); }

	
private:


	Vector2 gridPos_ = { 0.0f, 0.0f };
	Vector2 targetGridPos_ = { 0.0f, 0.0f };
	Vector2 direction_ = { 1.0f, 0.0f }; // 右向き
	Vector2 nextDirection_ = { 1.0f, 0.0f };

	bool isMoving_ = false; // 最初の状態。動かない
	float moveTimer_ = 0.0f;

	Vector3 startPos_;
	Vector3 endPos_;

	static constexpr float kMoveDuration = 0.5f; // 何秒かけて一マス移動するか

	bool isAlive_ = true;

	Vector3 velocity_ = {};

	// 爆弾関連
	bool bombActive_ = false; // 爆弾を飲み込んでいるか
	int bombProgress_ = 0;    // 爆弾進行度（何個分赤くなったか）
	int bombStartIndex_ = -1; // 爆弾が始まる体のインデックス
	float bombTimer_ = 0.0f;  // 爆弾進行用タイマー
	static constexpr float kDefaultBombStepTime = 0.5f; // 1マス赤くなるまでの時間（デフォルト）
	float bombStepTime_ = kDefaultBombStepTime;         // シーンごとに変更可能

	// 次のUpdateの1回だけ切り離し入力を無視するフラグ
	bool suppressCutOnce_ = false;

	// 追加: チュートリアル側が移動のみを禁止するためのロック
	bool movementLocked_ = false;
	// 追加: 今フレームで切り離しが行われたか
	bool detachedThisFrame_ = false;

private: // 変更禁止
	Camera* camera_ = nullptr;
	MapChipField* mapChipField_ = nullptr;
	Object3d* model_ = nullptr;
	Object3dCom* object3dCom_ = nullptr;
	uint32_t textureHandle_ = 0u;
	Transform worldTransform_{};

	
	Vector3 baseScale_{ 1.0f, 1.0f, 1.0f };

	// ブロックModelのサイズと一致させる
	static constexpr float unitLength = 2.0f;

	// 旧: 後方互換用（頭AABBをミラー）
	AABB playerAABB;

	// 新: 頭AABBと体AABB群
	AABB headAABB_;
	std::vector<AABB> bodyAABBs_;
	static constexpr float kAabbScale = 0.5f; // unitLengthに対する当たり判定縮小率

	// 向き
	LRDirection lrDirection_ = LRDirection::Unknown;
	UDDirection udDirection_ = UDDirection::Unknown;

	Vector2 lStick = { 0.0f, 0.0f };
	KeyInput* key = nullptr;

	// 体の増加部分（bodyParts_[0]は頭の位置）
	std::vector<Vector3> bodyParts_;
	std::deque<Transform> bodyPartTransforms_;

	// 追従遅延フレーム数
	static constexpr size_t kFollowDelay = 30; // 30フレーム遅れ

	// 頭の座標履歴
	std::deque<Vector3> headHistory_;
	// 頭の回転履歴（座標履歴と同じタイミングでpush/popし、インデックスを一致させる）
	std::deque<Vector3> headRotationHistory_;

	float deltaTime_ = 1.0f / 60.0f;

	// Playerの切り捨てた部分を壁にする
	std::vector<Vector3> wallPositions_;
	std::deque<Transform> wallTransforms_;

	std::vector<Object3d*> bodyModels_;//Body用のモデル
	std::vector<Object3d*> wallModels_;
	std::vector<bool> bodyPartIsRed_;

	SoundData Player_Cut_SoundData_;
	SoundData Player_Walk_SoundData_;
};