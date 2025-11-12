#include "Player.h"
#include <algorithm>
#include <cmath> 
#include <cassert>
#include <numbers>


extern KeyInput keyInput;
extern SoundManager* soundManager;

Player::~Player()
{
	for (auto* body : bodyModels_)
	{
		delete body->GetModel();
		delete body;
	}
	bodyModels_.clear();

	for (auto* wall : wallModels_)
	{
		if (wall && wall->GetModel() && wall->GetModel() != model_->GetModel())
		{
			delete wall->GetModel();
		}
		delete wall;
	}
	wallModels_.clear();
}

void Player::Initialize(Object3d* model, Camera* camera, const Vector3 pos, Object3dCom* object3dCom)
{

#ifdef _DEBUG
	assert(model);
	assert(camera);
#endif
	model_ = model;
	camera_ = camera;
	object3dCom_ = object3dCom;

	key = KeyInput::GetInstance();

	// ワールド座標 -> グリッド座標（中心が (n+0.5)*unitLength）
	gridPos_.x = std::round(pos.x / unitLength - 0.5f);
	gridPos_.y = std::round(pos.y / unitLength - 0.5f);
	targetGridPos_ = gridPos_;

	// 向き
	direction_ = { 0.0f, 0.0f };
	nextDirection_ = { 0.0f, 0.0f };
	isMoving_ = false;

	if (model_)
	{
		worldTransform_.scale_ = model_->GetScale();
	}
	else
	{
		worldTransform_.scale_ = { 1.0f, 1.0f, 1.0f };
	}

	
	baseScale_ = worldTransform_.scale_;

	// グリットに合わせてワールド座標を設定
	worldTransform_.translation_.x = (gridPos_.x + 0.5f) * unitLength; // 0.5fを加えてグリッドの中心に配置
	worldTransform_.translation_.y = (gridPos_.y + 0.5f) * unitLength; // 0.5fを加えてグリッドの中心に配置
	worldTransform_.translation_.z = 0.0f;

	bodyParts_.clear();
	bodyParts_.push_back(worldTransform_.translation_);

	headHistory_.clear();
	for (size_t i = 0; i < kFollowDelay * bodyParts_.size(); ++i)
	{
		headHistory_.push_back(worldTransform_.translation_);
	}
	// 回転履歴も同じ長さで初期化
	headRotationHistory_.clear();
	for (size_t i = 0; i < kFollowDelay * bodyParts_.size(); ++i)
	{
		headRotationHistory_.push_back(worldTransform_.rotation_);
	}

	bodyPartTransforms_.emplace_back();
	// bodyPartTransforms_.back().Initialize();
	bodyPartTransforms_.back().scale_ = worldTransform_.scale_;
	bodyPartTransforms_.back().rotation_ = worldTransform_.rotation_;
	bodyPartTransforms_.back().translation_ = bodyParts_.back();

	bodyPartIsRed_.clear();
	bodyPartIsRed_.push_back(false);

	// 初期の向き
	worldTransform_.rotation_.y = std::numbers::pi_v<float> / 2.0f;
	


	if (model_)
	{
		model_->SetCamera(camera_);
		model_->SetTranslate(worldTransform_.translation_);
		model_->SetRotate(worldTransform_.rotation_);
		model_->SetScale(worldTransform_.scale_);
		// 即時反映が必要なら Update を呼ぶ（シーン側で毎フレ呼んでいるなら不要）
		model_->Update();
	}


	// パーティクル
	ParticleManager::GetInstance()->CreateParticleGroup("placeBurst", "Resources/body.png");

	// サウンドデータ読み込み
	Player_Cut_SoundData_ = soundManager->SoundLoadWave("Resources/Audio/SE/player_cut.wav");
	Player_Walk_SoundData_ = soundManager->SoundLoadWave("Resources/Audio/SE/player_walk.wav");

#ifdef _DEBUG
	assert(&Player_Cut_SoundData_);
	assert(&Player_Walk_SoundData_);
#endif



}

