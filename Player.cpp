#include "Player.h"
#include "Easing.h"
#include <algorithm>
#include <cassert>
#include <numbers>
#define NOMINMAX

#include "MapChipField.h"
#include "InputManager.h"
#include "GraphicsCore.h"

Player::Player() {}

Player::~Player() {}

void Player::Initialize(Model* model, Camera* camera, const Vector3& position) {
	assert(model); // Nullptrチェック

	// 初期化処理
	model_ = model;
	camera_ = camera;

	worldTransform_.Initialize(GraphicsCore::GetInstance()->GetDevice());
	worldTransform_.scale_ = { 1.0f, 1.0f, 1.0f };
	baseScale_ = worldTransform_.scale_;

	worldTransform_.rotation_.y = -std::numbers::pi_v<float> / 0.12f;
	worldTransform_.translation_ = position;
}

void Player::Update(const Camera& camera) {
	float dt = 1.0f / 60.0f;

	// ===================================
	// 1.移動制御
	// ===================================
	const auto& input = InputManager::GetInstance();

	// 水平方向の移動
	if (onGround_) {
		if (input->PushKey('A') || input->PushKey('D')) {

			Vector3 acceleration = {};
			if (input->PushKey('A')) {
				// 右移動中の左入力
				if (velocity_.x > 0.0f) {
					// 速度と逆方向入力中は急ブレーキ
					velocity_.x *= (1.0f - kAttenuation);
				}

				acceleration.x -= kAceleration;

				if (lrDirection_ != LRDirection::kLeft) {
					// 左向きに変更
					lrDirection_ = LRDirection::kLeft;

					turnFirstRotationY_ = worldTransform_.rotation_.y;
					turnTimer_ = kTurnTime;
				}
			}
			else if (input->PushKey('D')) {
				// 左移動中の右入力
				if (velocity_.x < 0.0f) {
					// 速度と逆方向入力中は急ブレーキ
					velocity_.x *= (1.0f - kAttenuation);
				}
				acceleration.x += kAceleration;

				if (lrDirection_ != LRDirection::kRight) {
					// 右向きに変更
					lrDirection_ = LRDirection::kRight;
					turnFirstRotationY_ = worldTransform_.rotation_.y;
					turnTimer_ = kTurnTime;
				}
			}

			velocity_ += acceleration;

			// 最大速度制限
			velocity_.x = std::clamp(velocity_.x, -kLimitRunSpeed, kLimitRunSpeed);

		}
		else {
			velocity_.x *= (1.0f - kAttenuation);
		}

		// ジャンプ入力
		if (input->TriggerKey(VK_SPACE)) {
			// ジャンプ初速度を与える
			velocity_.y = kJumpAcceleration;
			// 空中状態に切り替え
			onGround_ = false;

			// 伸ばしアニメ開始
			StartJumpStretch();
		}
	}

	// ===================================
	// 重力加速度（常に適用）
	// ===================================
	velocity_ += Vector3{ 0.0f, -kGravityAcceleration, 0.0f };
	// 落下速度制限
	velocity_.y = (std::max)(velocity_.y, -kLimitFallSpeed);

	// ===================================
	// 2.移動量を加味して衝突判定する
	// ===================================

	CollisionMapInfo collisionMapInfo{};

	// このフレームの移動量を入れる
	collisionMapInfo.moveAmount = velocity_ * dt;

	MapChipCollisionCheck(collisionMapInfo);

	// ==================================
	// 3.衝突判定を反映して移動させる
	// ==================================
	ApplyCollisionResult(collisionMapInfo);

	// ==================================
	// 4.天井に接触している場合の処理
	// ==================================
	if (collisionMapInfo.isHitCeiling) {
		//DebugText::GetInstance()->ConsolePrintf("Ceiling Hit\n");
		// 天井に衝突した場合、Y方向の速度を0にする
		velocity_.y = 0.0f;
	}

	// ==================================
	// 5.壁に接触してる場合の処理
	// ==================================
	WallContactProcess(collisionMapInfo);

	// ==================================
	// 6.接地状態の切り替え
	// ==================================
	SetOnGroundState(collisionMapInfo);

	// ==================================
	// 7.旋回制御
	// ==================================

	// 旋回タイマー更新
	if (turnTimer_ > 0.0f) {
		turnTimer_ -= dt;
		if (turnTimer_ < 0.0f) {
			turnTimer_ = 0.0f;
		}
	}

	// 旋回制御
	{
		// 左右自機キャラの角度テーブル
		float destinationRotationTable[] = {
			-std::numbers::pi_v<float> / 0.12f, // 左
			std::numbers::pi_v<float> / 0.12f,  // 右
		};

		// 状態に応じた目標角度
		float destinationRotationY = destinationRotationTable[static_cast<uint32_t>(lrDirection_)];

		// 0～1 の補間係数
		float t = 1.0f - (turnTimer_ / kTurnTime);

		// イージングをかける
		t = Easing::EaseInOut(t);

		// 現在角度と目標角度の差を求める
		float delta = destinationRotationY - turnFirstRotationY_;

		// 差を -π ～ +π に正規化して「最短方向の差」にする
		const float twoPi = std::numbers::pi_v<float> *2.0f;
		while (delta > std::numbers::pi_v<float>) {
			delta -= twoPi;
		}
		while (delta < -std::numbers::pi_v<float>) {
			delta += twoPi;
		}

		// 旋回開始角度から「最短方向」に補間
		worldTransform_.rotation_.y = turnFirstRotationY_ + delta * t;
	}

	// スライムのつぶし・伸ばし
	UpdateSquash(dt);

	// ==================================
	// 8.ワールド行列の更新
	// ==================================

	worldTransform_.UpdateMatrix(camera);
}

