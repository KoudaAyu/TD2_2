#include "Player.h"
#include<algorithm>
#include<cassert>
#include "Controller.h"
#include "ParticleManager.h"
#include "Sprite.h"
#include "SpriteCom.h"
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif
Player::~Player()
{
	keyInput_ = nullptr;
	model_ = nullptr;
	camera_ = nullptr;
	object3dCom_ = nullptr;

	for (auto b : barriers_)
	{
		if (b) delete b;
		b = nullptr;
	}
	barriers_.clear();

	if (controller_)
	{
		delete controller_;
		controller_ = nullptr;
	}

	if (invincibleSprite_) { delete invincibleSprite_; invincibleSprite_ = nullptr; }
}

void Player::SetSpriteCom(SpriteCom* spriteCom, const std::string& texturePath)
{
	spriteCom_ = spriteCom;
	invincibleTexturePath_ = texturePath;
	// create sprite if texture available
	if (spriteCom_ && !invincibleTexturePath_.empty()) {
		int screenW = 1280; int screenH = 720;
		if (object3dCom_ && object3dCom_->GetDirectXCom()) {
			screenW = object3dCom_->GetDirectXCom()->GetClientWidth();
			screenH = object3dCom_->GetDirectXCom()->GetClientHeight();
		}
		invincibleSprite_ = spriteCom_->CreateSprite(invincibleTexturePath_, { static_cast<float>(screenW)/2.0f, static_cast<float>(screenH)/2.0f }, { 128.0f, 128.0f }, 0.0f, {0.5f,0.5f});
		if (invincibleSprite_) {
			Vector4 c = invincibleSprite_->GetColor();
			c.w = 0.0f; // hidden by default
			invincibleSprite_->SetColor(c);
			invincibleSprite_->Update();
		}
	}
}

void Player::Initialize(Object3d* model, Camera* camera, const Vector3 pos, Object3dCom* object3dCom)
{
	// モデル/カメラは外部で生成される想定。nullptr許可にして安全に扱う
	model_ = model;
	camera_ = camera;
	object3dCom_ = object3dCom;

	keyInput_ = KeyInput::GetInstance();
	assert(keyInput_ && "KeyInput::Initialize() が呼ばれていません。mainで生成&初期化してください。");

	// 初期Transform（スケール0で不可視にならないように 1 を設定）
	worldTransform_.Initialize();
	worldTransform_.SetTranslate(pos);

	if (model_)
	{
		model_->ApplyState(worldTransform_, camera_, true);
	}

	// コントローラー初期化（左スティックを移動に使用）
	controller_ = new Controller(0);

	soundManager_ = SoundManager::GetInstance();

	shotBarrierSoundData_ = soundManager_->SoundLoadWave("Resources/Audio/SE/Shot.wav");

	// ensure invincibility starts off
	invincible_ = false;
	invincibleTimer_ = 0.0f;

	// reset hit counter
	hitCount_ = 0;

	// create default invincible sprite if possible (deferred until GameScene sets spriteCom)
}