void Player::Update()
{

	if (!isAlive_)
	{
		return;
	}

	//コントローラー入力
	constexpr float stickThreshold = 0.2f;
	auto s = keyInput.GetLeftStick(0);
	lStick.x = s.x;
	lStick.y = s.y;

	// ポーズ復帰直後の誤爆防止：一回だけ切り離し入力を無視
	bool cutRequested = (keyInput.TriggerKey(DIK_SPACE) || keyInput.TriggerPadButton(XINPUT_GAMEPAD_A));
	if (suppressCutOnce_)
	{
		cutRequested = false;     // このフレームは無視
		suppressCutOnce_ = false;  // 次フレームから通常処理
	}

	// 今フレームの切り離し判定フラグをリセット
	detachedThisFrame_ = false;

	if (cutRequested)
	{
		// BODYが0（=頭のみ）のときは何もしない
		const size_t bodyCount = (bodyParts_.size() > 0) ? (bodyParts_.size() - 1) : 0;
		if (bodyCount > 0)
		{
			soundManager->SoundPlayWave(Player_Cut_SoundData_);

			if (bombActive_)
			{
				DetachBombParts();
			}
			else
			{
				RemoveLastPart();
			}
			// 切り離しが実行された
			detachedThisFrame_ = true;
			// 切り離したら移動ロックは解除しても良いが、外部で制御したいのでここでは触らない
		}
	}

	// movementLocked_ の間は、移動入力と移動更新を止める
	if (movementLocked_)
	{
		// AABB更新や見た目更新は続行
		UpdateBomb();
		UpdateAABB();

		worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
		worldTransform_.TransferMatrix();

		if (model_)
		{
			model_->SetTranslate(worldTransform_.translation_);
			model_->SetRotate(worldTransform_.rotation_);
			model_->SetScale(worldTransform_.scale_);
			model_->SetCamera(camera_);
			model_->Update();
		}
		return;
	}

	if ((keyInput.TriggerKey(DIK_A) || keyInput.TriggerKey(DIK_LEFT) ||
		keyInput.TriggerPadButton(XINPUT_GAMEPAD_DPAD_LEFT) || lStick.x < -stickThreshold) && direction_.x != 1.0f)
	{
		
		nextDirection_ = { -1.0f, 0.0f };
	}
	if ((keyInput.TriggerKey(DIK_D) || keyInput.TriggerKey(DIK_RIGHT) ||
		keyInput.TriggerPadButton(XINPUT_GAMEPAD_DPAD_RIGHT) || lStick.x > stickThreshold) && direction_.x != -1.0f)
	{
		nextDirection_ = { 1.0f, 0.0f };
	}
	if ((keyInput.TriggerKey(DIK_W) || keyInput.TriggerKey(DIK_UP) ||
		keyInput.TriggerPadButton(XINPUT_GAMEPAD_DPAD_UP) || lStick.y > stickThreshold) && direction_.y != -1.0f)
	{
		nextDirection_ = { 0.0f, 1.0f };
	}
	if ((keyInput.TriggerKey(DIK_S) || keyInput.TriggerKey(DIK_DOWN) ||
		keyInput.TriggerPadButton(XINPUT_GAMEPAD_DPAD_DOWN) || lStick.y < -stickThreshold) && direction_.y != 1.0f)
	{
		nextDirection_ = { 0.0f, -1.0f };
	}

	// 移動開始判定
	if (!isMoving_)
	{
		Vector2 nextGrid = { gridPos_.x + nextDirection_.x, gridPos_.y + nextDirection_.y };

		bool canMove = true;
		if (mapChipField_)
		{
			canMove = mapChipField_->IsMovable(static_cast<int>(nextGrid.x), static_cast<int>(nextGrid.y));
		}
		// 体との衝突判定もここで追加可能

		if (canMove)
		{
			// 直前の向きと比較して変化があれば歩行SEを再生
			Vector2 prevDir = direction_;
			if (nextDirection_.x != prevDir.x || nextDirection_.y != prevDir.y)
			{
				soundManager->SoundPlayWave(Player_Walk_SoundData_, false,0.2f);
			}

			direction_ = nextDirection_;
			targetGridPos_ = nextGrid;
			isMoving_ = true;
			moveTimer_ = 0.0f;
			startPos_ = { (gridPos_.x + 0.5f) * unitLength, (gridPos_.y + 0.5f) * unitLength, 0.0f };
			endPos_ = { (targetGridPos_.x + 0.5f) * unitLength, (targetGridPos_.y + 0.5f) * unitLength, 0.0f };
			if (direction_.x != 0.0f || direction_.y != 0.0f)
			{
				worldTransform_.rotation_.y = std::atan2(direction_.x, direction_.y);
			}
		}
	}

	// 補間移動
	if (isMoving_)
	{
		moveTimer_ += deltaTime_;
		float t = (std::min)(moveTimer_ / kMoveDuration, 1.0f);
		Vector3 interpPos = startPos_ * (1.0f - t) + endPos_ * t;
		worldTransform_.translation_ = interpPos;

		// 【修正点】追従のために必須：頭の履歴を随時追加（補間座標を含む）
		if (headHistory_.empty() || headHistory_.back() != worldTransform_.translation_)
		{
			headHistory_.push_back(worldTransform_.translation_);
			headRotationHistory_.push_back(worldTransform_.rotation_);
		}

		// 【修正点】追従のために必須：履歴が長すぎる場合は古いものを削除
		size_t requiredHistory = kFollowDelay * bodyParts_.size();
		while (headHistory_.size() > requiredHistory)
		{
			headHistory_.pop_front();
		}
		while (headRotationHistory_.size() > requiredHistory)
		{
			headRotationHistory_.pop_front();
		}

		// 各パーツを遅延追従させる（位置のみ。回転はTransform更新時に設定）
		for (size_t i = 0; i < bodyParts_.size(); ++i)
		{
			if (i == 0)
			{
				bodyParts_[0] = worldTransform_.translation_;
			}
			else
			{
				size_t historyIndex = headHistory_.size() - 1 - i * kFollowDelay;
				if (historyIndex < headHistory_.size())
				{
					bodyParts_[i] = headHistory_[historyIndex];
				}
			}
		}

		// 体パーツのTransform更新（回転も履歴で追従）
		for (size_t i = 0; i < bodyParts_.size(); ++i)
		{
			bodyPartTransforms_[i].translation_ = bodyParts_[i];
			bodyPartTransforms_[i].scale_ = worldTransform_.scale_;
			if (i == 0)
			{
				bodyPartTransforms_[i].rotation_ = worldTransform_.rotation_;
			}
			else
			{
				size_t historyIndex = headRotationHistory_.size() - 1 - i * kFollowDelay;
				if (headRotationHistory_.empty() || historyIndex >= headRotationHistory_.size())
				{
					bodyPartTransforms_[i].rotation_ = worldTransform_.rotation_;
				}
				else
				{
					bodyPartTransforms_[i].rotation_ = headRotationHistory_[historyIndex];
				}
			}
			bodyPartTransforms_[i].matWorld_ = MakeAffineMatrix(bodyPartTransforms_[i].scale_, bodyPartTransforms_[i].rotation_, bodyPartTransforms_[i].translation_);
			bodyPartTransforms_[i].TransferMatrix();
		}

		if (t >= 1.0f)
		{
			// 移動完了
			gridPos_ = targetGridPos_;
			isMoving_ = false;

			// 座標をグリッドの中心に確定スナップ
			worldTransform_.translation_.x = (gridPos_.x + 0.5f) * unitLength;
			worldTransform_.translation_.y = (gridPos_.y + 0.5f) * unitLength;

			// 履歴に最終確定位置と回転も追加し、長さを調整
			headHistory_.push_back(worldTransform_.translation_);
			headRotationHistory_.push_back(worldTransform_.rotation_);
			while (headHistory_.size() > requiredHistory) { headHistory_.pop_front(); }
			while (headRotationHistory_.size() > requiredHistory) { headRotationHistory_.pop_front(); }

			// bodyParts_[0]もスナップ後の座標に更新
			bodyParts_[0] = worldTransform_.translation_;

			// ここで自己衝突（頭 vs 体）を判定
			if (CheckSelfCollisionGrid()) {
				isAlive_ = false;
			}
			else if (CheckHeadCollisionWithDetachedWalls())
			{
				isAlive_ = false;
			}
		}
	}
	else
	{
		// 静止時も体パーツのTransformを更新（消失防止）
		for (size_t i = 0; i < bodyParts_.size(); ++i)
		{
			bodyPartTransforms_[i].translation_ = bodyParts_[i];
			bodyPartTransforms_[i].scale_ = worldTransform_.scale_;
			if (i == 0)
			{
				bodyPartTransforms_[i].rotation_ = worldTransform_.rotation_;
			}
			else
			{
				size_t historyIndex = headRotationHistory_.size() > 0 ? (headRotationHistory_.size() - 1 - i * kFollowDelay) : 0;
				if (headRotationHistory_.empty() || historyIndex >= headRotationHistory_.size())
				{
					bodyPartTransforms_[i].rotation_ = worldTransform_.rotation_;
				}
				else
				{
					bodyPartTransforms_[i].rotation_ = headRotationHistory_[historyIndex];
				}
			}
			bodyPartTransforms_[i].matWorld_ = MakeAffineMatrix(bodyPartTransforms_[i].scale_, bodyPartTransforms_[i].rotation_, bodyPartTransforms_[i].translation_);
			bodyPartTransforms_[i].TransferMatrix();
		}
	}

	UpdateBomb();
	UpdateAABB();

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();

	if (model_)
	{
		model_->SetTranslate(worldTransform_.translation_);

		model_->SetRotate(worldTransform_.rotation_);
		model_->SetScale(worldTransform_.scale_);
		// カメラが必要なら都度設定（不要なら削ってOK）
		model_->SetCamera(camera_);
		// すぐに GPU バッファ等に反映したい場合は Update を呼ぶ
		model_->Update();
	}
}

