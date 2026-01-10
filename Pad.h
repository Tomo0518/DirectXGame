#pragma once
#include <Windows.h>
#include <Xinput.h>
#include <array>
#include <cstdint>

// XInputライブラリのリンク
#pragma comment(lib, "xinput.lib")

/// <summary>
/// XInputコントローラを管理するクラス
/// </summary>
class Pad {
public:
    // ボタンの列挙型
    enum class Button {
        A, B, X, Y,
        DPadUp, DPadDown, DPadLeft, DPadRight,
        Start, Back,
        LShoulder, RShoulder,
        LThumb, RThumb,
        Count
    };

    explicit Pad(DWORD userIndex = 0);

    // 初期化
    void Initialize();

    // 毎フレームの更新
    void Update();

    // ボタン判定
    bool Push(Button button) const;
    bool Trigger(Button button) const;
    bool Release(Button button) const;

    // スティック入力取得 (デッドゾーン適用済み: -1.0f ~ 1.0f)
    float GetLeftStickX() const { return leftStickX_; }
    float GetLeftStickY() const { return leftStickY_; }
    float GetRightStickX() const { return rightStickX_; }
    float GetRightStickY() const { return rightStickY_; }

    // トリガー入力取得 (0.0f ~ 1.0f)
    float GetLeftTrigger() const { return leftTrigger_; }
    float GetRightTrigger() const { return rightTrigger_; }

    // 接続確認
    bool IsConnected() const { return isConnected_; }

    // 振動機能
    // leftMotor: 低周波（重い振動） 0.0f ~ 1.0f
    // rightMotor: 高周波（軽い振動） 0.0f ~ 1.0f
    void SetVibration(float leftMotor, float rightMotor);

private:
    // デッドゾーン適用関数
    float ApplyDeadzone(short value, short deadzone);

    DWORD userIndex_;   // コントローラー番号(0-3)
    bool isConnected_ = false;

    XINPUT_STATE state_ = {};
    XINPUT_STATE preState_ = {};

    // 処理済みの入力値
    float leftStickX_ = 0.0f;
    float leftStickY_ = 0.0f;
    float rightStickX_ = 0.0f;
    float rightStickY_ = 0.0f;
    float leftTrigger_ = 0.0f;
    float rightTrigger_ = 0.0f;

    // ボタンのマッピング用
    static const WORD kButtonMap[];
};