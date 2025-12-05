#include "Player.h"
#include<algorithm>
#include<cassert>
#include "Controller.h"
#include "ParticleManager.h"
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

}



void Player::Update()
{
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
	for (auto it = barriers_.begin(); it != barriers_.end();)
	{
		PlayerBarrier* b = *it;
		if (b)
		{
			b->Update();
			if (!b->IsActive())
			{
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
		}
		else
		{
			// blinking visual effect
			float phase = fmodf(invincibleTimer_, kInvincibleBlinkPeriod) / kInvincibleBlinkPeriod;
			float alpha = (phase < 0.5f) ? 0.25f : 1.0f;
			if (model_) {
				Vector4 c = {1.0f,1.0f,1.0f,alpha};
				model_->SetColor(c);
				if (model_->GetModel()) model_->GetModel()->SetColor(c);
			}
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
	/*if (keyInput_->PushKey(DIK_LEFT))
	{
		rot.y -= kRotSpeed;
	}*/

	// 方向キー右でY軸回転を加算（右回転）
	/*if (keyInput_->PushKey(DIK_RIGHT))
	{
		rot.y += kRotSpeed;
	}*/

	worldTransform_.SetRotate(rot);
}

void Player::Barrier()
{
	// respect canFire_ flag
	if (!canFire_) return;



	// バリア発射キーを変更: 例として LEFT CONTROL を使用（Triggerで発射）

	bool fireTriggered = keyInput_->TriggerKey(DIK_LCONTROL);
	// コントローラの A ボタンでも発射可能にする
	if (!fireTriggered && controller_ && controller_->IsConnected())
	{
		if (controller_->WasButtonPressedThisFrame(XINPUT_GAMEPAD_A))
		{
			fireTriggered = true;
			soundManager_->SoundPlayWave(shotBarrierSoundData_, false, 0.2f);
		}
	}

	// キーボードの 'SPACE' キーでも発射できるようにする
	if (fireTriggered || keyInput_->TriggerKey(DIK_SPACE))

	{
		const float kBarrierSpeed = 0.5f;

		Vector3 velocity;

		soundManager_->SoundPlayWave(shotBarrierSoundData_, false, 0.2f);

		// カメラの前方向に真っ直ぐ飛ぶように設定（レールシューティング風）
		if (camera_)
		{
			const Matrix4x4& camWorld = camera_->GetWorldMatrix();
			// camWorld の列 2 を前方向ベクトルとして利用
			Vector3 camForward = { camWorld.m[0][2], camWorld.m[1][2], camWorld.m[2][2] };
			// 垂直成分を取り除いて真っ直ぐ飛ぶようにする
			camForward.y = 0.0f;
			// 正規化
			Vector3 dir = Normalize(camForward);
			// 長さが0に近ければフォールバック
			if (Length(dir) <= 1e-6f)
			{
				dir = { 0.0f, 0.0f, 1.0f };
			}
			velocity = { dir.x * kBarrierSpeed, dir.y * kBarrierSpeed, dir.z * kBarrierSpeed };
		}
		else
		{
			// フォールバック: z正方向
			velocity = { 0.0f, 0.0f, kBarrierSpeed };
		}

		PlayerBarrier* barrier = new PlayerBarrier();
		barrier->Initialize(model_, worldTransform_.GetTranslate(), object3dCom_, velocity);

		// set birth wave so it won't hit next-wave enemies
		barrier->SetBirthWave(currentWaveForBarriers_);

		// 新しいバリアを配列に追加
		barriers_.push_back(barrier);
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

	// 小さなメッシュ(OBJ)パーティクルエフェクトを追加
	auto* pm = ParticleManager::GetInstance();
	if (pm)
	{
		Vector3 emitPos = worldTransform_.GetTranslate();
		emitPos.z += 0.5f; // 少し手前に出す
		// OBJベースの8方向バーストを生成（メッシュグループを使用）
		pm->EmitBurst8("defaultMesh", emitPos, 0.12f, 0.25f, 0.8f);
	}

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