void Player::Draw()
{
	if (!isAlive_ || !model_)
	{
		return;
	}
	model_->Draw(); // 頭の描画

	// bodyModels_ は体パーツ、bodyParts_[1..]に対応
	size_t bodyCount = bodyModels_.size();
	for (size_t i = 0; i < bodyCount; ++i)
	{
		// bodyPartIsRed_ のインデックスは bodyParts_ に合わせる
		size_t partIndex = i + 1; // bodyParts_[0]は頭

		// 安全確認
		if (partIndex >= bodyPartIsRed_.size()) continue;

		if (bodyPartIsRed_[partIndex])
		{
			bodyModels_[i]->GetModel()->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f }); // 赤
		}
		else
		{
			bodyModels_[i]->GetModel()->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f }); // 白
		}

		// Transform適用
		bodyModels_[i]->SetTranslate(bodyPartTransforms_[partIndex].translation_);
		bodyModels_[i]->SetRotate(bodyPartTransforms_[partIndex].rotation_);
		bodyModels_[i]->SetScale(bodyPartTransforms_[partIndex].scale_);
		bodyModels_[i]->Update();
		bodyModels_[i]->Draw();
	}

	for (auto* wall : wallModels_)
	{
		wall->Draw();
	}
}


void Player::Grow()
{
	Vector3 newPartPos;
	size_t insertIndex = bodyParts_.size(); // 挿入位置（デフォルトは尻尾）

	// 爆弾進行中かつ赤い部分がある場合の挿入位置の調整 (ロジック維持)
	auto it = std::find(bodyPartIsRed_.begin(), bodyPartIsRed_.end(), true);
	if (it != bodyPartIsRed_.end())
	{
		size_t redHeadIndex = std::distance(bodyPartIsRed_.begin(), it);
		if (redHeadIndex > 0)
		{
			insertIndex = redHeadIndex; // 赤い部分の一番頭側の直前に挿入
		}
	}

	// -------------------------------------------------------------------
	// 1. 新しいパーツの初期座標を、頭の履歴から計算する
	// -------------------------------------------------------------------

	// 新しいパーツのインデックス（頭を0としたときのパーツ番号）
	size_t newPartIndex = insertIndex;

	// このパーツが参照するべき頭の履歴の遅延インデックス
	size_t historyDelayIndex = newPartIndex * kFollowDelay;

	if (headHistory_.size() > historyDelayIndex)
	{
		// 履歴から、新しいパーツが追従すべき最初の目標位置を取得
		// headHistory_の最新位置からhistoryDelayIndex分さかのぼる
		newPartPos = headHistory_[headHistory_.size() - 1 - historyDelayIndex];
	}
	else
	{
		// 履歴がまだ不足している場合（ゲーム開始直後など）
		// 既存の最も後ろのパーツから1グリッド分後ろの位置を使用する
		if (!bodyParts_.empty())
		{
			newPartPos = bodyParts_.back();
			// 現在向いている方向と逆方向に1グリッド分ずらす
			Vector3 dir = { -direction_.x, -direction_.y, 0.0f };
			newPartPos.x += dir.x * unitLength;
			newPartPos.y += dir.y * unitLength;
		}
		else
		{
			// ありえないが、念のため頭の現在座標
			newPartPos = worldTransform_.translation_;
		}
	}

	// -------------------------------------------------------------------
	// 2. 算出した初期座標をグリッドの中心に強制スナップする
	// 【瞬間移動回避のための重要ロジック】
	// -------------------------------------------------------------------
	{
		// ワールド座標からグリッドインデックスへ変換
		int gridX = static_cast<int>(std::round(newPartPos.x / unitLength - 0.5f));
		int gridY = static_cast<int>(std::round(newPartPos.y / unitLength - 0.5f));

		// グリッドの中心座標へスナップバック
		newPartPos = { (gridX + 0.5f) * unitLength, (gridY + 0.5f) * unitLength, newPartPos.z };
	}

	// -------------------------------------------------------------------
	// 3. 各配列に新しいパーツを挿入し、初期設定を行う
	// -------------------------------------------------------------------

	// bodyParts_（座標リスト）
	bodyParts_.insert(bodyParts_.begin() + insertIndex, newPartPos);

	// bodyPartTransforms_（Transformリスト）
	bodyPartTransforms_.emplace(bodyPartTransforms_.begin() + insertIndex);
	bodyPartTransforms_[insertIndex].scale_ = worldTransform_.scale_;
	// 回転は履歴から設定（なければ現在の回転）
	if (headRotationHistory_.size() > historyDelayIndex)
	{
		bodyPartTransforms_[insertIndex].rotation_ = headRotationHistory_[headRotationHistory_.size() - 1 - historyDelayIndex];
	}
	else
	{
		bodyPartTransforms_[insertIndex].rotation_ = worldTransform_.rotation_;
	}
	bodyPartTransforms_[insertIndex].translation_ = newPartPos;
	bodyPartTransforms_[insertIndex].matWorld_ = MakeAffineMatrix(
		bodyPartTransforms_[insertIndex].scale_, bodyPartTransforms_[insertIndex].rotation_,
		bodyPartTransforms_[insertIndex].translation_);
	bodyPartTransforms_[insertIndex].TransferMatrix();

	// bodyModels_（3Dモデルインスタンス）
	Object3d* bodyObj = new Object3d();
	bodyObj->Initialize(object3dCom_);
	Model* newModel = new Model(*model_->GetModel()); // モデルの複製
	newModel->SetTexture("Resources/body.png");

	bodyObj->SetModel(newModel);
	bodyModels_.insert(bodyModels_.begin() + (insertIndex - 1), bodyObj); // bodyModels_は頭を含まないので-1

	// bodyPartIsRed_（爆弾の赤化フラグ）
	bodyPartIsRed_.insert(bodyPartIsRed_.begin() + insertIndex, false);
}

