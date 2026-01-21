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

void GraphicsCore::Initialize(HWND windowHandle, int width, int height) {
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
	m_RTVAllocator_.Create(device_.Get());
	m_DSVAllocator_.Create(device_.Get());
	m_SRVAllocator_.Create(device_.Get());

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
	hr_ = dxgiFactory_->CreateSwapChainForHwnd(
		commandListManager_.GetGraphicsQueue().GetD3D12CommandQueue(),
		static_cast<HWND>(windowHandle),
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain1);
	assert(SUCCEEDED(hr_));

	hr_ = swapChain1.As(&m_SwapChain_);
	assert(SUCCEEDED(hr_));

	// =============================================================
	// 6. バックバッファと深度バッファの初期化
	// =============================================================
	for (uint32_t i = 0; i < BufferCount; ++i) {
		Microsoft::WRL::ComPtr<ID3D12Resource> backBuffer;
		m_SwapChain_->GetBuffer(i, IID_PPV_ARGS(&backBuffer));

		// ColorBufferにラップする
		m_DisplayPlane_[i].CreateFromSwapChain(L"Primary SwapChain Buffer", backBuffer.Get());

		// 画面クリア時の色
		m_DisplayPlane_[i].SetClearColor(0.39f, 0.58f, 0.93f, 1.0f);

		m_DisplayPlane_[i].SetUsageState(D3D12_RESOURCE_STATE_PRESENT);
	}

	// 深度バッファ作成
	m_DepthBuffer_.Create(L"Scene Depth Buffer", width, height, DXGI_FORMAT_D32_FLOAT);
}

void GraphicsCore::Present() {
	// ================================
	// 1. スワップチェーンをフリップ
	// ================================
	HRESULT hr = m_SwapChain_->Present(1, 0);
	if (FAILED(hr)) {
		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
			// デバイスロスト時の処理が必要だが、現状はアサート
			assert(false && "Device Lost");
		}
	}

	// ===============================================
	// 2. 前フレームの完了を待つのではなく、現在のフレームの完了を待つ
	// ===============================================
	// ※ ExecuteCommandListで既にフェンスがシグナルされているので、
	//    その完了を待つだけで良い

	CommandQueue& graphicsQueue = commandListManager_.GetGraphicsQueue();

	// 現在の完了フェンス値を取得（ExecuteCommandListで既にシグナル済み）
	// 特に追加でシグナルする必要はない

	// 3. 次のバックバッファ番号を取得
	m_CurrentBackBufferIndex_ = m_SwapChain_->GetCurrentBackBufferIndex();
}

void GraphicsCore::Shutdown() {
	// GPUの処理完了を待機 (アイドル状態にする)
	commandListManager_.GetGraphicsQueue().WaitForIdle();

	// リソースの破棄
	// ComPtrは自動解放されるが、明示的に行うことで順序を制御できる
	for (auto& buffer : m_DisplayPlane_) buffer.Destroy();
	m_DepthBuffer_.Destroy();

	m_RTVAllocator_.Shutdown();
	m_DSVAllocator_.Shutdown();
	m_SRVAllocator_.Shutdown();

	commandListManager_.Shutdown();

	m_SwapChain_.Reset();
	dxgiFactory_.Reset();

#ifdef _DEBUG
	// リソースリークチェック
	{
		Microsoft::WRL::ComPtr<ID3D12DebugDevice> debugDevice;
		if (SUCCEEDED(device_.As(&debugDevice))) {
			debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
		}
	}
	infoQueue_.Reset();
	debugController_.Reset();
#endif
	device_.Reset();
}