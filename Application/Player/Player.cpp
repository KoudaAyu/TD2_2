#include "Player.h"
#include<algorithm>
#include<cassert>
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

	// 初期ジャンプ状態
	verticalVelocity_ = 0.0f;
	jumpCount_ = 0;
}



void Player::Update()
{
	//Playerが死亡している場合早期return
	if (!isAlive_)
	{
		return;
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

	// ジャンプ処理 (Space: トリガーでジャンプ／二段ジャンプ対応)
	if (keyInput_->TriggerKey(DIK_SPACE))
	{
		if (jumpCount_ < kMaxJumpCount)
		{
			// 一段目と二段目で初速を変える
			if (jumpCount_ == 0)
			{
				verticalVelocity_ = kFirstJumpVelocity;
			}
			else
			{
				verticalVelocity_ = kSecondJumpVelocity;
			}
			jumpCount_++;
		}
	}

	// 重力を適用
	verticalVelocity_ += kGravity;
	move.y += verticalVelocity_;

	// 移動を適用（X/Yはmoveベクトルで扱う）
	worldTransform_.SetTranslate(worldTransform_.GetTranslate() + move);

	// 地面判定: Yが下限に達したら着地扱いにする（ここでは -kMoveLimitY を地面とする）
	Vector3 pos = worldTransform_.GetTranslate();
	if (pos.y <= -kMoveLimitY)
	{
		pos.y = -kMoveLimitY;
		worldTransform_.SetTranslate(pos);
		verticalVelocity_ = 0.0f;
		jumpCount_ = 0; // 着地でジャンプ回数リセット
	}
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
	if (keyInput_->TriggerKey(DIK_LCONTROL))
	{
		const float kBarrierSpeed = 0.5f;

		Vector3 velocity({ 0.0f,0.0f,kBarrierSpeed });

		//速度ベクトルを自機の回転に合わせて回転させる
		worldTransform_.TransferMatrix();
		velocity = TransformNormal(velocity, worldTransform_.GetWorldMatrix());

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



