#include "Camera.h"
#include "Vector3.h"

// 前方宣言
class Player;

struct Rect {
	float left = 0.0f;
	float right = 0.0f;
	float top = 0.0f;
	float bottom = 0.0f;
};

class CameraController {
public:
	CameraController();
	~CameraController();

	// 初期化
	void Initialize();

	// 更新
	void Update();

	void SetTarget(Player* target) { target_ = target; }

	void Reset();

	Camera* GetCamera() { return &camera_; }

	void SetMovableArea(const Rect& area) { movableArea_ = area; }

private:
	Camera camera_;
	Player* target_ = nullptr;

	// 現在のオフセット
	Vector3 targetOffset_ = { 0.0f, 0.0f, -25.0f };

	// 目標座標
	Vector3 targetPosition_ = {};

	// 座標補完割合
	static inline const float kInterpolationRate = 0.1f;

	// 速度掛け率
	static inline const float kVelocityBias = 0.5f;

	// 追従対象の各方向へのカメラ移動範囲
	Rect movableArea_ = { 20.5f, 100.0f, 0.0f, 100.0f };

	// 追従対象を画面内に納めるためのマージン
	static inline const Rect kMargin = { 5.0f, 5.0f, 5.0f, 5.0f };
};