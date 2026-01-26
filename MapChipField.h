#pragma once
#include "Math.h"
#include <cstdint>
#include <string>
#include <vector>

static inline const float kBlockWidth = 2.0f;
static inline const float kBlockHeight = 2.0f;

static inline const uint32_t kNumBlockHorizontal = 100;
static inline const uint32_t kNumBlockVertical = 20;



enum class MapChipType {
	kBlank, // 空白
	kBlock, // ブロック
};

struct MapChipData {
	std::vector<std::vector<MapChipType>> data;
};

struct IndexSet {
	uint32_t xIndex;
	uint32_t yIndex;
};

class MapChipField {
public:
	void ResetMapChipData();

	void LoadMapChipCsv(const std::string& filePath);

	MapChipType GetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex);

	Vector3 GetMapChipPositionByIndex(uint32_t xIndex, uint32_t yIndex);

	uint32_t GetNumBlockVertical() const { return kNumBlockVertical; }
	uint32_t GetNumBlockHorizontal() const { return kNumBlockHorizontal; }



	IndexSet GetMapChipIndexSetByPosition(const Vector3& position);

	struct Rect {
		float left;
		float right;
		float bottom;
		float top;
	};

	Rect GetRextByIndex(uint32_t xIndex, uint32_t yIndex) {

		Vector3 center = GetMapChipPositionByIndex(xIndex, yIndex);

		Rect rect;
		rect.left = center.x - kBlockWidth / 2.0f;
		rect.right = center.x + kBlockWidth / 2.0f;
		rect.bottom = center.y - kBlockHeight / 2.0f;
		rect.top = center.y + kBlockHeight / 2.0f;
		return rect;
	}

private:
	MapChipData mapChipData_;
};