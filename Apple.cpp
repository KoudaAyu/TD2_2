#include "Apple.h"
#include"MapChipField.h"
#include"Player.h"
#include <random>
#include <vector>
#include <utility> 

Apple::~Apple()
{
	delete model_;
	model_ = nullptr;
}

void Apple::Initialize(Object3d* model, Camera* camera, const Vector3 pos)
{
	model_ = model;
	camera_ = camera;
#ifdef _DEBUG
	assert(model_);
	assert(camera_);
#endif

	// 安全な初期値を設定（スケールの未初期化によるフレーム描画崩れ防止）
	worldTransform_.scale_ = { 0.5f, 0.5f, 0.5f };
	worldTransform_.rotation_ = { 0.0f, 0.0f, 0.0f };
	worldTransform_.translation_ = pos;

	// Object3d にも即時反映
	model_->SetScale(worldTransform_.scale_);
	model_->SetTranslate(worldTransform_.translation_);
	model_->Update();
}

void Apple::Update()
{
	UpdateAABB();

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();

	// スケール・座標を常に同期（フレーム内の一瞬の拡大防止）
	//model_->SetScale(worldTransform_.scale_);
	model_->SetTranslate(worldTransform_.translation_);
	model_->Update();
}
void Apple::Draw()
{
	if (isAlive_)
	{
		model_->Draw();
	}
}

void Apple::Respawn(MapChipField* map, const Player& player)
{
	if (!map)
	{
		isAlive_ = false;
		return;
	}

	const int w = static_cast<int>(map->GetNumBlockHorizontal());
	const int h = static_cast<int>(map->GetNumBlockVirtical());

	std::vector<std::pair<int, int>> candidates;
	candidates.reserve(static_cast<size_t>(w) * static_cast<size_t>(h));

	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			// 壁は除外
			if (map->GetMapChipTypeByIndex(x, y) == MapChipType::kWall)
				continue;

			// 重要: y(上0) -> y(下0) に変換してから占有判定
			const int gyBottom = h - 1 - y;
			if (player.IsOccupyingGrid(x, gyBottom))
				continue;

			candidates.emplace_back(x, y);
		}
	}

	// すべて占有されている場合は、占有を無視して壁以外から選ぶ（従来のフォールバックを維持）
	if (candidates.empty())
	{
		for (int y = 0; y < h; ++y)
		{
			for (int x = 0; x < w; ++x)
			{
				if (map->GetMapChipTypeByIndex(x, y) == MapChipType::kWall)
					continue;
				candidates.emplace_back(x, y);
			}
		}
		if (candidates.empty())
		{
			isAlive_ = false;
			return;
		}
	}

	std::random_device rd;
	std::mt19937 rng(rd());
	std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
	auto [gx, gy] = candidates[dist(rng)];

	worldTransform_.translation_ = map->GetMapChipPositionByIndex(static_cast<uint32_t>(gx), static_cast<uint32_t>(gy));

	// 即時に見た目へ反映（1フレームの位置/スケールずれ防止）
	UpdateAABB();
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
	if (model_) {
		model_->SetScale(worldTransform_.scale_);
		model_->SetTranslate(worldTransform_.translation_);
		model_->Update();
	}

	isAlive_ = true;
}


void Apple::UpdateAABB()
{
	constexpr float scale = 0.5f; // ← ここで縮小率を調整
	float half = (1.0f * scale) / 2.0f;
	const Vector3& pos = GetPosition();
	appleAABB.min = { pos.x - half, pos.y - half, pos.z - half };
	appleAABB.max = { pos.x + half, pos.y + half, pos.z + half };
}