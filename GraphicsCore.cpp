#include "GraphicsCore.h"
#include "Logger.h"
#include <cassert>
#include <format>

GraphicsCore* GraphicsCore::GetInstance() {
	static GraphicsCore instance;
	return &instance;
}

void GraphicsCore::Initialize() {
#ifdef _DEBUG
	// デバッグレイヤーの有効化
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController_)))) {
		debugController_->EnableDebugLayer();
		// GPU側でのチェックも有効化
		debugController_->SetEnableGPUBasedValidation(TRUE);
	}
#endif

	// DXGIファクトリーの生成
	hr_ = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory_));
	assert(SUCCEEDED(hr_));

	// 使用するアダプタ(GPU)の決定
	Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter = nullptr;

	// 高性能GPUを優先して探す
	for (UINT i = 0; dxgiFactory_->EnumAdapterByGpuPreference(i,
		DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND; ++i) {

		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr_ = useAdapter->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr_));

		// ソフトウェアアダプタ（Microsoft Basic Render Driverなど）は除外
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
			// ログに出力
			Log(std::format(L"Use Adapter:{}\n", adapterDesc.Description));
			break;
		}
		useAdapter = nullptr;
	}
	assert(useAdapter != nullptr); // 適切なGPUが見つからなかった

	// D3D12デバイスの生成
	// 機能レベルの候補（高い順）
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
	};
	const char* featureLevelStrings[] = { "12.2", "12.1", "12.0" };

	for (size_t i = 0; i < _countof(featureLevels); ++i) {
		hr_ = D3D12CreateDevice(useAdapter.Get(), featureLevels[i], IID_PPV_ARGS(&device_));
		if (SUCCEEDED(hr_)) {
			Log(std::format("Feature Level : {}\n", featureLevelStrings[i]));
			break;
		}
	}
	assert(device_ != nullptr);
	Log("Complete create D3D12Device!!!\n");

#ifdef _DEBUG
	// デバッグ時のエラー停止設定
	if (SUCCEEDED(device_->QueryInterface(IID_PPV_ARGS(&infoQueue_)))) {
		// ヤバいエラー時に停止
		infoQueue_->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		infoQueue_->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		infoQueue_->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

		// 抑制するメッセージ
		D3D12_MESSAGE_ID denyIds[] = {
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
		};
		D3D12_MESSAGE_SEVERITY severities[] = {
			D3D12_MESSAGE_SEVERITY_INFO
		};

		D3D12_INFO_QUEUE_FILTER filter = {};
		filter.DenyList.NumIDs = _countof(denyIds);
		filter.DenyList.pIDList = denyIds;
		filter.DenyList.NumSeverities = _countof(severities);
		filter.DenyList.pSeverityList = severities;

		infoQueue_->PushStorageFilter(&filter);
	}
#endif
}

void GraphicsCore::Initialize(void* windowHandle, int width, int height) {
#ifdef _DEBUG

	// ================================
	// 1. デバッグレイヤー有効化
	// ================================
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController_)))) {
		debugController_->EnableDebugLayer();
		// GPU側でのチェックも有効化
		debugController_->SetEnableGPUBasedValidation(TRUE);
	}
#endif

	// ===============================
	// 2. DXGIファクトリーの生成
	// ===============================
	hr_ = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory_));
	assert(SUCCEEDED(hr_));

	// 使用するアダプタ(GPU)の決定
	Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter = nullptr;

	// 高性能GPUを優先して探す
	for (UINT i = 0; dxgiFactory_->EnumAdapterByGpuPreference(i,
		DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND; ++i) {

		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr_ = useAdapter->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr_));

		// ソフトウェアアダプタ（Microsoft Basic Render Driverなど）は除外
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
			// ログに出力
			Log(std::format(L"Use Adapter:{}\n", adapterDesc.Description));
			break;
		}
		useAdapter = nullptr;
	}
	assert(useAdapter != nullptr); // 適切なGPUが見つからなかった

	// ================================
	// 3. D3D12デバイスの生成
	// ================================
	// 機能レベルの候補（高い順）
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
	};
	const char* featureLevelStrings[] = { "12.2", "12.1", "12.0" };

	for (size_t i = 0; i < _countof(featureLevels); ++i) {
		hr_ = D3D12CreateDevice(useAdapter.Get(), featureLevels[i], IID_PPV_ARGS(&device_));
		if (SUCCEEDED(hr_)) {
			Log(std::format("Feature Level : {}\n", featureLevelStrings[i]));
			break;
		}
	}
	assert(device_ != nullptr);
	Log("Complete create D3D12Device!!!\n");

