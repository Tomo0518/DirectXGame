//#pragma once
//#include "TomoEngine.h"
//#include "Vector3.h"
//
//class MapChipField;
//
//struct CollisionMapInfo {
//	// 天井衝突フラグ
//	bool isHitCeiling = false;
//	// 接地フラグ
//	bool isOnGround = false;
//	// 壁接触フラグ
//	bool isHitWall = false;
//	// 移動量
//	Vector3 moveAmount = { 0.0f, 0.0f, 0.0f };
//};
//
//enum Corner {
//	kRightBottom, // 右下
//	kLeftBottom,  // 左下
//	kRightTop,    // 右上
//	kLeftTop,     // 左上
//
//	kNumConer // コーナー数
//};
//
//class Player {
//public:
//	Player();
//	~Player();
//
//	// 初期化
//	void Initialize(TomoEngine::Model* model, TomoEngine::Camera* camera, const Vector3& position);
//
//	// 更新
//	void Update();
//
//	// 描画
//	void Draw();
//
//	// マップチップ衝突判定
//	void MapChipCollisionCheck(CollisionMapInfo& info);
//
//	// 上方向衝突判定関数
//	void CeilingCollisionCheck(CollisionMapInfo& info);
//	// 下方向衝突判定関数
//	void GroundCollisionCheck(CollisionMapInfo& info);
//	// 右方向衝突判定関数
//	void RightWallCollisionCheck(CollisionMapInfo& info);
//	// 左方向衝突判定関数
//	void LeftWallCollisionCheck(CollisionMapInfo& info);
//
//	// プレイヤーの四隅の座標を取得
//	Vector3 CornerPosition(const Vector3& center, Corner coner);
//
//	// 判定結果を反映して移動させる
//	void ApplyCollisionResult(const CollisionMapInfo& info);
//
//	// 接地状態の切り替え処理
//	void SetOnGroundState(const CollisionMapInfo& info);
//
//	// ===================================
//	// セッター
//	// ===================================
//	void SetMapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; }
//
//	// ===================================
//	// ゲッター
//	// ===================================
//	WorldTransform const& GetWorldTransform() const { return worldTransform_; }
//	const Vector3& GetVelocity() const { return velocity_; }
//	const bool GetOnGround() const { return onGround_; }
//
//private:
//	// ワールドトランスフォーム
//	WorldTransform worldTransform_;
//
//	// モデル
//	Model* model_ = nullptr;
//
//	// カメラ
//	Camera* camera_ = nullptr;
//
//	// テクスチャハンドル
//	uint32_t textureHandle_ = 0u;
//
//	// マップチップによるフィールド
//	MapChipField* mapChipField_ = nullptr;
//
//	// ===================================
//	// 移動制御
//	// ===================================
//	Vector3 velocity_ = {};
//
//	bool onGround_ = false;
//
//	// 左右
//	enum class LRDirection { kRight, kLeft };
//
//	LRDirection lrDirection_ = LRDirection::kRight;
//
//	// 旋回開始時の角度
//	float turnFirstRotationY_ = 0.0f;
//	// 旋回タイマー
//	float turnTimer_ = 0.0f;
//
//	// ===================================
//	// ジャンプ時のつぶしと伸ばし制御
//	// ===================================
//	enum class SquashState {
//		kNone,
//		kJumpStretch, // ジャンプ時の縦伸び
//		kLandSquash,  // 着地時の横つぶれ
//	};
//
//	SquashState squashState_ = SquashState::kNone;
//	float squashTimer_ = 0.0f;
//	float squashDuration_ = 0.15f; // 1アニメーションの時間(秒)
//
//	// ベーススケールと開始/終了スケール
//	Vector3 baseScale_ = { 1.0f, 1.0f, 1.0f };
//	Vector3 squashStart_ = { 1.0f, 1.0f, 1.0f };
//	Vector3 squashEnd_ = { 1.0f, 1.0f, 1.0f };
//
//	void StartJumpStretch();
//	void StartLandSquash();
//	void UpdateSquash(float dt);
//
//	// ===================================
//	// 定数
//	// ===================================
//
//	// プレイヤーの当たり判定サイズ
//	static inline const float kWidth = 0.8f;
//	static inline const float kHeight = 0.8f;
//
//	// 判定用の微小値
//	static inline const float kBlank = 0.01f;
//
//	// 加速度
//	static inline const float kAceleration = 3.0f;
//	// 減衰率
//	static inline const float kAttenuation = 0.3f;
//	// 最高速度
//	static inline const float kLimitRunSpeed = 10.0f;
//	// 旋回時間<秒>
//	static inline const float kTurnTime = 0.2f;
//
//	// 重力加速度(下方向)
//	static inline const float kGravityAcceleration = 9.8f;
//	// 最大落下速度(下方向)
//	static inline const float kLimitFallSpeed = 10.0f;
//	// ジャンプ初速(上方向)
//	static inline const float kJumpAcceleration = 60.0f;
//	// 着地時の速度減衰率
//	static inline const float kAttenuationLanding = 0.5f;
//};