void Player::MapChipCollisionCheck(CollisionMapInfo& info) {
	if (mapChipField_ == nullptr) {
		return;
	}

	// 上方向衝突判定
	CeilingCollisionCheck(info);

	// 下方向衝突判定
	GroundCollisionCheck(info);

	// 右方向衝突判定
	RightWallCollisionCheck(info);

	// 左方向衝突判定
	LeftWallCollisionCheck(info);
}

// 下方向衝突判定処理
void Player::GroundCollisionCheck(CollisionMapInfo& info) {
	// 下方向移動がない場合は判定不要
	if (info.moveAmount.y >= 0) {
		return;
	}

	// 移動後の4つの角の座標
	std::array<Vector3, static_cast<uint32_t>(Corner::kNumConer)> positionsNew{};
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPosition(worldTransform_.translation_ + info.moveAmount, static_cast<Corner>(i));
	}

	MapChipType mapChipType;
	MapChipType mapChipTypeNext;
	bool hit = false;

	// 左下点の判定
	IndexSet indexSet;
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftBottom]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex - 1);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		// 移動前のセル番号を取得
		Vector3 currentLeftBottom = CornerPosition(worldTransform_.translation_, Corner::kLeftBottom);
		IndexSet indexSetNow = mapChipField_->GetMapChipIndexSetByPosition(currentLeftBottom);

		// Y方向のセル境界をまたいだ場合のみ判定
		if (indexSetNow.yIndex != indexSet.yIndex) {
			hit = true;
		}
	}

	// 右下点の判定
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightBottom]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex - 1);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		// 移動前のセル番号を取得
		Vector3 currentRightBottom = CornerPosition(worldTransform_.translation_, Corner::kRightBottom);
		IndexSet indexSetNow = mapChipField_->GetMapChipIndexSetByPosition(currentRightBottom);

		// Y方向のセル境界をまたいだ場合のみ判定
		if (indexSetNow.yIndex != indexSet.yIndex) {
			hit = true;
		}
	}

	if (hit) {
		const Vector3 centerNew = worldTransform_.translation_ + info.moveAmount;
		const Vector3 bottomCenterNew = centerNew + Vector3{ 0.0f, -kHeight / 2.0f, 0.0f };
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(bottomCenterNew);
		// めり込み先ブロックの範囲矩形
		MapChipField::Rect rect = mapChipField_->GetRextByIndex(indexSet.xIndex, indexSet.yIndex);
		info.moveAmount.y = (std::min)(0.0f, positionsNew[kLeftTop].y - rect.top + kHeight / 2.0f + kBlank);
		// 接地フラグON
		info.isOnGround = true;
	}
}

