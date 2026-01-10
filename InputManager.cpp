#include "InputManager.h"
#include <cassert>

InputManager* InputManager::GetInstance() {
    static InputManager instance;
    return &instance;
}

void InputManager::Initialize(HWND hwnd) {
    hwnd_ = hwnd;
    pad_.Initialize();
}

void InputManager::Update() {
    // 1. キーボード情報の更新
    // 前回の状態を保存
    keyPreState_ = keyState_;
    // 現在のキーボード状態を全取得
    if (!GetKeyboardState(keyState_.data())) {
        // 取得失敗時はゼロクリア等の処理が必要だが、通常は失敗しない
        keyState_.fill(0);
    }

    // 2. マウス情報の更新
    mouseButtonPreState_ = mouseButtonState_;

    // マウスボタンの状態取得 (標準的なGetKeyStateを使用)
    // 0x8000 は最上位ビット(押されているか)をチェックするマスク
    mouseButtonState_[static_cast<int>(MouseButton::Left)] = (GetKeyState(VK_LBUTTON) & 0x8000) != 0;
    mouseButtonState_[static_cast<int>(MouseButton::Right)] = (GetKeyState(VK_RBUTTON) & 0x8000) != 0;
    mouseButtonState_[static_cast<int>(MouseButton::Middle)] = (GetKeyState(VK_MBUTTON) & 0x8000) != 0;

    // マウス座標の更新
    POINT p;
    if (GetCursorPos(&p)) {
        // スクリーン座標からクライアント座標へ変換
        if (hwnd_) {
            ScreenToClient(hwnd_, &p);
        }
        mousePrePos_ = mousePos_;
        mousePos_ = Vector2(static_cast<float>(p.x), static_cast<float>(p.y));

        // 移動量の計算
        mouseDelta_ = mousePos_ - mousePrePos_;
    }

    // 3. パッド情報の更新
    pad_.Update();
}

void InputManager::ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    // ホイール入力の処理
    if (msg == WM_MOUSEWHEEL) {
        // 120の倍数で送られてくる値を加算
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        wheel_ += delta;
    }
}

// ===========================================================================
// キーボード関連
// ===========================================================================

bool InputManager::PushKey(BYTE keyNumber) const {
    // 0x80 は押されている状態を示すビット
    return (keyState_[keyNumber] & 0x80) != 0;
}

bool InputManager::TriggerKey(BYTE keyNumber) const {
    // 現在押されていて、かつ前回押されていなかった
    return ((keyState_[keyNumber] & 0x80) != 0) && ((keyPreState_[keyNumber] & 0x80) == 0);
}

bool InputManager::ReleaseKey(BYTE keyNumber) const {
    // 現在押されていなくて、かつ前回押されていた
    return ((keyState_[keyNumber] & 0x80) == 0) && ((keyPreState_[keyNumber] & 0x80) != 0);
}

// ===========================================================================
// マウス関連
// ===========================================================================

bool InputManager::PushMouse(MouseButton button) const {
    return mouseButtonState_[static_cast<int>(button)];
}

bool InputManager::TriggerMouse(MouseButton button) const {
    return mouseButtonState_[static_cast<int>(button)] && !mouseButtonPreState_[static_cast<int>(button)];
}

bool InputManager::ReleaseMouse(MouseButton button) const {
    return !mouseButtonState_[static_cast<int>(button)] && mouseButtonPreState_[static_cast<int>(button)];
}

Vector2 InputManager::GetMousePosition() const {
    return mousePos_;
}