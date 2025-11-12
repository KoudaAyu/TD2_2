#pragma once
#include "DirectXCom.h"

class ModelCommon
{
public:
	// 初期化
	void Initialize(DirectXCom* dxCommon);

	// ゲッター
	DirectXCom* GetDxCommon()const { return dxCommon_; }
private:
	DirectXCom* dxCommon_ ;
};