void Player::RemoveLastPart()
{
	if (bodyParts_.size() > 1)
	{
		Vector3 removedPartPos = bodyParts_.back();

		int gridX =
			static_cast<int>(std::round(removedPartPos.x / unitLength - 0.5f));
		int gridY =
			static_cast<int>(std::round(removedPartPos.y / unitLength - 0.5f));
		Vector3 snappedPos = { (gridX + 0.5f) * unitLength,
							  (gridY + 0.5f) * unitLength, removedPartPos.z };

		// 壁Transform（★基準スケールを使用）
		wallTransforms_.emplace_back();
		wallTransforms_.back().translation_ = snappedPos;
		wallTransforms_.back().scale_ = baseScale_; // ← 修正
		// 一律の回転（向きを統一）
		wallTransforms_.back().rotation_ = { 0.0f, 0.0f, 0.0f };
		wallTransforms_.back().matWorld_ = MakeAffineMatrix(
			wallTransforms_.back().scale_, wallTransforms_.back().rotation_,
			wallTransforms_.back().translation_);
		wallTransforms_.back().TransferMatrix();

		// 壁オブジェクト（★基準スケールを使用）
		Object3d* wallObj = new Object3d();
		wallObj->Initialize(object3dCom_);
		Model* wallModel = new Model(*model_->GetModel());
		wallModel->SetTexture("Resources/body.png");
		wallObj->SetModel(wallModel);
		wallObj->SetTranslate(snappedPos);
		wallObj->SetScale(baseScale_); // ← 修正
		// 一律の回転（向きを統一）
		wallObj->SetRotate({ 0.0f, 0.0f, 0.0f });
		wallObj->SetCamera(camera_);
		wallObj->Update();
		wallModels_.push_back(wallObj);


		// パーティクル発生
		ParticleManager::GetInstance()->EmitBurst8(
			"placeBurst",      // グループ名
			snappedPos,        // 発生位置
			0.20f,             // 速度
			0.25f,             // パーティクルのサイズ
			0.40f              // 寿命(秒)
		);


		if (!bodyModels_.empty())
		{
			delete bodyModels_.back()->GetModel();
			delete bodyModels_.back();
			bodyModels_.pop_back();
		}


		bodyParts_.pop_back();
		bodyPartTransforms_.pop_back();
		bodyPartIsRed_.pop_back();
	}
}