// 上方向衝突判定処理
void Player::CeilingCollisionCheck(CollisionMapInfo& info) {
	// 上方向移動がない場合は判定不要
	if (info.moveAmount.y <= 0) {
		return;
	}

	// 移動後の4つの角の座標
	std::array<Vector3, static_cast<uint32_t>(Corner::kNumConer)> positionsNew{};
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPosition(worldTransform_.translation_ + info.moveAmount, static_cast<Corner>(i));
	}

	MapChipType mapChipType;
	bool hit = false;

	// 左上点の判定
	IndexSet indexSet;
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	if (mapChipType == MapChipType::kBlock) {
		// 移動前のセル番号を取得
		Vector3 currentLeftTop = CornerPosition(worldTransform_.translation_, Corner::kLeftTop);
		IndexSet indexSetNow = mapChipField_->GetMapChipIndexSetByPosition(currentLeftTop);

		// Y方向のセル境界をまたいだ場合のみ判定
		if (indexSetNow.yIndex != indexSet.yIndex) {
			hit = true;
		}
	}

	// 右上点の判定
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	if (mapChipType == MapChipType::kBlock) {
		// 移動前のセル番号を取得
		Vector3 currentRightTop = CornerPosition(worldTransform_.translation_, Corner::kRightTop);
		IndexSet indexSetNow = mapChipField_->GetMapChipIndexSetByPosition(currentRightTop);

		// Y方向のセル境界をまたいだ場合のみ判定
		if (indexSetNow.yIndex != indexSet.yIndex) {
			hit = true;
		}
	}

	if (hit) {
		// めり込みを排除する方向に移動量を設定する
		const Vector3 centerNew = worldTransform_.translation_ + info.moveAmount;
		const Vector3 topCenterNew = centerNew + Vector3{ 0.0f, kHeight / 2.0f, 0.0f };
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(topCenterNew);
		// めり込み先ブロックの範囲矩形
		MapChipField::Rect rect = mapChipField_->GetRextByIndex(indexSet.xIndex, indexSet.yIndex);
		info.moveAmount.y = (std::max)(0.0f, positionsNew[kLeftBottom].y - rect.bottom - kHeight / 2.0f - kBlank);
		// 天井衝突フラグON
		info.isHitCeiling = true;
	}
}

// 右方向衝突判定処理
void Player::RightWallCollisionCheck(CollisionMapInfo& info) {
	// 右方向移動がない場合は判定不要
	if (info.moveAmount.x <= 0.0f) {
		return;
	}

	// 移動後の4つの角の座標
	std::array<Vector3, static_cast<uint32_t>(Corner::kNumConer)> positionsNew{};
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPosition(worldTransform_.translation_ + info.moveAmount, static_cast<Corner>(i));
	}

	MapChipType mapChipType;
	bool hit = false;

	// 右上点の判定
	IndexSet indexSet;
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	if (mapChipType == MapChipType::kBlock) {
		// 移動前のセル番号を取得
		Vector3 currentRightTop = CornerPosition(worldTransform_.translation_, Corner::kRightTop);
		IndexSet indexSetNow = mapChipField_->GetMapChipIndexSetByPosition(currentRightTop);

		// X方向のセル境界をまたいだ場合のみ判定
		if (indexSetNow.xIndex != indexSet.xIndex) {
			hit = true;
		}
	}

	// 右下点の判定
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightBottom]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	if (mapChipType == MapChipType::kBlock) {
		// 移動前のセル番号を取得
		Vector3 currentRightBottom = CornerPosition(worldTransform_.translation_, Corner::kRightBottom);
		IndexSet indexSetNow = mapChipField_->GetMapChipIndexSetByPosition(currentRightBottom);

		// X方向のセル境界をまたいだ場合のみ判定
		if (indexSetNow.xIndex != indexSet.xIndex) {
			hit = true;
		}
	}

	if (hit) {
		// めり込みを排除する方向に移動量を設定する
		const Vector3 centerNew = worldTransform_.translation_ + info.moveAmount;
		const Vector3 rightCenterNew = centerNew + Vector3{ kWidth / 2.0f, 0.0f, 0.0f };
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(rightCenterNew);
		// めり込み先ブロックの範囲矩形
		MapChipField::Rect rect = mapChipField_->GetRextByIndex(indexSet.xIndex, indexSet.yIndex);
		info.moveAmount.x = (std::min)(0.0f, rect.left - positionsNew[kRightTop].x + kWidth / 2.0f + kBlank);
		// 壁に接触したことを判定結果に記録する
		info.isHitWall = true;
	}
}

