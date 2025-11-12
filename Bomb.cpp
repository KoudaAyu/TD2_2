#include "Bomb.h"
#include "MapChipField.h"
#include "Player.h"
#include <random>
#include <vector>
#include <utility>

Bomb::~Bomb()
{
	delete model_;
	model_ = nullptr;
}

void Bomb::Initialize(Object3d* model, Camera* camera, const Vector3 pos)
{
	model_ = model;
	camera_ = camera;
	worldTransform_.translation_ = pos;
}

void Bomb::Update()
{
	UpdateAABB();

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();

	model_->SetTranslate(worldTransform_.translation_);
	model_->Update();
}

void Bomb::Draw()
{
	model_->Draw();
}

void Bomb::UpdateAABB()
{
	constexpr float scale = 0.5f; // ← ここで縮小率を調整
	float half = (1.0f * scale) / 2.0f;
	const Vector3& pos = GetPosition();
	bombAABB.min = { pos.x - half, pos.y - half, pos.z - half };
	bombAABB.max = { pos.x + half, pos.y + half, pos.z + half };
}

void Bomb::Respawn(MapChipField* map, const Player& player)
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

	// フォールバック: 候補0ならプレイヤー占有だけ外して探す
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

	std::mt19937 rng{ std::random_device{}() };
	std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
	auto [gx, gy] = candidates[dist(rng)];

	// ワールド座標へ反映
	worldTransform_.translation_ = map->GetMapChipPositionByIndex(static_cast<uint32_t>(gx), static_cast<uint32_t>(gy));
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();

	isAlive_ = true;
}