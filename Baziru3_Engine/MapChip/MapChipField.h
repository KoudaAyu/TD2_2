#pragma once
#include <string>
#include <vector>
#include "Vector.h"

enum class MapChipType
{
	kBlank,
	kWall,
	kPlayerSpawn,
	kAppleSpawn,
	kBombSpawn,
};

struct MapChipData
{
	std::vector<std::vector<MapChipType>> data;
};

class MapChipField
{
public:
	void Initialize();
	// リセット（現在のグリッドサイズで確保）
	void ResetMapChipData();
	// 読み込み（CSVのサイズに合わせて動的に確保・設定）
	void LoadmapChipCsv(const std::string& filePath);
	// 種類別取得
	MapChipType GetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex);
	// マップチップ座標の取得
	Vector3 GetMapChipPositionByIndex(uint32_t xIndex, uint32_t yIndex);

	// グリッドサイズ（シーン側で明示的に設定したい場合）
	void SetGridSize(uint32_t vertical, uint32_t horizontal);

	// 既存内容を可能な範囲で保持しつつ縦横サイズを変更する。vertical縦
	void ResizeKeepContent(uint32_t vertical, uint32_t horizontal, MapChipType fill = MapChipType::kBlank);

	const uint32_t GetNumBlockVirtical() const { return numBlockVertical_; }
	const uint32_t GetNumBlockHorizontal() const { return numBlockHorizontal_; }

	const float GetBlockWidth() const { return kBlockWidth; }
	const float GetBlockHeight() const { return kBlockHeight; }

	// 通行判定
	bool IsMovable(int gridX, int gridY) const
	{
		if (gridX < 0 || gridY < 0 || gridX >= fieldWidth_ || gridY >= fieldHeight_)
			return false;
		return field_[gridY][gridX] == 0;
	}

	// プレイヤースポーン取得（見つかれば true）
	bool TryGetPlayerSpawnIndex(uint32_t& outX, uint32_t& outY) const;
	Vector3 GetPlayerSpawnWorldPosition() const; // 見つからない場合は (0,0,0)

private:
	static inline const float kBlockWidth = 2.0f;
	static inline const float kBlockHeight = 2.0f;

	// シーンごとに変更可能なサイズ（CSVで上書きされます）
	uint32_t numBlockVertical_ = 10;
	uint32_t numBlockHorizontal_ = 10;

	MapChipData mapChipData_;

	int fieldWidth_ = 0;
	int fieldHeight_ = 0;
	std::vector<std::vector<int>> field_; // 0:通行可, 1:壁

	// スポーン保持
	int playerSpawnX_ = -1;
	int playerSpawnY_ = -1;
};