#ifdef _DEBUG
	// デバッグ時のエラー停止設定
	if (SUCCEEDED(device_->QueryInterface(IID_PPV_ARGS(&infoQueue_)))) {
		// ヤバいエラー時に停止
		infoQueue_->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		infoQueue_->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		infoQueue_->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

		// 抑制するメッセージ
		D3D12_MESSAGE_ID denyIds[] = {
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
		};
		D3D12_MESSAGE_SEVERITY severities[] = {
			D3D12_MESSAGE_SEVERITY_INFO
		};

		D3D12_INFO_QUEUE_FILTER filter = {};
		filter.DenyList.NumIDs = _countof(denyIds);
		filter.DenyList.pIDList = denyIds;
		filter.DenyList.NumSeverities = _countof(severities);
		filter.DenyList.pSeverityList = severities;

		infoQueue_->PushStorageFilter(&filter);
	}
#endif

	// ====================================
	// 4. マネージャー・アロケータの初期化
	// ====================================
	commandListManager_.Create(device_.Get());
	m_RTVAllocator.Create(device_.Get());
	m_DSVAllocator.Create(device_.Get());
	m_SRVAllocator.Create(device_.Get()); // これはShaderVisible=trueにする拡張が必要かも

	// ===================================
	// 5. スワップチェーンの作成
	// ===================================
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = FALSE;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = BufferCount;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain1;

	// コマンドキューを渡す際、CommandQueueクラスから生ポインタを取り出す
	// ※GetD3D12CommandQueue() が必要です
	HRESULT hr = dxgiFactory_->CreateSwapChainForHwnd(
		commandListManager_.GetGraphicsQueue().GetD3D12CommandQueue(),
		static_cast<HWND>(windowHandle),
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain1);
	assert(SUCCEEDED(hr));

	hr = swapChain1.As(&m_SwapChain);
	assert(SUCCEEDED(hr));

	// =============================================================
	// 6. バックバッファと深度バッファの初期化
	// =============================================================
	for (uint32_t i = 0; i < BufferCount; ++i)
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> backBuffer;
		m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));

		// ここでColorBufferにラップする！
		m_DisplayPlane[i].CreateFromSwapChain(L"Primary SwapChain Buffer", backBuffer.Get());

		// クリアカラーをセット（コーンフラワーブルー）
		m_DisplayPlane[i].SetClearColor(0.39f, 0.58f, 0.93f, 1.0f);
	}

	// 深度バッファ作成
	m_DepthBuffer.Create(L"Scene Depth Buffer", width, height, DXGI_FORMAT_D32_FLOAT);
}

void GraphicsCore::Shutdown() {
#ifdef _DEBUG
	infoQueue_.Reset();
	debugController_.Reset();
#endif
	device_.Reset();
	dxgiFactory_.Reset();
}

//void GraphicsCore::Shutdown()
//{
//	// 1. GPUの処理完了を待機
//	// これをしないと、まだ使用中のリソースを破壊してクラッシュする可能性があります
//	// ※CommandQueueに「アイドル待機」機能があればそれを呼びますが、
//	//   簡易的には直前のフレームのフェンス完了を待つことで代用します。
//	//   （本当は CommandListManager::IdleGPU() のような関数を作って呼ぶのがベスト）
//	commandListManager_.GetGraphicsQueue().WaitForFence(commandListManager_.GetGraphicsQueue().GetNextFenceValue() - 1);
//
//	// 2. リソースの破棄 (GPUが触らなくなったので安全)
//	for (auto& buffer : m_DisplayPlane) buffer.Destroy();
//	m_DepthBuffer.Destroy();
//
//	// 3. アロケータの破棄
//	m_RTVAllocator.Shutdown();
//	m_DSVAllocator.Shutdown();
//	m_SRVAllocator.Shutdown();
//
//	// 4. スワップチェーンの破棄 (Queueより先に消す)
//	m_SwapChain.Reset();
//
//	// 5. コマンドマネージャの破棄 (Queueの解放)
//	commandListManager_.Shutdown();
//
//	// 6. デバイス・ファクトリーの破棄
//	device_.Reset();
//	dxgiFactory_.Reset();
//
//#ifdef _DEBUG
//	// 7. リソースリークの報告 (これができるとプロっぽい！)
//	// device_などがResetされた後に残っているオブジェクトがあれば出力ウィンドウに出ます
//	{
//		Microsoft::WRL::ComPtr<ID3D12DebugDevice> debugDevice;
//		if (SUCCEEDED(debugController_.As(&debugDevice))) {
//			debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
//		}
//	}
//
//	infoQueue_.Reset();
//	debugController_.Reset();
//#endif
//}