void Player::Update()
{
	// reset per-frame became-invincible flag at start of Update
	becameInvincibleThisFrame_ = false;
	// advance invincibility age frame counter if active
	if (invincible_)
	{
		if (invincibleAgeFrames_ >= 0) invincibleAgeFrames_++;
	}

	//Playerが死亡している場合早期return
	if (!isAlive_)
	{
		return;
	}

	// コントローラーの状態を更新
	if (controller_)
	{
		controller_->Update();
	}

	Move();

	MoveLimit();

	// 回転処理を追加
	Rotate();

	// モデルを上下移動に応じて傾ける（ピッチ）
	{
		// 垂直入力の取得（キーボード+コントローラー）
		float verticalInput = 0.0f;
		float horizontalInput = 0.0f;
		if (keyInput_->PushKey(DIK_W)) verticalInput += 1.0f;
		if (keyInput_->PushKey(DIK_S)) verticalInput -= 1.0f;
		if (keyInput_->PushKey(DIK_A)) horizontalInput -= 1.0f;
		if (keyInput_->PushKey(DIK_D)) horizontalInput += 1.0f;
		if (controller_ && controller_->IsConnected())
		{
			Controller::Stick ls = controller_->GetLeftStick();
			verticalInput += ls.y; // 左スティックのYを加算
			horizontalInput += ls.x; // 左スティックのXを加算
		}
		// clamp
		verticalInput = std::clamp(verticalInput, -1.0f, 1.0f);
		horizontalInput = std::clamp(horizontalInput, -1.0f, 1.0f);

		// 目標ピッチ角度（上移動で少し上に傾ける）。負号は向き合わせの調整
		float targetPitch = -verticalInput * kMaxTiltAngle;
		// 目標ロール角度（右移動で右へ傾ける）
		// 横入力の符号を反転して左右の傾きを修正
		float targetRoll = -horizontalInput * kMaxTiltAngle;

		// 現在の回転を取得して滑らかに補間（ピッチ=X, ヨー=Y, ロール=Z）
		Vector3 rot = worldTransform_.GetRotate();
		rot.x += (targetPitch - rot.x) * kTiltSmoothing;
		rot.z += (targetRoll - rot.z) * kTiltSmoothing;
		worldTransform_.SetRotate(rot);
	}

	Barrier();

	// ワールド行列の更新（必要なら維持）
	worldTransform_.TransferMatrix();

	// Object3d 側へ反映し、即時Updateはせずこの後にUpdateを呼ぶ
	if (model_)
	{
		model_->ApplyState(worldTransform_, camera_, false);
		model_->Update();
	}

	// Update barriers and remove inactive
	for (auto it = barriers_.begin(); it != barriers_.end();) {
		PlayerBarrier* b = *it;
		if (b) {
			b->Update();
			if (!b->IsActive()) {
				delete b;
				it = barriers_.erase(it);
				continue;
			}
		}
		++it;
	}

	// update controller vibration timer and apply vibration if needed
	if (controllerVibrationTimer_ > 0.0f)
	{
		controllerVibrationTimer_ -= 1.0f / 60.0f; // assume 60fps
		if (controllerVibrationTimer_ <= 0.0f)
		{
			// stop vibration
			if (controller_ && controller_->IsConnected())
			{
				controller_->SetVibration(0.0f, 0.0f);
			}
			controllerVibrationTimer_ = 0.0f;
		}
		else
		{
			// maintain vibration
			if (controller_ && controller_->IsConnected())
			{
				controller_->SetVibration(kCollisionVibrationAmplitude, kCollisionVibrationAmplitude);
			}
		}
	}

	// invincibility timer update
	if (invincible_)
	{
		invincibleTimer_ -= 1.0f / 60.0f;
		if (invincibleTimer_ <= 0.0f)
		{
			invincible_ = false;
			invincibleTimer_ = 0.0f;
			// ensure model alpha restored
			if (model_ && model_->GetModel()) {
				Vector4 col = {1.0f,1.0f,1.0f,1.0f};
				model_->SetColor(col);
				model_->GetModel()->SetColor(col);
			}
			// keep invincible HUD sprite hidden
			if (invincibleSprite_) {
				Vector4 c = invincibleSprite_->GetColor();
				c.w = 0.0f;
				invincibleSprite_->SetColor(c);
				invincibleSprite_->Update();
			}
		}
		else
		{
			// blinking visual effect on model only
			float phase = fmodf(invincibleTimer_, kInvincibleBlinkPeriod) / kInvincibleBlinkPeriod;
			float alpha = (phase < 0.5f) ? 0.25f : 1.0f;
			if (model_) {
				Vector4 c = {1.0f,1.0f,1.0f,alpha};
				model_->SetColor(c);
				if (model_->GetModel()) model_->GetModel()->SetColor(c);
			}
			// do NOT show or update invincibleSprite here; temporary feedback sprites are created by GameScene
		}
	}
}

void Player::Draw()
{
	for (auto b : barriers_)
	{
		if (b) b->Draw(*camera_);
	}

	if (model_)
	{
		model_->Draw();
	}

	// do not draw persistent invincible HUD sprite here; GameScene spawns temporary HUD sprites when appropriate
	// if (invincibleSprite_) { invincibleSprite_->Draw(); }
}

void Player::ClearBarriers()
{
	for (auto b : barriers_)
	{
		if (b) delete b;
	}
	barriers_.clear();
}