void Player::UpdateAABB()
{
	// AABB 半径（縮小率）
	const float half = (unitLength * kAabbScale) / 2.0f;

	// 頭AABB
	{
		const Vector3& pos = GetPosition();
		headAABB_.min = { pos.x - half, pos.y - half, pos.z - half };
		headAABB_.max = { pos.x + half, pos.y + half, pos.z + half };
	}

	// 体AABB（bodyParts_[1..]に対応）
	if (bodyParts_.size() > 1)
	{
		bodyAABBs_.resize(bodyParts_.size() - 1);
		for (size_t i = 1; i < bodyParts_.size(); ++i)
		{
			const Vector3& p = bodyParts_[i];
			AABB aabb;
			aabb.min = { p.x - half, p.y - half, p.z - half };
			aabb.max = { p.x + half, p.y + half, p.z + half };
			bodyAABBs_[i - 1] = aabb;
		}
	}
	else
	{
		bodyAABBs_.clear();
	}

	// 後方互換: 旧メンバにもミラー（頭AABBを返す設計だったため）
	playerAABB = headAABB_;
}

void Player::EatBomb()
{
	if (bombActive_)
		return; // すでに爆弾進行中なら何もしない

	if (bodyParts_.size() <= 1)
	{
		// 頭だけの場合は即死
		isAlive_ = false;
		return;
	}

	// 体がある場合は爆弾進行開始
	bombActive_ = true;
	bombProgress_ = 0;
	bombStartIndex_ = static_cast<int>(bodyParts_.size()) - 1;
	bombTimer_ = 0.0f;
}

