#pragma once
#include <Windows.h>
#include <array>
#include <vector>
#include "Vector2.h"
#include "Pad.h"

// マウスボタンの定義
enum class MouseButton {
    Left = 0,
    Right = 1,
    Middle = 2,
};

// 入力システム全体を管理するクラス（シングルトン）
class InputManager {
public:
    // シングルトンインスタンスの取得
    static InputManager* GetInstance();

    // 削除・コピー禁止
    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;

    // 初期化（ウィンドウハンドルが必要）
    void Initialize(HWND hwnd);

    // 毎フレームの更新処理
    void Update();

    // ウィンドウプロシージャからメッセージを受け取る（ホイール用）
    void ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    /// <summary>
    /// キーが押されているか (Press)
    /// </summary>
    bool PushKey(BYTE keyNumber) const;

    /// <summary>
    /// キーが押された瞬間か (Trigger)
    /// </summary>
    bool TriggerKey(BYTE keyNumber) const;

    /// <summary>
    /// キーが離された瞬間か (Release)
    /// </summary>
    bool ReleaseKey(BYTE keyNumber) const;

    /// <summary>
    /// マウスボタンが押されているか (Press)
    /// </summary>
    bool PushMouse(MouseButton button) const;

    /// <summary>
    /// マウスボタンが押された瞬間か (Trigger)
    /// </summary>
    bool TriggerMouse(MouseButton button) const;

    /// <summary>
    /// マウスボタンが離された瞬間か (Release)
    /// </summary>
    bool ReleaseMouse(MouseButton button) const;

    /// <summary>
    /// マウスのクライアント座標を取得
    /// </summary>
    Vector2 GetMousePosition() const;

    /// <summary>
    /// マウスの移動量（前フレーム比）を取得
    /// </summary>
    Vector2 GetMouseDelta() const { return mouseDelta_; }

    /// <summary>
    /// マウスホイールの累積量を取得
    /// </summary>
    int GetWheel() const { return wheel_; }

    /// <summary>
    /// XInput対応ゲームパッドを取得
    /// </summary>
    Pad* GetPad() { return &pad_; }

private:
    InputManager() = default;
    ~InputManager() = default;

    HWND hwnd_ = nullptr;

    // キーボード
    std::array<BYTE, 256> keyState_ = { 0 };     // 現在のキー状態
    std::array<BYTE, 256> keyPreState_ = { 0 };  // 1フレーム前のキー状態

    // マウス
    using MouseState = std::array<bool, 3>;
    MouseState mouseButtonState_ = { false };
    MouseState mouseButtonPreState_ = { false };
    Vector2 mousePos_ = { 0, 0 };
    Vector2 mousePrePos_ = { 0, 0 };
    Vector2 mouseDelta_ = { 0, 0 };
    int wheel_ = 0; // ホイール累積値

    // ゲームパッド
    Pad pad_;
};


