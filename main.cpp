#include <Windows.h>
#include <cstdint>
#include <string>
#include <format>
#include "ConvertString.h"
#include "Logger.h"

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
		wc.lpszClassName,				// 利用するクラス名
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

#ifdef _DEBUG
	ID3D12Debug1* debugController = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		debugController->EnableDebugLayer();
		debugController->SetEnableGPUBasedValidation(TRUE);
	}
#endif

	/////////////////////////////////////////////
	// デバイスの設定
	////////////////////////////////////////////

	// ==============================================
	// DXGIファクトリーの生成
	// ==============================================

	IDXGIFactory7* dxgiFactory = nullptr;

	HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));

	assert(SUCCEEDED(hr));

	// ==============================================
	// アダプタ(GPU)を決定する
	// ==============================================

	IDXGIAdapter4* useAdapter = nullptr;

	// 高性能GPUを搭載しているアダプターを取得
	for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(i,
		DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND; ++i) {

		// アダプターの情報を取得
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr)); // 成功確認

		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
			// 採用したアダプターの情報をログに出力。wstringの方なので注意
			std::wstring wdesc(adapterDesc.Description);
			Log(std::format(L"Use Adapter:{}\n", wdesc));
			// ソフトウェアアダプターでなければ採用
			break;
		}
		useAdapter = nullptr; // ソフトウェアアダプタの場合は見なかったことにする
	}

	// 適切なアダプタが見つからなかったので起動できない
	assert(useAdapter != nullptr);

	// ================================================
	// D3D12デバイスの生成
	// ================================================

	ID3D12Device* device = nullptr;

	// 機能レベルとログ出力用の文字列
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
	};

	const char* featureLevelStrings[] = { "12.2","12.1","12.0" };

	//　高い順に機能レベル
	for (size_t i = 0; i < _countof(featureLevels); ++i) {
		// デバイスの生成を試みる
		hr = D3D12CreateDevice(
			useAdapter,				// 利用するアダプター
			featureLevels[i],		// 機能レベル
			IID_PPV_ARGS(&device)	// 作成するデバイスへのポインタ
		);

		if (SUCCEEDED(hr)) {
			// 成功したらログ出力
			Log(std::format("Feature Level : {}\n", featureLevelStrings[i]));
			break; // ループを抜ける
		}
	}

	// デバイスの生成がうまくいかなかったので起動できない時
	assert(device != nullptr);
	Log("Complete create D3D12Device!!!\n");

	//#ifdef _DEBUG
	//	ID3D12InfoQueue* infoQueue = nullptr;
	//	if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
	//		// やばいエラー時止まる
	//		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
	//
	//		// エラー時に止まる
	//		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
	//
	//		// 警告時に止まる
	//		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
	//
	//		D3D12_MESSAGE_ID denyIds[] = {
	//			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
	//		};
	//
	//		// 抑制するレベル
	//		D3D12_MESSAGE_SEVERITY severities[] = {
	//			D3D12_MESSAGE_SEVERITY_INFO
	//		};
	//
	//		D3D12_INFO_QUEUE_FILTER filter = {};
	//		filter.DenyList.NumIDs = _countof(denyIds);
	//		filter.DenyList.pIDList = denyIds;
	//		filter.DenyList.NumSeverities = _countof(severities);
	//		filter.DenyList.pSeverityList = severities;
	//
	//		// 指定したメッセージの表示を抑制する
	//		infoQueue->PushStorageFilter(&filter);
	//
	//		// 解放
	//		infoQueue->Release();
	//	}
	//#endif

		// ===========================================
		// コマンドキューを生成する
		// ===========================================
	ID3D12CommandQueue* commandQueue = nullptr;
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
	/*commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	commandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQueueDesc.NodeMask = 0;*/
	hr = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue));

	// コマンドキューの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));

	// ===========================================
	// コマンドリストを作成する
	// ===========================================
	// コマンドアロケータを生成する
	ID3D12CommandAllocator* commandAllocator = nullptr;
	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	assert(SUCCEEDED(hr));

	// コマンドリストを作成する
	ID3D12GraphicsCommandList* commandList = nullptr;
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, IID_PPV_ARGS(&commandList));

	// コマンドリストの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));

	// ============================================
	// スワップチェーンを作成する
	// ============================================
	IDXGISwapChain4* swapChain = nullptr;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = kClientWidth;								// 画面の幅、ウィンドウのクライアント領域を同じものにしておく
	swapChainDesc.Height = kClientHeight;							// 画面の高さ、ウィンドウのクライアント領域を同じものにしておく
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;				// 色のフォーマット
	swapChainDesc.SampleDesc.Count = 1;								// マルチサンプリングしない
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	// 描画のターゲットとして使う
	swapChainDesc.BufferCount = 2;									// バッファ数は2つでダブルバッファリング
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;		// モニターに移したら中身を破棄

	// コマンドキュー、ウィンドウハンドル、設定を渡して生成する
	hr = dxgiFactory->CreateSwapChainForHwnd(
		commandQueue,		// コマンドキュー
		hwnd,				// ウィンドウハンドル
		&swapChainDesc,		// スワップチェーンの設定
		nullptr,			// フルスクリーン設定
		nullptr,			// 制限
		reinterpret_cast<IDXGISwapChain1**>(&swapChain)); // 作成するスワップチェーンのポインタ

	assert(SUCCEEDED(hr));

	// ==================================
	// Discripter Heapの生成
	// ==================================
	ID3D12DescriptorHeap* rtvDescriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc = {};
	rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; // レンダーターゲットビュー
	rtvDescriptorHeapDesc.NumDescriptors = 2; // ダブルバッファリングなので2つ
	hr = device->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap));

	// ディスクリプタヒープの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));

	// SwapChainからResourceを引っ張てくる
	ID3D12Resource* swapChainResources[2] = { nullptr };
	hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));
	assert(SUCCEEDED(hr));// リソースの取得がうまくいかなかったので起動できない

	hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));
	assert(SUCCEEDED(hr));// リソースの取得がうまくいかなかったので起動できない

	// ==================================
	// レンダーターゲットビュー(RTV)の生成
	// ==================================
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // 出力結果をSRGBに変換して書き込む
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D; // 2Dテクスチャとして書き込む

	// ディスクリプタヒープのハンドルを取得
	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	// RTVを二つ作るのでディスクリプタを2つ用意
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2] = {};

	// まずは一つ目を作る
	rtvHandles[0] = rtvStartHandle;
	device->CreateRenderTargetView(swapChainResources[0], &rtvDesc, rtvHandles[0]);

	// 二つ目のハンドルを計算
	rtvHandles[1].ptr = rtvHandles[0].ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// 二つ目のRTVを作成
	device->CreateRenderTargetView(swapChainResources[1], &rtvDesc, rtvHandles[1]);

	// ==================================

	typedef struct D3D12_CPU_DESCRITOR_HANDLE {
		SIZE_T ptr;
	} D3D12_CPU_DESCRIPTOR_HANDLE;

	// ==================================
	// 画面をクリアする処理が含まれたコマンドリストの記録
	// ==================================
	// これから書き込むバックバッファのインデックスを取得
	UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

	// 描画先のRTVを指定
	commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, nullptr);

	// 指定した色で画面をクリアする
	float clearColor[] = { 0.1f,0.25f,0.5f,1.0f }; // 青っぽい色 RGBA
	commandList->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0, nullptr);

	// コマンドリストの記録を終了
	hr = commandList->Close();
	assert(SUCCEEDED(hr));

	// ==================================
	// GUIにキックする
	// ==================================
	// GUIにコマンドリストの実行を行わせる
	ID3D12CommandList* commandlLists[] = { commandList };
	commandQueue->ExecuteCommandLists(1, commandlLists);

	// GPUとOSに画面の交換を行うように通知する
	swapChain->Present(1, 0);

	// 次のフレーム用のコマンドリストを準備
	hr = commandAllocator->Reset();
	assert(SUCCEEDED(hr));
	hr = commandList->Reset(commandAllocator, nullptr);
	assert(SUCCEEDED(hr));

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
