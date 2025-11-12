#include "MapChipField.h"
#include <cassert>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace MapChipFieldData
{
	std::map<std::string, MapChipType> mapChipTable = {
		{"0", MapChipType::kBlank      },
		{"1", MapChipType::kWall       },
		{"2", MapChipType::kPlayerSpawn},
		{"3", MapChipType::kAppleSpawn },
		{"4", MapChipType::kBombSpawn  },
	};
}

void MapChipField::Initialize()
{
}

void MapChipField::ResetMapChipData()
{
	mapChipData_.data.clear();
	mapChipData_.data.resize(numBlockVertical_);
	for (std::vector<MapChipType>& row : mapChipData_.data)
	{
		row.resize(numBlockHorizontal_, MapChipType::kBlank);
	}

	// 通行配列も同期
	fieldHeight_ = static_cast<int>(numBlockVertical_);
	fieldWidth_  = static_cast<int>(numBlockHorizontal_);
	field_.assign(fieldHeight_, std::vector<int>(fieldWidth_, 0));

	// スポーン初期化
	playerSpawnX_ = -1;
	playerSpawnY_ = -1;
}

void MapChipField::LoadmapChipCsv(const std::string& filePath)
{
	// ファイルを開く
	std::ifstream file(filePath);
#ifdef _DEBUG
	assert(file.is_open());
#endif

	// 行単位でまず全読み込み
	std::vector<std::string> lines;
	std::string line;
	while (std::getline(file, line))
	{
		lines.push_back(line);
	}
	file.close();

	// CSVのサイズを決定（行数、最大列数）
	uint32_t rows = static_cast<uint32_t>(lines.size());
	uint32_t cols = 0;
	{
		for (const std::string& ln : lines)
		{
			uint32_t c = 0;
			std::istringstream ls(ln);
			std::string word;
			while (std::getline(ls, word, ',')) { ++c; }
			if (c > cols) cols = c;
		}
	}

	// サイズを反映
	numBlockVertical_   = rows > 0 ? rows : 0;
	numBlockHorizontal_ = cols > 0 ? cols : 0;

	ResetMapChipData(); // 上記サイズで確保

	// 再度パースして格納
	for (uint32_t i = 0; i < numBlockVertical_; ++i)
	{
		std::istringstream ls(lines[i]);
		std::string word;
		for (uint32_t j = 0; j < numBlockHorizontal_; ++j)
		{
			if (!std::getline(ls, word, ',')) break;

			auto it = MapChipFieldData::mapChipTable.find(word);
			if (it != MapChipFieldData::mapChipTable.end())
			{
				mapChipData_.data[i][j] = it->second;
			}
			else
			{
				mapChipData_.data[i][j] = MapChipType::kBlank;
			}
		}
	}

	// 通行配列とスポーン位置を同期
	playerSpawnX_ = -1;
	playerSpawnY_ = -1;
	for (uint32_t y = 0; y < numBlockVertical_; ++y)
	{
		for (uint32_t x = 0; x < numBlockHorizontal_; ++x)
		{
			const MapChipType t = mapChipData_.data[y][x];
			field_[y][x] = (t == MapChipType::kWall) ? 1 : 0;

			if (t == MapChipType::kPlayerSpawn && playerSpawnX_ < 0)
			{
				playerSpawnX_ = static_cast<int>(x);
				playerSpawnY_ = static_cast<int>(y);
			}
		}
	}
}

MapChipType MapChipField::GetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex)
{
	if (xIndex >= numBlockHorizontal_ || yIndex >= numBlockVertical_)
	{
		return MapChipType::kBlank;
	}
	return mapChipData_.data[yIndex][xIndex];
}

Vector3 MapChipField::GetMapChipPositionByIndex(uint32_t xIndex, uint32_t yIndex)
{
	float halfW = kBlockWidth * 0.5f;
	float halfH = kBlockHeight * 0.5f;
	// 上原点（yIndex=0 が最上段）
	return Vector3(
		kBlockWidth * xIndex + halfW,
		kBlockHeight * (numBlockVertical_ - 1 - yIndex) + halfH,
		0.0f
	);
}

void MapChipField::SetGridSize(uint32_t vertical, uint32_t horizontal)
{
	numBlockVertical_   = vertical;
	numBlockHorizontal_ = horizontal;

	fieldHeight_ = static_cast<int>(vertical);
	fieldWidth_  = static_cast<int>(horizontal);
	field_.assign(fieldHeight_, std::vector<int>(fieldWidth_, 0));

	// マップデータも合わせる
	mapChipData_.data.clear();
	mapChipData_.data.resize(numBlockVertical_, std::vector<MapChipType>(numBlockHorizontal_, MapChipType::kBlank));

	playerSpawnX_ = -1;
	playerSpawnY_ = -1;
}

void MapChipField::ResizeKeepContent(uint32_t vertical, uint32_t horizontal, MapChipType fill)
{
	// 元データを退避
	const uint32_t oldH = numBlockVertical_;
	const uint32_t oldW = numBlockHorizontal_;
	auto oldData = std::move(mapChipData_.data);

	// 新サイズ確定
	numBlockVertical_ = vertical;
	numBlockHorizontal_ = horizontal;

	// 新マップをfillで初期化
	mapChipData_.data.clear();
	mapChipData_.data.resize(numBlockVertical_, std::vector<MapChipType>(numBlockHorizontal_, fill));

	// 共通領域をコピー
	const uint32_t copyH = std::min(oldH, numBlockVertical_);
	const uint32_t copyW = std::min(oldW, numBlockHorizontal_);
	for (uint32_t y = 0; y < copyH; ++y)
	{
		for (uint32_t x = 0; x < copyW; ++x)
		{
			mapChipData_.data[y][x] = oldData[y][x];
		}
	}

	// 通行配列とスポーンを再構築
	fieldHeight_ = static_cast<int>(numBlockVertical_);
	fieldWidth_ = static_cast<int>(numBlockHorizontal_);
	field_.assign(fieldHeight_, std::vector<int>(fieldWidth_, 0));

	playerSpawnX_ = -1;
	playerSpawnY_ = -1;
	for (uint32_t y = 0; y < numBlockVertical_; ++y)
	{
		for (uint32_t x = 0; x < numBlockHorizontal_; ++x)
		{
			const MapChipType t = mapChipData_.data[y][x];
			field_[y][x] = (t == MapChipType::kWall) ? 1 : 0;
			if (t == MapChipType::kPlayerSpawn && playerSpawnX_ < 0)
			{
				playerSpawnX_ = static_cast<int>(x);
				playerSpawnY_ = static_cast<int>(y);
			}
		}
	}
}

bool MapChipField::TryGetPlayerSpawnIndex(uint32_t& outX, uint32_t& outY) const
{
	if (playerSpawnX_ < 0 || playerSpawnY_ < 0) return false;
	outX = static_cast<uint32_t>(playerSpawnX_);
	outY = static_cast<uint32_t>(playerSpawnY_);
	return true;
}

Vector3 MapChipField::GetPlayerSpawnWorldPosition() const
{
	if (playerSpawnX_ < 0 || playerSpawnY_ < 0)
	{
		return Vector3{0.0f, 0.0f, 0.0f};
	}
	return const_cast<MapChipField*>(this)->GetMapChipPositionByIndex(
		static_cast<uint32_t>(playerSpawnX_),
		static_cast<uint32_t>(playerSpawnY_));
}