// 左方向衝突判定処理
void Player::LeftWallCollisionCheck(CollisionMapInfo& info) {
	// 左方向移動がない場合は判定不要
	if (info.moveAmount.x >= 0.0f) {
		return;
	}

	// 移動後の4つの角の座標
	std::array<Vector3, static_cast<uint32_t>(Corner::kNumConer)> positionsNew{};
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPosition(worldTransform_.translation_ + info.moveAmount, static_cast<Corner>(i));
	}

	MapChipType mapChipType;
	bool hit = false;

	// 左上点の判定
	IndexSet indexSet;
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	if (mapChipType == MapChipType::kBlock) {
		// 移動前のセル番号を取得
		Vector3 currentLeftTop = CornerPosition(worldTransform_.translation_, Corner::kLeftTop);
		IndexSet indexSetNow = mapChipField_->GetMapChipIndexSetByPosition(currentLeftTop);

		// X方向のセル境界をまたいだ場合のみ判定
		if (indexSetNow.xIndex != indexSet.xIndex) {
			hit = true;
		}
	}

	// 左下点の判定
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftBottom]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	if (mapChipType == MapChipType::kBlock) {
		// 移動前のセル番号を取得
		Vector3 currentLeftBottom = CornerPosition(worldTransform_.translation_, Corner::kLeftBottom);
		IndexSet indexSetNow = mapChipField_->GetMapChipIndexSetByPosition(currentLeftBottom);

		// X方向のセル境界をまたいだ場合のみ判定
		if (indexSetNow.xIndex != indexSet.xIndex) {
			hit = true;
		}
	}

	if (hit) {
		// めり込みを排除する方向に移動量を設定する
		const Vector3 centerNew = worldTransform_.translation_ + info.moveAmount;
		const Vector3 leftCenterNew = centerNew + Vector3{ -kWidth / 2.0f, 0.0f, 0.0f };
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(leftCenterNew);
		// めり込み先ブロックの範囲矩形
		MapChipField::Rect rect = mapChipField_->GetRextByIndex(indexSet.xIndex, indexSet.yIndex);
		info.moveAmount.x = (std::max)(0.0f, positionsNew[kLeftTop].x - rect.right - kWidth / 2.0f - kBlank);
		// 壁に接触したことを判定結果に記録する
		info.isHitWall = true;
	}
}

void Player::SetOnGroundState(const CollisionMapInfo& info) {
	// 自キャラが設置状態化の判定

	if (onGround_) { // 設置状態の処理
		if (velocity_.y > 0.0f) {
			onGround_ = false;
		}
		else {
			// 落下判定
			MapChipType mapChipType;
			bool hit = true;

			// 左下点の判定
			IndexSet indexSet;
			indexSet = mapChipField_->GetMapChipIndexSetByPosition(
				CornerPosition({ worldTransform_.translation_.x, worldTransform_.translation_.y - kBlank, worldTransform_.translation_.z }, Corner::kLeftBottom));
			mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
			if (mapChipType == MapChipType::kBlank) {
				hit = false;
			}
			// 右下点の判定
			indexSet = mapChipField_->GetMapChipIndexSetByPosition(
				CornerPosition({ worldTransform_.translation_.x, worldTransform_.translation_.y - kBlank, worldTransform_.translation_.z }, Corner::kRightBottom));
			mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
			if (mapChipType == MapChipType::kBlank) {
				hit = false;
			}

			if (!hit) {
				// 空中状態に切り替える
				onGround_ = false;
			}
		}

	}
	else { // 空中状態の処理
		if (info.isOnGround) {
			// 接地状態に変更
			onGround_ = true;
			// 着地時の速度調整
			velocity_.x *= (1.0f - kAttenuationLanding);
			velocity_.y = 0.0f;

			// つぶしアニメ開始
			StartLandSquash();
		}
	}
}