void Player::UpdateBomb()
{
	if (!bombActive_) return;

	bombTimer_ += 1.0f / 60.0f;

	if (bombTimer_ >= bombStepTime_)
	{
		bombTimer_ = 0.0f;

		if (bombProgress_ < bodyParts_.size())
		{
			// 尻尾側から赤くするインデックスを計算
			int redIndex = static_cast<int>(bodyParts_.size()) - 1 - bombProgress_;
			bodyPartIsRed_[redIndex] = true; // 赤化
			bombProgress_++;
		}

		if (bombProgress_ >= bodyParts_.size())
		{
			isAlive_ = false;
		}
	}
}

void Player::DetachBombParts()
{

    if (!bombActive_ || bombProgress_ == 0)
        return;

    for (int i = 0; i < bombProgress_ && bodyParts_.size() > 1; ++i)
    {
        Vector3 removedPartPos = bodyParts_.back();

        int gridX = static_cast<int>(std::round(removedPartPos.x / unitLength - 0.5f));
        int gridY = static_cast<int>(std::round(removedPartPos.y / unitLength - 0.5f));

        Vector3 wallPos = removedPartPos;
        if (mapChipField_)
        {
            wallPos = mapChipField_->GetMapChipPositionByIndex(gridX, gridY);
        }
        else
        {
            wallPos = { (gridX + 0.5f) * unitLength, (gridY + 0.5f) * unitLength, removedPartPos.z };
        }

        // 壁Transform（★基準スケール）
        wallTransforms_.emplace_back();
        wallTransforms_.back().translation_ = wallPos;
        wallTransforms_.back().scale_ = baseScale_; // ← 修正
        // 一律の回転（向きを統一）
        wallTransforms_.back().rotation_ = { 0.0f, 0.0f, 0.0f };
        wallTransforms_.back().matWorld_ = MakeAffineMatrix(
            wallTransforms_.back().scale_, wallTransforms_.back().rotation_,
            wallTransforms_.back().translation_);
        wallTransforms_.back().TransferMatrix();

        // 壁オブジェクト（★基準スケール）
        Object3d* wallObj = new Object3d();
        wallObj->Initialize(object3dCom_);
        Model* wallModel = new Model(*model_->GetModel());
        wallModel->SetTexture("Resources/body.png");
        wallObj->SetModel(wallModel);
        wallObj->SetTranslate(wallPos);
        wallObj->SetScale(baseScale_); // ← 修正
        // 一律の回転（向きを統一）
        wallObj->SetRotate({ 0.0f, 0.0f, 0.0f });
        wallObj->SetCamera(camera_);
        wallObj->Update();
        wallModels_.push_back(wallObj);
     

	  ParticleManager::GetInstance()->EmitBurst8(
		  "placeBurst",      // グループ名
		  wallPos,        // 発生位置
		  0.20f,             // 速度
		  0.25f,             // パーティクルのサイズ
		  0.40f              // 寿命(秒)
	  );

      
        bodyParts_.pop_back();
        bodyPartTransforms_.pop_back();
        bodyPartIsRed_.pop_back();
    }

    bombActive_ = false;
    bombProgress_ = 0;

}

