#pragma once
#include "IScene.h"

class GameScene :public IScene{
	public:
	GameScene();
	~GameScene();

	void Initialize() override;
	void Update() override;
	void Render() override;
};

