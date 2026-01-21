#define NOMINMAX
#include "Pad.h"
#include <cmath>
#include <algorithm>

// ボタンとXInputビットマスクの対応テーブル
const WORD Pad::kButtonMap[] = {
    XINPUT_GAMEPAD_A,
    XINPUT_GAMEPAD_B,
    XINPUT_GAMEPAD_X,
    XINPUT_GAMEPAD_Y,
    XINPUT_GAMEPAD_DPAD_UP,
    XINPUT_GAMEPAD_DPAD_DOWN,
    XINPUT_GAMEPAD_DPAD_LEFT,
    XINPUT_GAMEPAD_DPAD_RIGHT,
    XINPUT_GAMEPAD_START,
    XINPUT_GAMEPAD_BACK,
    XINPUT_GAMEPAD_LEFT_SHOULDER,
    XINPUT_GAMEPAD_RIGHT_SHOULDER,
    XINPUT_GAMEPAD_LEFT_THUMB,
    XINPUT_GAMEPAD_RIGHT_THUMB
};

Pad::Pad(DWORD userIndex) : userIndex_(userIndex) {
}

void Pad::Initialize() {
    // 状態をクリア
    ZeroMemory(&state_, sizeof(XINPUT_STATE));
    ZeroMemory(&preState_, sizeof(XINPUT_STATE));
    isConnected_ = false;
}

void Pad::Update() {
    preState_ = state_;

    // 入力状態を取得
    DWORD result = XInputGetState(userIndex_, &state_);
    isConnected_ = (result == ERROR_SUCCESS);

    if (!isConnected_) {
        ZeroMemory(&state_, sizeof(XINPUT_STATE));
        return;
    }

    // デッドゾーンの適用と正規化 (-1.0 ~ 1.0)
    leftStickX_ = ApplyDeadzone(state_.Gamepad.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
    leftStickY_ = ApplyDeadzone(state_.Gamepad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
    rightStickX_ = ApplyDeadzone(state_.Gamepad.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
    rightStickY_ = ApplyDeadzone(state_.Gamepad.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);

    // トリガーの処理 (0.0 ~ 1.0)
    // 閾値(XINPUT_GAMEPAD_TRIGGER_THRESHOLD = 30)を考慮
    if (state_.Gamepad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD) {
        leftTrigger_ = static_cast<float>(state_.Gamepad.bLeftTrigger) / 255.0f;
    }
    else {
        leftTrigger_ = 0.0f;
    }

    if (state_.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD) {
        rightTrigger_ = static_cast<float>(state_.Gamepad.bRightTrigger) / 255.0f;
    }
    else {
        rightTrigger_ = 0.0f;
    }
}

float Pad::ApplyDeadzone(short value, short deadzone) {
    if (std::abs(value) < deadzone) {
        return 0.0f;
    }

    // デッドゾーンを超えた値を 0.0~1.0 (または -1.0~0.0) に正規化
    // 最大値(32767)に対する割合
    float ret = static_cast<float>(value) / 32767.0f;

    // 完全に1.0を超えないようにクランプ
    return std::max(-1.0f, std::min(ret, 1.0f));
}

bool Pad::Push(Button button) const {
    if (!isConnected_) return false;
    return (state_.Gamepad.wButtons & kButtonMap[static_cast<int>(button)]) != 0;
}

bool Pad::Trigger(Button button) const {
    if (!isConnected_) return false;
    WORD mask = kButtonMap[static_cast<int>(button)];
    return ((state_.Gamepad.wButtons & mask) != 0) && ((preState_.Gamepad.wButtons & mask) == 0);
}

bool Pad::Release(Button button) const {
    if (!isConnected_) return false;
    WORD mask = kButtonMap[static_cast<int>(button)];
    return ((state_.Gamepad.wButtons & mask) == 0) && ((preState_.Gamepad.wButtons & mask) != 0);
}

void Pad::SetVibration(float leftMotor, float rightMotor) {
    if (!isConnected_) return;

    XINPUT_VIBRATION vibration;
    ZeroMemory(&vibration, sizeof(XINPUT_VIBRATION));

    // 0.0-1.0 を 0-65535 に変換
    vibration.wLeftMotorSpeed = static_cast<WORD>(std::clamp(leftMotor, 0.0f, 1.0f) * 65535);
    vibration.wRightMotorSpeed = static_cast<WORD>(std::clamp(rightMotor, 0.0f, 1.0f) * 65535);

    XInputSetState(userIndex_, &vibration);
}