bool Player::IsOccupyingGrid(int gridX, int gridY) const
{
	// ヘルパー関数を使って占有グリッドをチェック
	for (const auto& occupiedGrid : GetOccupiedGridPositions())
	{
		if (occupiedGrid.first == gridX && occupiedGrid.second == gridY)
		{
			return true;
		}
	}
	return false;
}

std::vector<std::pair<int, int>> Player::GetOccupiedGridPositions() const
{
	std::vector<std::pair<int, int>> occupied;

	// ワールド座標 -> グリッド座標 変換ラムダ
	auto posToGrid = [&](const Vector3& p) -> std::pair<int, int>
		{
			int gx = static_cast<int>(std::round(p.x / unitLength - 0.5f));
			int gy = static_cast<int>(std::round(p.y / unitLength - 0.5f));
			return { gx, gy };
		};

	// プレイヤーの頭の現在グリッド位置
	occupied.push_back(posToGrid(worldTransform_.translation_));

	// プレイヤーの体のグリッド位置
	for (size_t i = 1; i < bodyParts_.size(); ++i)
	{
		// bodyParts_[i] は Update() で更新された補間座標
		Vector3 currentPos = bodyParts_[i];

		// 補間座標をグリッドにスナップして占有位置を判定
		occupied.push_back(posToGrid(currentPos));
	}

	// 設置済みの壁の位置
	for (const auto& wt : wallTransforms_)
	{
		occupied.push_back(posToGrid(wt.translation_));
	}

	// 重複を削除して返す（念のため）
	std::sort(occupied.begin(), occupied.end());
	occupied.erase(std::unique(occupied.begin(), occupied.end()), occupied.end());

	return occupied;
}

