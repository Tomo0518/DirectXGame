#include "Window.h"
#include "Logger.h"
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_win32.h"

// ImGuiのWindowsメッセージハンドラ（外部定義）
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

Window* Window::GetInstance() {
	static Window instance;
	return &instance;
}

// ウィンドウプロシージャ
// OSから送られてくるメッセージを処理するコールバック関数
LRESULT CALLBACK Window::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lpram) {
	// ImGuiへのメッセージ伝達
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lpram)) {
		return true;
	}

	// メッセージに応じた処理
	switch (msg) {
	case WM_DESTROY: // ウィンドウが破棄された
		PostQuitMessage(0); // OSに対してアプリ終了を通知
		return 0;
	}
	// 標準のメッセージ処理
	return DefWindowProcW(hwnd, msg, wparam, lpram);
}

void Window::CreateGameWindow(const std::wstring& title, int32_t width, int32_t height) {
	// COMライブラリの初期化（DirectXなどで必要となる場合があるため）
	CoInitializeEx(0, COINIT_MULTITHREADED);

	// ウィンドウクラスの設定
	wc_.lpfnWndProc = WindowProc;
	wc_.lpszClassName = L"DirectXGameWindowClass";
	wc_.hInstance = GetModuleHandle(nullptr);
	wc_.hCursor = LoadCursor(nullptr, IDC_ARROW);

	// ウィンドウクラスをOSに登録
	RegisterClass(&wc_);

	// ウィンドウサイズの設定（クライアント領域のサイズからウィンドウ全体のサイズを計算）
	RECT wrc = { 0, 0, width, height };
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, FALSE);

	// ウィンドウの生成
	hwnd_ = CreateWindow(
		wc_.lpszClassName,		// クラス名
		title.c_str(),			// タイトルバーの文字
		WS_OVERLAPPEDWINDOW,	// ウィンドウスタイル
		CW_USEDEFAULT, CW_USEDEFAULT, // 表示位置（OSにお任せ）
		wrc.right - wrc.left,	// ウィンドウ幅
		wrc.bottom - wrc.top,	// ウィンドウ高
		nullptr,				// 親ウィンドウハンドル
		nullptr,				// メニューハンドル
		wc_.hInstance,			// インスタンスハンドル
		nullptr					// オプション
	);

	// ウィンドウを表示
	ShowWindow(hwnd_, SW_SHOW);
}

bool Window::ProcessMessage() {
	MSG msg{};
	// メッセージキューからメッセージを取り出す
	if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg); // キー入力メッセージの変換
		DispatchMessageW(&msg); // ウィンドウプロシージャへ送る
	}

	// WM_QUITメッセージが来たら終了フラグを立てる
	if (msg.message == WM_QUIT) {
		return true;
	}
	return false;
}

Window::~Window() {
	// ウィンドウクラスの登録解除
	if (wc_.hInstance) {
		UnregisterClass(wc_.lpszClassName, wc_.hInstance);
	}
}