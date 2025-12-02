#include "Player.h"
#include<algorithm>
#include<cassert>
#include "../../Baziru3_Engine/IO/XBox/Controller.h"
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

void Player::Move()
{
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
	if (keyInput_->PushKey(DIK_LEFT))
	{
		rot.y -= kRotSpeed;
	}

	// 方向キー右でY軸回転を加算（右回転）
	if (keyInput_->PushKey(DIK_RIGHT))
	{
		rot.y += kRotSpeed;
	}

	worldTransform_.SetRotate(rot);
}

void Player::Barrier()
{
	// バリア発射キーを変更: 例として LEFT CONTROL を使用（Triggerで発射）

	bool fireTriggered = keyInput_->TriggerKey(DIK_LCONTROL);
	// コントローラの B ボタンでも発射可能にする
	if (!fireTriggered && controller_ && controller_->IsConnected()) {
		if (controller_->WasButtonPressedThisFrame(XINPUT_GAMEPAD_B)) {
			fireTriggered = true;
		}
	}

	// キーボードの '1' キーでも発射できるようにする
	if (fireTriggered || keyInput_->TriggerKey(DIK_SPACE))

	{
		const float kBarrierSpeed = 0.5f;

		Vector3 velocity;

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
			if (Length(dir) <= 1e-6f) {
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

		// 新しいバリアを配列に追加
		barriers_.push_back(barrier);
	}
}

void Player::OnCollision()
{
#ifndef _DEBUG
	isAlive_ = false;
#endif

	// 小さなメッシュ(OBJ)パーティクルエフェクトを追加
	auto* pm = ParticleManager::GetInstance();
	if (pm)
	{
		Vector3 emitPos = worldTransform_.GetTranslate();
		emitPos.z += 0.5f; // 少し手前に出す
		// OBJベースの8方向バーストを生成（メッシュグループを使用）
		pm->EmitBurst8("defaultMesh", emitPos, 0.12f, 0.25f, 0.8f);
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

	ImGui::End();
}
#endif