std::vector<std::pair<int, int>> Player::GetWallGridPositions() const
{
	std::vector<std::pair<int, int>> out;
	out.reserve(wallTransforms_.size() + wallPositions_.size());

	auto worldToGrid = [&](const Vector3& p) -> std::pair<int, int>
		{
			// MapChipField と同一の基準: ブロック中心が (n + 0.5) * unitLength になる配置
			const float bw = unitLength; // = 2.0f
			int gxBottom = static_cast<int>(std::round(p.x / bw - 0.5f));
			int gyBottom = static_cast<int>(std::round(p.y / bw - 0.5f));
			return { gxBottom, gyBottom }; // 上下の反転はGameScene側でMapに合わせて扱う
		};

	// Transform から拾う
	for (const auto& wt : wallTransforms_)
	{
		out.emplace_back(worldToGrid(wt.translation_));
	}
	// 直接の位置保持があればそれも拾う
	for (const auto& pos : wallPositions_)
	{
		out.emplace_back(worldToGrid(pos));
	}

	return out;
}


Vector3 Player::SetPosition(const Vector3& position)
{
	// ワールド位置
	worldTransform_.translation_ = position;

	// 描画オブジェクトも同期
	if (model_)
	{
		model_->SetTranslate(position);
	}

	// 行列更新
	worldTransform_.TransferMatrix();

	// グリッド座標・移動状態の初期化
	// マスの一辺長 unitLength (=2.0f) 基準で丸め
	gridPos_.x = std::round(position.x / unitLength - 0.5f);
	gridPos_.y = std::round(position.y / unitLength - 0.5f);
	targetGridPos_ = gridPos_;
	startPos_ = position;
	endPos_ = position;
	isMoving_ = false;
	moveTimer_ = 0.0f;
	velocity_ = { 0.0f, 0.0f, 0.0f };

	// 頭の履歴もリセットして現在位置から開始
	headHistory_.clear();
	headHistory_.push_back(position);
	// 回転履歴もリセット
	headRotationHistory_.clear();
	headRotationHistory_.push_back(worldTransform_.rotation_);

	// AABB 更新
	UpdateAABB();

	return worldTransform_.translation_;
}


bool Player::IsHeadCollidingWith(const AABB& other) const
{
	return IsCollisionAABBAABB(headAABB_, other);
}

bool Player::IsBodyCollidingWith(const AABB& other) const
{
	for (const auto& aabb : bodyAABBs_)
	{
		if (IsCollisionAABBAABB(aabb, other)) return true;
	}
	return false;
}

// 末尾付近に実装を追加
bool Player::CheckSelfCollisionGrid() const
{
	// 頭のグリッド
	const int hx = static_cast<int>(std::round(worldTransform_.translation_.x / unitLength - 0.5f));
	const int hy = static_cast<int>(std::round(worldTransform_.translation_.y / unitLength - 0.5f));

	// 体パーツ（1..）の各グリッドと照合
	for (size_t i = 1; i < bodyParts_.size(); ++i)
	{
		const Vector3& p = bodyParts_[i];
		const int gx = static_cast<int>(std::round(p.x / unitLength - 0.5f));
		const int gy = static_cast<int>(std::round(p.y / unitLength - 0.5f));
		if (hx == gx && hy == gy)
		{
			return true;
		}
	}
	return false;
}

bool Player::CheckHeadCollisionWithDetachedWalls() const
{
	// 頭のグリッド
	const int hx = static_cast<int>(std::round(worldTransform_.translation_.x / unitLength - 0.5f));
	const int hy = static_cast<int>(std::round(worldTransform_.translation_.y / unitLength - 0.5f));

	auto posToGrid = [&](const Vector3& p) -> std::pair<int, int>
		{
			int gx = static_cast<int>(std::round(p.x / unitLength - 0.5f));
			int gy = static_cast<int>(std::round(p.y / unitLength - 0.5f));
			return { gx, gy };
		};

	// Transform 由来の壁
	for (const auto& wt : wallTransforms_)
	{
		auto [gx, gy] = posToGrid(wt.translation_);
		if (hx == gx && hy == gy) { return true; }
	}
	// 位置配列に残っている分
	for (const auto& p : wallPositions_)
	{
		auto [gx, gy] = posToGrid(p);
		if (hx == gx && hy == gy) { return true; }
	}
	return false;
}
