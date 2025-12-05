#include <Windows.h>
#include <cstdint>
#include <string>
#include <format>
#include "ConvertString.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include<cassert>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

// ウィンドウプロシージャ
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg,
	WPARAM wparam, LPARAM lpram) {

	// メッセージに応じた処理
	switch (msg) {
	case WM_DESTROY: // ウィンドウが破棄された
		PostQuitMessage(0); // OSに対して、アプリ終了を伝える
		return 0;
	}
	return DefWindowProcW(hwnd, msg, wparam, lpram);
}

void Log(const std::string& message) {
	OutputDebugStringA(message.c_str());
}



// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	WNDCLASS wc{  };

	wc.lpfnWndProc = DefWindowProc;
	wc.lpszClassName = L"MyWindowClass";
	wc.hInstance = GetModuleHandle(nullptr);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	RegisterClass(&wc);


	// ウィンドウの幅と高さ
	const int32_t kClientWidth = 1280;
	const int32_t kClientHeight = 720;

	// ウィンドウサイズを決める
	RECT wrc = { 0,0,kClientWidth,kClientHeight };

	// ウィンドウサイズを補正する
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, FALSE);

	//// 出力ウィンドウへの文字出力
	//OutputDebugStringA("Hello,DirectX!\n");

	HWND hwnd = CreateWindow(
		wc.lpszClassName,				//利用するクラス名
		L"CG2",							// タイトルバーの文字
		WS_OVERLAPPEDWINDOW,			// よく見るウィンドウスタイル
		CW_USEDEFAULT, CW_USEDEFAULT,	// ウィンドウ表示位置x,y座標 
		wrc.right - wrc.left,			// ウィンドウ幅
		wrc.bottom - wrc.top,			// ウィンドウ高
		nullptr,						// 親ウィンドウハンドル
		nullptr,						// メニューハンドル
		wc.hInstance,					// インスタンスハンドル（全角スペースを半角に修正）
		nullptr							// オプション
	);

	MSG msg{};

	// DXGIファクトリーの生成
	IDXGIFactory7* dxgiFactory = nullptr;

	// ウィンドウのxボタンが押されるまでループ
	while (msg.message != WM_QUIT) {
		// Windowsメッセージが来ていたら最優先で処理させる
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
		else {
			//==================================
			// ゲームの処理 ↓↓
			//==================================
			ShowWindow(hwnd, SW_SHOW);

		}
	}



	return 0;


}
