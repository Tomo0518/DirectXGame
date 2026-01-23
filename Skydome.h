#pragma once
#include "Model.h"
#include "WorldTransform.h"
#include "Camera.h"

class Skydome {
public:
	Skydome();
	~Skydome();

	// 初期化
	void Initialize(Model* model,Camera* camera);

	// 更新
	void Update();

	// 描画
	void Draw(ID3D12GraphicsCommandList* list);

private:
	// ワールドトランスフォーム
	WorldTransform worldTransform_;
	// モデル
	Model* model_ = nullptr;
	// カメラ
	Camera* camera_ = nullptr;
};
