#pragma once
#include <Windows.h>
#include <cstdint>
#include <string>

static const int kClientWidth = 1280;
static const int kClientHeight = 720;

// WindowsAPIに関連するウィンドウ管理を行うクラス
class Window {
public:
	// シングルトンインスタンスを取得する
	static Window* GetInstance();

	// ウィンドウを生成し初期化を行う
	// title: タイトルバーの文字列, width: クライアント領域の幅, height: クライアント領域の高さ
	void CreateGameWindow(const std::wstring& title = L"DirectXGame", int32_t width = kClientWidth, int32_t height = kClientHeight);

	// Windowsメッセージを処理する
	// true: 終了メッセージが来た（アプリ終了）, false: 継続
	bool ProcessMessage();

	// ウィンドウハンドル(HWND)を取得する
	HWND GetHwnd() const { return hwnd_; }

	void Shutdown() {
		// ウィンドウを破棄
		if (hwnd_) {
			DestroyWindow(hwnd_);
			hwnd_ = nullptr;
		}
		// ウィンドウクラスを登録解除
		if (wc_.hInstance) {
			UnregisterClass(wc_.lpszClassName, wc_.hInstance);
		}
	}

	// 登録したウィンドウクラスの解除などを行うデストラクタ
	~Window();

private:

	// シングルトンパターンのためコンストラクタを隠蔽
	Window() = default;
	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;

	// OSから呼び出されるウィンドウプロシージャ
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lpram);

	// メンバ変数
	HWND hwnd_ = nullptr;
	WNDCLASS wc_{};
};