void Player::Move()
{
	// prevent movement if disabled (e.g., during fade)
	if (!canMove_) return;

	Vector3 move = { 0.0f,0.0f,0.0f };
	const float kCharacterSpeed = 0.2f;

	// キーボード入力
	if (keyInput_->PushKey(DIK_A))
	{
		move.x -= kCharacterSpeed;
	}

	if (keyInput_->PushKey(DIK_D))
	{
		move.x += kCharacterSpeed;
	}

	if (keyInput_->PushKey(DIK_W))
	{
		move.y += kCharacterSpeed;
	}
	if (keyInput_->PushKey(DIK_S))
	{
		move.y -= kCharacterSpeed;
	}

	// コントローラー左スティックによる移動（キーボードと併用）
	if (controller_ && controller_->IsConnected())
	{
		Controller::Stick ls = controller_->GetLeftStick();
		// 左スティックのX軸は左右、Y軸は前後に対応（キーボードのW/Sと同じ符号）
		move.x += ls.x * kCharacterSpeed;
		move.y += ls.y * kCharacterSpeed;
	}

	// 移動を適用（X/Yはmoveベクトルで扱う）
	worldTransform_.SetTranslate(worldTransform_.GetTranslate() + move);
}

void Player::MoveLimit()
{
	// 位置を取得し、範囲を超えないように clamp して戻す
	Vector3 t = worldTransform_.GetTranslate();
	t.x = std::clamp(t.x, -kMoveLimitX, kMoveLimitX);
	t.y = std::clamp(t.y, -kMoveLimitY, kMoveLimitY);
	worldTransform_.SetTranslate(t);
}

void Player::Rotate()
{
	const float kRotSpeed = 0.02f;

	// 現在の回転を取得
	Vector3 rot = worldTransform_.GetRotate();

	// 方向キー左でY軸回転を減算（左回転）


	// 方向キー右でY軸回転を加算（右回転）


	worldTransform_.SetRotate(rot);
}

void Player::Barrier()
{
	// respect canFire_ flag
	if (!canFire_) return;

	// 静的ローカルでクールダウン管理（メンバを増やさず対応）
	static int s_fireCooldownFrames = 0;
	static const int kBaseInterval = 12;   // 基準間隔
	static const int kMinInterval  = 5;    // 最短間隔

	if (s_fireCooldownFrames > 0) {
		--s_fireCooldownFrames;
	}

	// 入力状態で連射間隔を調整: 縦移動継続で徐々に短縮、横移動でリセット
	bool verticalHeld = keyInput_->PushKey(DIK_W) || keyInput_->PushKey(DIK_S);
	bool horizontalHeld = keyInput_->PushKey(DIK_A) || keyInput_->PushKey(DIK_D);
	if (controller_ && controller_->IsConnected()) {
		Controller::Stick ls = controller_->GetLeftStick();
		verticalHeld   = verticalHeld   || (std::abs(ls.y) > 0.3f);
		horizontalHeld = horizontalHeld || (std::abs(ls.x) > 0.3f);
	}

	static float s_verticalHeldTime = 0.0f; // 秒
	if (verticalHeld) {
		s_verticalHeldTime += (1.0f/60.0f);
	}
	if (horizontalHeld || !verticalHeld) {
		// 横入力が入ったら即解除、縦入力が途切れてもしばらくの蓄積はリセット
		s_verticalHeldTime = 0.0f;
	}

	// 経過時間に応じて段階的に短縮（わかりやすい階段式）
	// 0.0s~: 12f, 0.5s~: 10f, 1.0s~: 8f, 1.5s~: 6f, 2.0s~: 5f
	int dynamicInterval = kBaseInterval;
	if (s_verticalHeldTime >= 2.0f)      dynamicInterval = 5;
	else if (s_verticalHeldTime >= 1.5f) dynamicInterval = 6;
	else if (s_verticalHeldTime >= 1.0f) dynamicInterval = 8;
	else if (s_verticalHeldTime >= 0.5f) dynamicInterval = 10;

	// 発射入力: トリガー or 長押し
	bool fireTriggered = keyInput_->TriggerKey(DIK_LCONTROL) || keyInput_->TriggerKey(DIK_SPACE);
	bool fireHeld = keyInput_->PushKey(DIK_LCONTROL) || keyInput_->PushKey(DIK_SPACE);
	// コントローラの A ボタン
	if (controller_ && controller_->IsConnected())
	{
		if (controller_->WasButtonPressedThisFrame(XINPUT_GAMEPAD_A))
		{
			fireTriggered = true;
		}
		if (controller_->IsButtonDown(XINPUT_GAMEPAD_A))
		{
			fireHeld = true;
		}
	}

	// 連打優遇を排除: トリガーでもクールダウンを満たしていないと発射しない
	bool shouldFire = false;
	bool anyFireInput = fireTriggered || fireHeld;
	if (anyFireInput && s_fireCooldownFrames <= 0) {
		shouldFire = true;
	}

	if (shouldFire)
	{
		const float kBarrierSpeed = 0.5f;

		Vector3 velocity;

		soundManager_->SoundPlayWave(shotBarrierSoundData_, false, 0.2f);

		// カメラの前方向に真っ直ぐ飛ぶように設定（レールシューティング風）
		if (camera_)
		{
			const Matrix4x4& camWorld = camera_->GetWorldMatrix();
			Vector3 camForward = { camWorld.m[0][2], camWorld.m[1][2], camWorld.m[2][2] };
			camForward.y = 0.0f;
			Vector3 dir = Normalize(camForward);
			if (Length(dir) <= 1e-6f)
			{
				dir = { 0.0f, 0.0f, 1.0f };
			}
			velocity = { dir.x * kBarrierSpeed, dir.y * kBarrierSpeed, dir.z * kBarrierSpeed };
		}
		else
		{
			velocity = { 0.0f, 0.0f, kBarrierSpeed };
		}

		PlayerBarrier* barrier = new PlayerBarrier();
		barrier->Initialize(model_, worldTransform_.GetTranslate(), object3dCom_, velocity);
		barrier->SetBirthWave(currentWaveForBarriers_);
		barriers_.push_back(barrier);

		// クールダウンリセット（動的間隔を使用）
		s_fireCooldownFrames = dynamicInterval;
	}
}