void Player::WallContactProcess(const CollisionMapInfo& info) {
	// 壁に接触している場合の処理
	if (info.isHitWall) {
		// X方向の速度を0にする
		velocity_.x *= (1.0f - kAttenuationWall);
	}
}

// プレイヤーの四隅の座標を取得
Vector3 Player::CornerPosition(const Vector3& center, Corner corner) {
	Vector3 offsetTable[] = {
		{kWidth / 2.0f,  -kHeight / 2.0f, 0.0f}, // 右下
		{-kWidth / 2.0f, -kHeight / 2.0f, 0.0f}, // 左下
		{kWidth / 2.0f,  kHeight / 2.0f,  0.0f}, // 右上
		{-kWidth / 2.0f, kHeight / 2.0f,  0.0f}, // 左上
	};

	return center + offsetTable[static_cast<uint32_t>(corner)];
}

// 判定結果を反映して移動させる
void Player::ApplyCollisionResult(const CollisionMapInfo& info) {
	// 移動量を反映
	worldTransform_.translation_ += info.moveAmount;
}



/////////////////////////////////////////////////////
// 描画処理
//////////////////////////////////////////////////////
void Player::Draw(ID3D12GraphicsCommandList* list) {
	// 描画処理
	//model_->Draw(list,worldTransform_);
	model_->Draw(list, worldTransform_);
}

// ================================
// つぶしと伸ばし用の関数
// ================================
// ジャンプ直後の「縦に伸びる」アニメ開始
void Player::StartJumpStretch() {
	squashState_ = SquashState::kJumpStretch;
	squashTimer_ = squashDuration_;

	// XZを少し細く、Yを伸ばす
	squashStart_ = baseScale_;
	squashEnd_ = {
		baseScale_.x * 0.8f,
		baseScale_.y * 1.3f,
		baseScale_.z * 0.8f,
	};
}

// 着地直後の「横につぶれる」アニメ開始
void Player::StartLandSquash() {
	squashState_ = SquashState::kLandSquash;
	squashTimer_ = squashDuration_;

	// XZを太く、Yを低く
	squashStart_ = baseScale_;
	squashEnd_ = {
		baseScale_.x * 1.3f,
		baseScale_.y * 0.7f,
		baseScale_.z * 1.3f,
	};
}

// 毎フレームの更新
void Player::UpdateSquash(float dt) {
	if (squashState_ == SquashState::kNone) {
		// 何も再生中でなければベーススケールにしておく
		worldTransform_.scale_ = baseScale_;
		return;
	}

	squashTimer_ -= dt;
	if (squashTimer_ < 0.0f) {
		squashTimer_ = 0.0f;
	}

	// 0～1 の正規化時間
	float t = 1.0f - (squashTimer_ / squashDuration_);
	t = std::clamp(t, 0.0f, 1.0f);

	// イージング
	float e = Easing::EaseInOut(t);

	// 開始→終了への補間
	Vector3 scale{};
	scale.x = squashStart_.x + (squashEnd_.x - squashStart_.x) * e;
	scale.y = squashStart_.y + (squashEnd_.y - squashStart_.y) * e;
	scale.z = squashStart_.z + (squashEnd_.z - squashStart_.z) * e;

	worldTransform_.scale_ = scale;

	// 終了したら状態を戻す
	if (squashTimer_ <= 0.0f) {
		worldTransform_.scale_ = baseScale_;
		squashState_ = SquashState::kNone;
	}
}