void Player::OnCollision()
{
	// if currently invincible, ignore
	if (invincible_) return;

	// increment hit counter and check death
	++hitCount_;
	if (hitCount_ >= kMaxHits)
	{
		isAlive_ = false;
		// trigger death visual/effects similar to previous behavior
		auto* pm = ParticleManager::GetInstance();
		if (pm)
		{
			Vector3 emitPos = worldTransform_.GetTranslate();
			emitPos.z += 0.5f; // 少し手前に出す
			pm->EmitBurst8("defaultMesh", emitPos, 0.12f, 0.25f, 0.8f);
		}

		if (camera_)
		{
			camera_->StartShake(kCollisionShakeAmplitude, kCollisionShakeDuration);
		}

		// stop here; don't start invincibility for death
		return;
	}

	// start invincibility instead of immediate death
	invincible_ = true;
	invincibleTimer_ = kInvincibleDuration;
	becameInvincibleThisFrame_ = true; // mark that invincibility began this frame
	invincibleAgeFrames_ = 0; // age 0 indicates started this frame

	// 小さなメッシュ(OBJ)パーティクルエフェクトを追加
	auto* pm = ParticleManager::GetInstance();
	if (pm)
	{
		Vector3 emitPos = worldTransform_.GetTranslate();
		emitPos.z += 0.5f; // 少し手前に出す
		// OBJベースの8方向バーストを生成（メッシュグループを使用）
		pm->EmitBurst8("defaultMesh", emitPos, 0.12f, 0.25f, 0.8f);

	}

	// remove sprite creation here: sprite should be shown only when an enemy attack hits while already invincible (handled in GameScene)

	// コントローラ振動を開始
	if (controller_ && controller_->IsConnected())
	{
		controllerVibrationTimer_ = kCollisionVibrationDuration;
		controller_->SetVibration(kCollisionVibrationAmplitude, kCollisionVibrationAmplitude);
	}

	// カメラがある場合は衝突時にカメラを揺らす
	if (camera_)
	{
		camera_->StartShake(kCollisionShakeAmplitude, kCollisionShakeDuration);
	}
}

#ifdef USE_IMGUI
void Player::DrawImGui()
{
	if (!ImGui::Begin("Player"))
	{
	 ImGui::End();
	 return;
	}

	// 基本情報表示
	ImGui::Text("Alive: %s", isAlive_ ? "true" : "false");
	ImGui::Text("Invincible: %s", invincible_ ? "true" : "false");

	Vector3 translate = worldTransform_.GetTranslate();
	if (ImGui::DragFloat3("Translate", &translate.x, 0.1f))
	{
		worldTransform_.SetTranslate(translate);
		worldTransform_.TransferMatrix();
		if (model_)
		{
			model_->ApplyState(worldTransform_, camera_, true);
		}
	}

	ImGui::DragFloat("Invincible Timer", &invincibleTimer_, 0.01f, 0.0f, 5.0f);

	ImGui::End();
}
#endif







