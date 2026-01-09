#include <Windows.h>
#include <cstdint>
#include <string>
#include <format>

#include "TomoEngine.h"

#include <numbers>
#include<cassert>
#include <vector>

#include "wrl.h"

struct Material {
	Vector4 color; // RGBA
	int enableLighting; // ライティングを有効にするか
	float padding[3]; // 12 bytes (16-byte alignment)
	Matrix4x4 uvTransform; // UV変換行列
};

struct TransformationMatrix {
	Matrix4x4 WVP; // ワールド×ビュー×プロジェクション行列
	Matrix4x4 World; // ワールド行列
};

struct DirectionalLight {
	Vector4 color;    // ライトの色
	Vector3 direction; // ライトの向き（単位ベクトル）
	float intensity;  // ライトの強度
	float padding[3];  // 12 bytes (16-byte境界合わせ)
};

Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(const Microsoft::WRL::ComPtr<ID3D12Device>& device, size_t sizeInBytes) {
	assert(device != nullptr);

	// アップロードヒープの設定
	D3D12_HEAP_PROPERTIES heapProperties = {};
	heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD; // CPUからGPUへ書き込み

	// バッファリソースの設定（引数サイズを使用）
	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Alignment = 0;
	resDesc.Width = static_cast<UINT64>(sizeInBytes);
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.Format = DXGI_FORMAT_UNKNOWN;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, // Uploadヒープは通常GenericRead開始
		nullptr,
		IID_PPV_ARGS(&resource)
	);
	assert(SUCCEEDED(hr));
	if (FAILED(hr)) {
		return nullptr;
	}
	return resource;
}

// ウィンドウプロシージャ
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg,
	WPARAM wparam, LPARAM lpram) {

	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lpram)) {
		return true;
	}

	// メッセージに応じた処理
	switch (msg) {
	case WM_DESTROY: // ウィンドウが破棄された
		PostQuitMessage(0); // OSに対して、アプリ終了を伝える
		return 0;
	}
	return DefWindowProcW(hwnd, msg, wparam, lpram);
}

Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(
	// compilerするShaderファイルへのパス
	const std::wstring& filePath,
	// compilerに使用するProfile
	const wchar_t* profile,
	// 初期化で生成したものを3つ
	IDxcUtils* dxcUtils,
	IDxcCompiler3* dxcCompiler,
	IDxcIncludeHandler* includeHandler
) {
	// ========================================
	// 1.hlslファイルの読み込み
	// ========================================
	Log(ConvertString(std::format(L"Begin CompileShader, path:{},profile:{}\n", filePath, profile)));

	Microsoft::WRL::ComPtr<IDxcBlobEncoding> shaderSource = nullptr;
	HRESULT hr = dxcUtils->LoadFile(
		filePath.c_str(),
		nullptr,
		&shaderSource
	);
	assert(SUCCEEDED(hr));

	DxcBuffer shaderSourceBuffer;
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
	shaderSourceBuffer.Encoding = DXC_CP_ACP;

	// ========================================
	// 2.Compileする
	// ========================================
	LPCWSTR arguments[] = {
		filePath.c_str(),
		L"-E", L"main",
		L"-T", profile,
		L"-Zi",	L"-Qembed_debug",
		L"-Od",
		L"-Zpr",
	};

	Microsoft::WRL::ComPtr<IDxcResult> shaderResult = nullptr;
	hr = dxcCompiler->Compile(
		&shaderSourceBuffer,
		arguments,
		_countof(arguments),
		includeHandler,
		IID_PPV_ARGS(&shaderResult)
	);
	assert(SUCCEEDED(hr));

	// ========================================
	// 3.警告・エラーの確認
	// ========================================
	Microsoft::WRL::ComPtr<IDxcBlobUtf8> shaderError = nullptr;
	shaderResult->GetOutput(
		DXC_OUT_ERRORS,
		IID_PPV_ARGS(&shaderError),
		nullptr
	);

	if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
		Log(shaderError->GetStringPointer());
		assert(false);
	}

	// ========================================
	// 4.コンパイル結果を受け取って返す
	// ========================================
	Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob = nullptr;
	hr = shaderResult->GetOutput(
		DXC_OUT_OBJECT,
		IID_PPV_ARGS(&shaderBlob),
		nullptr
	);
	assert(SUCCEEDED(hr));

	Log(ConvertString(std::format(L"Compile Succeeded,path:{},profile:{}\n", filePath, profile)));

	// ComPtrなので自動的にReleaseされる
	return shaderBlob;
}

// ===========================================
// テクスチャ制御
// ===========================================
// テクスチャを読み込む
DirectX::ScratchImage LoadTexture(const std::string& filePath) {
	// テクスチャファイルを読んでプログラムを扱えるようにする
	DirectX::ScratchImage image{};
	std::wstring filePathW = ConvertString(filePath);
	HRESULT hr = DirectX::LoadFromWICFile(
		filePathW.c_str(),
		DirectX::WIC_FLAGS_FORCE_SRGB,
		nullptr,
		image
	);
	assert(SUCCEEDED(hr));

	// ミニマップの作成
	DirectX::ScratchImage mipImages{};
	hr = DirectX::GenerateMipMaps(
		image.GetImages(),
		image.GetImageCount(),
		image.GetMetadata(),
		DirectX::TEX_FILTER_SRGB,
		0,
		mipImages
	);
	assert(SUCCEEDED(hr));

	// ミニマップ付きのデータを返す
	return mipImages;
}

[[nodiscard]]
Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(const Microsoft::WRL::ComPtr<ID3D12Resource>& texture, const DirectX::ScratchImage& mipImages, const Microsoft::WRL::ComPtr<ID3D12Device>& device,
	const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& commandList) {
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	DirectX::PrepareUpload(device.Get(), mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subresources);
	uint64_t intermediateSize = GetRequiredIntermediateSize(texture.Get(), 0, UINT(subresources.size()));
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource = CreateBufferResource(device.Get(), intermediateSize);
	UpdateSubresources(commandList.Get(), texture.Get(), intermediateResource.Get(), 0, 0, UINT(subresources.size()), subresources.data());

	// Tetureへの転送後は利用できるよう、D3D12_RESOURCE_STATE_COPY_DESTからD3D12_RESOURCE_STATE_GENERIC_READへResourceStateを変更する
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = texture.Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	commandList->ResourceBarrier(1, &barrier);

	return intermediateResource;
}

Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(const Microsoft::WRL::ComPtr<ID3D12Device>& device, const DirectX::TexMetadata& metadata) {
	/* 1. metadataをもとにリソース設定を行う
	--------------------------------------*/
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(metadata.width);					// Textureの幅
	resourceDesc.Height = UINT(metadata.height);				// Textureの高さ
	resourceDesc.MipLevels = UINT16(metadata.mipLevels);		// ミップマップのレベル数
	resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize);	// 奥行き or 配列Textureの配列数
	resourceDesc.Format = metadata.format;						// 色フォーマット
	resourceDesc.SampleDesc.Count = 1;							// サンプリング数(1固定)
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension); // 2D Texture

	/* 2. 利用するヒープの設定
	----------------------------------------*/
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // 細かい設定を行う

	/* 3. リソースの生成
	---------------------------------------*/
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr; // ComPtrで受け取る
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,					// ヒープ設定
		D3D12_HEAP_FLAG_NONE,				// ヒープフラグ
		&resourceDesc,						// リソース設定
		D3D12_RESOURCE_STATE_COPY_DEST,	// リソースの使用法指定
		nullptr,							// 最初のリソース状態用のクリア値(テクスチャでは不要)
		IID_PPV_ARGS(&resource)				// 作成するリソースへのポインタ
	);
	assert(SUCCEEDED(hr));
	return resource;
}

// テクスチャへデータ転送
void UploadTextureData(const Microsoft::WRL::ComPtr<ID3D12Resource>& texture, const DirectX::ScratchImage& mipImages) {
	// Meta情報を取得
	const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
	// 全ミップマップ分ループ
	for (size_t mipLevel = 0; mipLevel < metadata.mipLevels; ++mipLevel) {
		// ミップマップレベルに応じたImage情報を取得
		const DirectX::Image* img = mipImages.GetImage(mipLevel, 0, 0);

		// テクスチャへデータ転送
		HRESULT hr = texture->WriteToSubresource(
			UINT(mipLevel),		// ミップマップレベル
			nullptr,			// 全領域へコピー
			img->pixels,		// 転送元データポインタ
			UINT(img->rowPitch),	// 1ラインのサイズ
			UINT(img->slicePitch)	// 1枚のサイズ
		);
		assert(SUCCEEDED(hr));
	}
}

// DepthStencilTexture(奥行きの根幹をなすもの)
Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStancilTextureResourece(const Microsoft::WRL::ComPtr<ID3D12Device>& device, int32_t width, int32_t height) {
	// 生成するResourceの設定
	D3D12_RESOURCE_DESC resourceDesc{}; // ComPtrではなく通常の構造体にする
	resourceDesc.Width = width;					// Textureの幅
	resourceDesc.Height = height;				// Textureの高さ
	resourceDesc.MipLevels = 1;					// ミップマップのレベル数
	resourceDesc.DepthOrArraySize = 1;			// 奥行き or 配列Textureの配列数
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;	// 色フォーマット
	resourceDesc.SampleDesc.Count = 1;			// サンプリング数(1固定)
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // 2D Texture
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // DepthStencilとして使う通知

	// 利用するヒープの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // VRAM上に作る

	// 深度値のクリア最適化設定
	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f; // 深度クリア値(最遠)
	depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // フォーマット

	// リソースの生成
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr; // ComPtrで受け取る
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,					// ヒープ設定
		D3D12_HEAP_FLAG_NONE,				// ヒープフラグ
		&resourceDesc,						// リソース設定
		D3D12_RESOURCE_STATE_DEPTH_WRITE,	// 深度値書き込みとして使用する通知
		&depthClearValue,					// クリア最適化設定
		IID_PPV_ARGS(&resource)				// 作成するリソースへのポインタ
	);
	assert(SUCCEEDED(hr));
	return resource;
}

D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(
	ID3D12DescriptorHeap* descriptorHeap,
	uint32_t descriptorSize,
	uint32_t index
) {
	// デスクリプタヒープの先頭ハンドルを取得
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	// index分だけオフセットを加算
	handleCPU.ptr += UINT64(descriptorSize) * index;
	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(
	ID3D12DescriptorHeap* descriptorHeap,
	uint32_t descriptorSize,
	uint32_t index
) {
	// デスクリプタヒープの先頭ハンドルを取得
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	// index分だけオフセットを加算
	handleGPU.ptr += UINT64(descriptorSize) * index;
	return handleGPU;
}

// 球体を表す構造体
struct Sphere {
	Vector3 center; //!< 中心点
	float radius;   //!< 半径
	Vector3 rotation; //!< 回転角
};

struct D3DResourceLeakChecker {
	~D3DResourceLeakChecker() {
		// リソースリークチェック
		Microsoft::WRL::ComPtr<IDXGIDebug1> debug;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
			debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
		}
	}
};

static size_t Align256(size_t size) {
	return (size + 255) & ~size_t(255);
}

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	D3DResourceLeakChecker leakChecker;

	CoInitializeEx(0, COINIT_MULTITHREADED);

	WNDCLASS wc{  };

	wc.lpfnWndProc = WindowProc;
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

	ShowWindow(hwnd, SW_SHOW);

#ifdef _DEBUG
	Microsoft::WRL::ComPtr<ID3D12Debug1> debugController = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		// デバッグレイヤーを有効化
		debugController->EnableDebugLayer();
		// さらにGPU側でもチェックを行うようにする
		debugController->SetEnableGPUBasedValidation(TRUE);
	}
#endif

	/////////////////////////////////////////////
	// デバイスの設定
	////////////////////////////////////////////

	// ==============================================
	// DXGIファクトリーの生成
	// ==============================================
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory = nullptr;

	HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));

	assert(SUCCEEDED(hr));

	// ==============================================
	// アダプタ(GPU)を決定する
	// ==============================================
	Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter = nullptr;

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
	Microsoft::WRL::ComPtr<ID3D12Device> device = nullptr;

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
			useAdapter.Get(),				// 利用するアダプター
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

#ifdef _DEBUG
	ID3D12InfoQueue* infoQueue = nullptr;
	if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
		// やばいエラー時止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);

		// エラー時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);

		// 警告時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

		// 抑制するメッセージID
		D3D12_MESSAGE_ID denyIds[] = {
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
		};

		// 抑制するレベル
		D3D12_MESSAGE_SEVERITY severities[] = {
			D3D12_MESSAGE_SEVERITY_INFO
		};

		D3D12_INFO_QUEUE_FILTER filter = {};
		filter.DenyList.NumIDs = _countof(denyIds);
		filter.DenyList.pIDList = denyIds;
		filter.DenyList.NumSeverities = _countof(severities);
		filter.DenyList.pSeverityList = severities;

		// 指定したメッセージの表示を抑制する
		infoQueue->PushStorageFilter(&filter);

		// 解放
		infoQueue->Release();
	}
#endif

	// ===========================================
	// コマンドキューを生成する
	// ===========================================
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue = nullptr;
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
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator = nullptr;
	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	assert(SUCCEEDED(hr));

	// コマンドリストを作成する
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList = nullptr;
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList));

	// コマンドリストの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));

	hr = commandList->Close();
	assert(SUCCEEDED(hr));

	// ============================================
	// スワップチェーンを作成する
	// ============================================
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain = nullptr;
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
		commandQueue.Get(),		// コマンドキュー
		hwnd,				// ウィンドウハンドル
		&swapChainDesc,		// スワップチェーンの設定
		nullptr,			// フルスクリーン設定
		nullptr,			// 制限
		reinterpret_cast<IDXGISwapChain1**>(swapChain.GetAddressOf())); // 作成するスワップチェーンのポインタ

	assert(SUCCEEDED(hr));

	// ==================================
	// Discripter Heapの生成
	// ==================================
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap = CreateDescriptorHeap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
	//D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc = {};
	//rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; // レンダーターゲットビュー
	//rtvDescriptorHeapDesc.NumDescriptors = 2; // ダブルバッファリングなので2つ
	//hr = device->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap));

	// ディスクリプタヒープの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));

	// SwapChainからResourceを引っ張てくる
	Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResources[2] = { nullptr };
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
	device->CreateRenderTargetView(swapChainResources[0].Get(), &rtvDesc, rtvHandles[0]);

	// 二つ目のハンドルを計算
	rtvHandles[1].ptr = rtvHandles[0].ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// 二つ目のRTVを作成
	device->CreateRenderTargetView(swapChainResources[1].Get(), &rtvDesc, rtvHandles[1]);

	// ==================================

	//typedef struct D3D12_CPU_DESCRITOR_HANDLE {
	//	SIZE_T ptr;
	//} D3D12_CPU_DESCRIPTOR_HANDLE;

	// ==================================
	// FenceとEventの生成
	// ==================================
	Microsoft::WRL::ComPtr<ID3D12Fence> fence = nullptr;
	uint64_t fenceValue = 0;
	hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	assert(SUCCEEDED(hr));

	// FenceのSingnalを待つためのEventを生成
	HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	assert(fenceEvent != nullptr);

	// ==================================
	// DXCの初期化
	// ==================================
	// dxcCompilerを初期化
	Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils = nullptr;
	Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler = nullptr;
	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
	assert(SUCCEEDED(hr));
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
	assert(SUCCEEDED(hr));

	// 現時点でincludeはしないが、includeに対応するための設定を行っておく
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler = nullptr;
	hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
	assert(SUCCEEDED(hr));

	// ==================================
	// Textureを読んで転送する（Copy命令をコマンドに積む→実行→完了待ち）
	// ==================================
	DirectX::ScratchImage mipImages = LoadTexture("resources/uvChecker.png");
	const DirectX::TexMetadata& metadata = mipImages.GetMetadata();

	// ResourceObject で受け取る
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource = CreateTextureResource(device, metadata);

	// 二枚目のTextureを読んで転送する（Copy命令をコマンドに積む→実行→完了待ち）
	DirectX::ScratchImage mipImages2 = LoadTexture("resources/monsterBall.png");
	const DirectX::TexMetadata& metadata2 = mipImages2.GetMetadata();

	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource2 = CreateTextureResource(device, metadata2);

	// テクスチャ転送コマンドを積むために、コマンドリストを開く
	hr = commandAllocator->Reset();
	assert(SUCCEEDED(hr));
	hr = commandList->Reset(commandAllocator.Get(), nullptr);
	assert(SUCCEEDED(hr));

	// Upload命令を積む（IntermediateResource を受け取って保持する）
	ResourceObject intermediateResource = UploadTextureData(textureResource, mipImages, device, commandList);

	// 2枚目
	ResourceObject intermediateResource2 = UploadTextureData(textureResource2, mipImages2, device, commandList);

	// コマンドを閉じて実行
	hr = commandList->Close();
	assert(SUCCEEDED(hr));

	ID3D12CommandList* commandLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(1, commandLists);

	// 実行完了を待つ（ここで GPU がコピーを終える）
	fenceValue++;
	hr = commandQueue->Signal(fence.Get(), fenceValue);
	assert(SUCCEEDED(hr));
	if (fence->GetCompletedValue() < fenceValue) {
		hr = fence->SetEventOnCompletion(fenceValue, fenceEvent);
		assert(SUCCEEDED(hr));
		WaitForSingleObject(fenceEvent, INFINITE);
	}

	// ==================================
	// RootSignatureの生成
	// ==================================
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature = {};
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// =================================
	// DescriptorRangeの生成
	// =================================
	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0;
	descriptorRange[0].NumDescriptors = 1;
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// =================================
	// Sampler の設定
	// =================================
	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;          // バイリニア
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;        // 0〜1 外リピート
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;      // 比較しない
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;                        // ありったけの mip
	staticSamplers[0].ShaderRegister = 0;                                // s0 を使う
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;  // PixelShader で使う

	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

	// =================================
	// RootParameter作成
	// =================================
	D3D12_ROOT_PARAMETER rootParameters[4] = {};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // 定数バッファビュー　// b0のbと一致する
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // ピクセルシェーダーで使う
	rootParameters[0].Descriptor.ShaderRegister = 0; // レジスタ番号0 // b0の0と一致する　もしb11と紐づけたいなら11にする

	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // 定数バッファビュー　// b1のbと一致する
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // 頂点シェーダーで使う
	rootParameters[1].Descriptor.ShaderRegister = 0; // レジスタ番号

	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // ディスクリプタテーブルを使用
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // ピクセルシェーダーで使う
	rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;	// Tableの中身の配列を指定
	rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange); // 配列の長さ

	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使用
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // 頂点シェーダーで使う
	rootParameters[3].Descriptor.ShaderRegister = 1; // レジスタ番号1


	descriptionRootSignature.pParameters = rootParameters; // ルートパラメータ配列へのポインタ
	descriptionRootSignature.NumParameters = _countof(rootParameters); // ルートパラメータの配列の長さ

	//WVP用のリソースを作る
	ResourceObject wvpResource = CreateBufferResource(device, Align256(sizeof(TransformationMatrix)));
	// データを書き込む
	TransformationMatrix* wvpData = nullptr;
	wvpResource.Get()->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));
	wvpData->WVP = Matrix4x4::MakeIdentity4x4();
	wvpData->World = Matrix4x4::MakeIdentity4x4();

	// シリアライズしてバイナリにする
	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
	hr = D3D12SerializeRootSignature(
		&descriptionRootSignature,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&signatureBlob,
		&errorBlob
	);

	if (FAILED(hr)) {
		// エラーがあったらログに出す
		Log(ConvertString(static_cast<const char*>(errorBlob->GetBufferPointer())));
		assert(false);
	}

	// バイナリをもとに生成
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	hr = device->CreateRootSignature(
		0,
		signatureBlob->GetBufferPointer(),
		signatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature)
	);
	assert(SUCCEEDED(hr));


	// ==================================
	// InputLayoutの設定
	// ==================================
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	inputElementDescs[1].SemanticName = "TEXCOORD";
	inputElementDescs[1].SemanticIndex = 0;
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	inputElementDescs[2].SemanticName = "NORMAL";
	inputElementDescs[2].SemanticIndex = 0;
	inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);

	// ==================================
	// BlendStateの設定
	// ==================================
	D3D12_BLEND_DESC blendDesc = {};
	// 全ての色要素を書き込む
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	// ==================================
	// RasterizerStateの設定
	// ==================================
	D3D12_RASTERIZER_DESC rasterizerDesc = {};
	// 裏面をカリングする
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	// 三角形の中を塗りつぶす
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	// ==================================
	// Shaderのコンパイル
	//===================================
	Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = CompileShader(L"Object3D.VS.hlsl",
		L"vs_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get());
	assert(vertexShaderBlob != nullptr);

	Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = CompileShader(L"Object3D.PS.hlsl",
		L"ps_6_0", dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get());
	assert(pixelShaderBlob != nullptr);

	// =================================
	// PSOを生成する
	// =================================
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc = {};
	graphicsPipelineStateDesc.pRootSignature = rootSignature.Get(); // ルートシグネチャ
	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc; // InputLayout
	graphicsPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(),vertexShaderBlob->GetBufferSize() }; // 頂点シェーダー
	graphicsPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(),pixelShaderBlob->GetBufferSize() }; // ピクセルシェーダー
	graphicsPipelineStateDesc.BlendState = blendDesc; // ブレンドステート
	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc; // ラスタライザーステート
	// 書き込むRTVの情報
	graphicsPipelineStateDesc.NumRenderTargets = 1;
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	// === DepthStencilの設定 ===
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
	// Depthの機能を有効化する
	depthStencilDesc.DepthEnable = true;
	// 書き込みも有効化
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	// 比較関数はLessEqual -> 近ければ描画される
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	// POSに代入しさらにDSVのformatを設定する
	graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// 利用するトポロジ(形状)のタイプ。三角形
	graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	graphicsPipelineStateDesc.SampleDesc.Count = 1; 	// どのように画面に色を打ち込むのかの設定
	graphicsPipelineStateDesc.SampleDesc.Quality = 0;
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	// 実際に生成
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState = nullptr;
	hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));
	assert(SUCCEEDED(hr));

	// ==================================
	// VertexResourceの生成
	// ==================================
	// 頂点リソース用のヒープ設定
	D3D12_HEAP_PROPERTIES uploadHeapProperties = {};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD; // UploadHeapを使う
	// 頂点リソースの設定
	D3D12_RESOURCE_DESC vertexResourceDesc = {};
	// バッファリソース。テクスチャの場合はまた別の設定をする
	vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vertexResourceDesc.Width = sizeof(VertexData) * 6; // 頂点データ全体のサイズ
	vertexResourceDesc.Height = 1; // バッファなので高さは1
	vertexResourceDesc.DepthOrArraySize = 1; // バッファなので深度は1
	vertexResourceDesc.MipLevels = 1; // バッファなのでミップレベルは1
	vertexResourceDesc.SampleDesc.Count = 1; // バッファなのでサンプル数は1
	// バッファの場合はこれにする決まり
	vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	// 頂点リソースを生成
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource = nullptr;
	hr = device->CreateCommittedResource(
		&uploadHeapProperties, // ヒープ設定
		D3D12_HEAP_FLAG_NONE,
		&vertexResourceDesc, // リソース設定
		D3D12_RESOURCE_STATE_GENERIC_READ, // アップロードに使うのでGenericRead
		nullptr,
		IID_PPV_ARGS(&vertexResource)
	);
	assert(SUCCEEDED(hr));



	// ==================================
	// 平行光源用リソースの生成
	// ==================================
	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource = CreateBufferResource(device, Align256(sizeof(DirectionalLight)));
	// データを書き込む
	DirectionalLight* directionalLightData = nullptr;
	// 書き込むためのアドレスを取得
	directionalLightResource->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));
	// 平行光源の情報を設定
	directionalLightData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f); // 白色
	directionalLightData->direction = Vector3(0.0f, -1.0f, 0.0f);
	directionalLightData->intensity = 1.0f;

	// ==================================
	// Sprite用のResourceの生成
	// ==================================
	ResourceObject vertexResourceSprite = CreateBufferResource(device, sizeof(VertexData) * 6); // 頂点6つ分のサイズ

	// 頂点バッファビューを作成する
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite = {};
	// リソースの先頭のアドレスから使う
	vertexBufferViewSprite.BufferLocation = vertexResourceSprite.Get()->GetGPUVirtualAddress();
	// 使用するリソースのサイズは頂点6つ分のサイズ
	vertexBufferViewSprite.SizeInBytes = sizeof(VertexData) * 6;
	// 1頂点あたりのサイズ
	vertexBufferViewSprite.StrideInBytes = sizeof(VertexData);

	/*頂点データを設定する*/
	VertexData* vertexDataSprite = nullptr;
	vertexResourceSprite.Get()->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSprite));
	// 一枚目の三角形
	vertexDataSprite[0].position = { 0.0f, 360.0f, 0.0f, 1.0f }; // 左下
	vertexDataSprite[0].texcoord = { 0.0f, 1.0f };
	vertexDataSprite[1].position = { 0.0f, 0.0f, 0.0f, 1.0f }; // 左上
	vertexDataSprite[1].texcoord = { 0.0f, 0.0f };
	vertexDataSprite[2].position = { 640.0f, 360.0f, 0.0f, 1.0f }; // 右下
	vertexDataSprite[2].texcoord = { 1.0f, 1.0f };

	// 二枚目の三角形
	vertexDataSprite[3].position = { 0.0f, 0.0f, 0.0f, 1.0f }; // 左上
	vertexDataSprite[3].texcoord = { 0.0f, 0.0f };
	vertexDataSprite[4].position = { 640.0f, 0.0f, 0.0f, 1.0f }; // 右上
	vertexDataSprite[4].texcoord = { 1.0f, 0.0f };
	vertexDataSprite[5].position = { 640.0f, 360.0f, 0.0f, 1.0f }; // 右下
	vertexDataSprite[5].texcoord = { 1.0f, 1.0f };

	// Sprite用のTransformationMatrix用のリソースを作る
	//ID3D12Resource* transformationMatrixResourceSprite = CreateBufferResource(device.Get(), sizeof(Matrix4x4));
	ResourceObject transformationMatrixResourceSprite = CreateBufferResource(device, Align256(sizeof(Matrix4x4)));
	Matrix4x4* transformationMatrixDataSprite = nullptr;
	// 書き込むためのアドレスを取得
	transformationMatrixResourceSprite.Get()->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixDataSprite));
	// 単位行列を書き込む
	*transformationMatrixDataSprite = Matrix4x4::MakeIdentity4x4();

	// CPUで動かす用の行列を用意
	Transform transformSprite{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };

	// ==================================
	// Sphere用のResourceの生成
	// ==================================
	// 分割数
	const uint32_t kSubdivision = 16;
	// 頂点数 (分割数 * 分割数 * 6頂点)
	const uint32_t kSphereVertexCount = kSubdivision * kSubdivision * 6;

	// 頂点リソースの作成
	ResourceObject vertexResourceSphere = CreateBufferResource(device, sizeof(VertexData) * kSphereVertexCount);

	// 頂点バッファビューを作成する
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSphere = {};
	vertexBufferViewSphere.BufferLocation = vertexResourceSphere.Get()->GetGPUVirtualAddress();
	vertexBufferViewSphere.SizeInBytes = sizeof(VertexData) * kSphereVertexCount;
	vertexBufferViewSphere.StrideInBytes = sizeof(VertexData);

	// 頂点データを書き込む
	VertexData* vertexDataSphere = nullptr;
	vertexResourceSphere.Get()->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSphere));

	// 経度分割1つ分の角度 phi
	const float kLonEvery = 2.0f * std::numbers::pi_v<float> / float(kSubdivision);
	// 緯度分割1つ分の角度 theta
	const float kLatEvery = std::numbers::pi_v<float> / float(kSubdivision);

	// 緯度の方向に分割
	for (uint32_t latIndex = 0; latIndex < kSubdivision; ++latIndex) {
		float lat = -std::numbers::pi_v<float> / 2.0f + kLatEvery * latIndex; // theta

		// 経度の方向に分割
		for (uint32_t lonIndex = 0; lonIndex < kSubdivision; ++lonIndex) {
			float lon = lonIndex * kLonEvery; // phi

			// 書き込む頂点の先頭インデックス
			uint32_t start = (latIndex * kSubdivision + lonIndex) * 6;

			// 基準点a, b, c, dの計算に必要な角度
			float lat_b = lat + kLatEvery;
			float lon_c = lon + kLonEvery;

			// UV座標の計算用
			float u_curr = float(lonIndex) / float(kSubdivision);
			float u_next = float(lonIndex + 1) / float(kSubdivision);
			float v_curr = 1.0f - float(latIndex) / float(kSubdivision);
			float v_next = 1.0f - float(latIndex + 1) / float(kSubdivision);

			// 頂点a (左下)
			vertexDataSphere[start].position.x = std::cos(lat) * std::cos(lon);
			vertexDataSphere[start].position.y = std::sin(lat);
			vertexDataSphere[start].position.z = std::cos(lat) * std::sin(lon);
			vertexDataSphere[start].position.w = 1.0f;
			vertexDataSphere[start].texcoord = { u_curr, v_curr };
			vertexDataSphere[start].normal = { vertexDataSphere[start].position.x, vertexDataSphere[start].position.y, vertexDataSphere[start].position.z };

			// 頂点b (左上)
			vertexDataSphere[start + 1].position.x = std::cos(lat_b) * std::cos(lon);
			vertexDataSphere[start + 1].position.y = std::sin(lat_b);
			vertexDataSphere[start + 1].position.z = std::cos(lat_b) * std::sin(lon);
			vertexDataSphere[start + 1].position.w = 1.0f;
			vertexDataSphere[start + 1].texcoord = { u_curr, v_next };
			vertexDataSphere[start + 1].normal = { vertexDataSphere[start + 1].position.x, vertexDataSphere[start + 1].position.y, vertexDataSphere[start + 1].position.z };

			// 頂点c (右下)
			vertexDataSphere[start + 2].position.x = std::cos(lat) * std::cos(lon_c);
			vertexDataSphere[start + 2].position.y = std::sin(lat);
			vertexDataSphere[start + 2].position.z = std::cos(lat) * std::sin(lon_c);
			vertexDataSphere[start + 2].position.w = 1.0f;
			vertexDataSphere[start + 2].texcoord = { u_next, v_curr };
			vertexDataSphere[start + 2].normal = { vertexDataSphere[start + 2].position.x, vertexDataSphere[start + 2].position.y, vertexDataSphere[start + 2].position.z };

			// 頂点d (右上) - 2枚目の三角形用
			vertexDataSphere[start + 3] = vertexDataSphere[start + 1]; // b
			vertexDataSphere[start + 4].position.x = std::cos(lat_b) * std::cos(lon_c);
			vertexDataSphere[start + 4].position.y = std::sin(lat_b);
			vertexDataSphere[start + 4].position.z = std::cos(lat_b) * std::sin(lon_c);
			vertexDataSphere[start + 4].position.w = 1.0f;
			vertexDataSphere[start + 4].texcoord = { u_next, v_next };
			vertexDataSphere[start + 4].normal = { vertexDataSphere[start + 4].position.x, vertexDataSphere[start + 4].position.y, vertexDataSphere[start + 4].position.z };
			vertexDataSphere[start + 5] = vertexDataSphere[start + 2]; // c
		}
	}
	// 書き込み終了
	vertexResourceSphere.Get()->Unmap(0, nullptr);

	// Sphere用のTransformationMatrix用のリソースを作る
	ResourceObject wvpResourceSphere = CreateBufferResource(device, Align256(sizeof(TransformationMatrix)));
	TransformationMatrix* wvpDataSphere = nullptr;
	wvpResourceSphere.Get()->Map(0, nullptr, reinterpret_cast<void**>(&wvpDataSphere));
	wvpDataSphere->WVP = Matrix4x4::MakeIdentity4x4();
	wvpDataSphere->World = Matrix4x4::MakeIdentity4x4();

	// ==================================
	// DepthStencil用のResourceの生成
	// ==================================
	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource = CreateDepthStancilTextureResourece(device, kClientWidth, kClientHeight);

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap = CreateDescriptorHeap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

	// DepthStencilView(DSV)の設定
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;			// フォーマット
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;	// 2Dテクスチャ

	// DepthStencilViewの生成
	device->CreateDepthStencilView(depthStencilResource.Get(), &dsvDesc, dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// ==================================
	// Material用のResourceの生成
	// ==================================
	ResourceObject materialResource = CreateBufferResource(device, Align256(sizeof(Material))); // RGBA分のサイズ
	//Vector4* materialData = nullptr;
	Material* materialData = nullptr;
	materialResource.Get()->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	// 赤色に設定
	materialData->color = { 1.0f,0.0f,0.0f,1.0f };
	materialData->enableLighting = true;
	materialData->uvTransform = Matrix4x4::MakeIdentity4x4();



	// ==================================
	// Material用のResourceの生成
	// ==================================
	ResourceObject materialResourceSprite = CreateBufferResource(device, Align256(sizeof(Material))); // RGBA分のサイズ
	//Vector4* materialData = nullptr;
	Material* materialDataSprite = nullptr;
	materialResourceSprite.Get()->Map(0, nullptr, reinterpret_cast<void**>(&materialDataSprite));
	// 赤色に設定
	materialDataSprite->color = { 1.0f,1.0f,1.0f,1.0f };
	materialDataSprite->enableLighting = false;
	materialDataSprite->uvTransform = Matrix4x4::MakeIdentity4x4();

	Transform uvTransformSprite{
		{1.0f,1.0f,1.0f},
		{0.0f,0.0f,0.0f},
		{0.0f,0.0f,0.0f},
	};


	// indexResourceの生成
	// ----------------------------------
	ResourceObject indexResource = CreateBufferResource(device, sizeof(uint32_t) * 6); // 32bit*6個分

	// Viewを作成する
	D3D12_INDEX_BUFFER_VIEW indexBufferViewSprite = {};

	// リソースの先頭のアドレスから使う
	indexBufferViewSprite.BufferLocation = indexResource.Get()->GetGPUVirtualAddress();
	// 使用するリソースのサイズは32bit*6個分
	indexBufferViewSprite.SizeInBytes = sizeof(uint32_t) * 6;
	// 1頂点あたりのサイズ
	indexBufferViewSprite.Format = DXGI_FORMAT_R32_UINT;

	// インデックスデータを設定する
	uint32_t* indexDataSprite = nullptr;
	indexResource.Get()->Map(0, nullptr, reinterpret_cast<void**>(&indexDataSprite));
	indexDataSprite[0] = 0;
	indexDataSprite[1] = 1;
	indexDataSprite[2] = 2;
	indexDataSprite[3] = 3;
	indexDataSprite[4] = 4;
	indexDataSprite[5] = 5;

	// ==================================
	// VertexBufferViewを作成する
	// ==================================
	// 頂点バッファビューを作成する
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
	// リソースの先頭のアドレスから使う
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	// 使用するリソースのサイズは頂点3つ分のサイズ
	vertexBufferView.SizeInBytes = sizeof(VertexData) * 6;
	// 1頂点あたりのサイズ
	vertexBufferView.StrideInBytes = sizeof(VertexData);

	// ==================================
	// Resourceにデータを書き込む
	// ==================================
	// 頂点データ
	//Vector4* vertexData = nullptr;
	VertexData* vertexData = nullptr;
	// 書き込むためのアドレスを取得
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	// 左下
	vertexData[0].position = { -0.5f,-0.5f,0.0f,1.0f };
	vertexData[0].texcoord = { 0.0f,1.0f };
	vertexData[0].normal = { 0.0f,0.0f,-1.0f };
	// 上
	vertexData[1].position = { 0.0f,0.5f,0.0f,1.0f };
	vertexData[1].texcoord = { 0.5f,0.0f };
	vertexData[1].normal = { 0.0f,0.0f,-1.0f };
	// 右下
	vertexData[2].position = { 0.5f,-0.5f,0.0f,1.0f };
	vertexData[2].texcoord = { 1.0f,1.0f };
	vertexData[2].normal = { 0.0f,0.0f,-1.0f };

	// 左下2
	vertexData[3].position = { -0.5f, -0.5f, 0.5f, 1.0f };
	vertexData[3].texcoord = { 0.0f, 1.0f };
	vertexData[3].normal = { 0.0f,0.0f,1.0f };

	// 上2
	vertexData[4].position = { 0.0f, 0.0f, 0.0f, 1.0f };
	vertexData[4].texcoord = { 0.5f, 0.0f };
	vertexData[4].normal = { 0.0f,0.0f,1.0f };

	// 右下2
	vertexData[5].position = { 0.5f, -0.5f, -0.5f, 1.0f };
	vertexData[5].texcoord = { 1.0f, 1.0f };
	vertexData[5].normal = { 0.0f,0.0f,1.0f };

	// ==================================
	// ViewportとScissor(シザー)
	// ==================================
	// ビューポート設定
	D3D12_VIEWPORT viewport = {};
	// クライアント領域のサイズと一緒にして画面全体に表示
	viewport.Width = kClientWidth;
	viewport.Height = kClientHeight;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	// シザー矩形設定
	D3D12_RECT scissorRect = {};
	// 基本的にビューポートと同じサイズにする
	scissorRect.left = 0;
	scissorRect.right = kClientWidth;
	scissorRect.top = 0;
	scissorRect.bottom = kClientHeight;

	// ==================================
	// オブジェクト初期化、宣言
	// ==================================
	Transform transform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
	Transform cameraTransform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,-5.0f} };

	// ImGui で操作するマテリアルカラー (RGBA)
	float materialColor[4] = { 1.0f,0.0f,0.0f,1.0f };

	Matrix4x4* transformationMatrixData{};

	Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	Matrix4x4 cameraMatrix = MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
	Matrix4x4 viewMatrix = Matrix4x4::Inverse(cameraMatrix);
	Matrix4x4 projectionMatrix = Matrix4x4::MakeParspectiveFovMatrix(
		0.45f, // 視野角
		static_cast<float>(kClientWidth) / static_cast<float>(kClientHeight), // アスペクト比
		0.1f, // ニアクリップ距離
		100.0f // ファークリップ距離
	);

	// Sphereの初期化
	Sphere sphere;
	sphere.center = { 0.0f, 0.0f, 0.0f };
	sphere.radius = 0.5f;
	sphere.rotation = { 0.0f, 0.0f, 0.0f };

	Matrix4x4 worldViewProjectionMatrix = Matrix4x4::Multiply(worldMatrix, Matrix4x4::Multiply(viewMatrix, projectionMatrix));
	transformationMatrixData = &worldViewProjectionMatrix;

	// ==================================
	// ImGuiの初期化
	// ==================================
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap = CreateDescriptorHeap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX12_Init(
		device.Get(), swapChainDesc.BufferCount, rtvDesc.Format,
		srvDescriptorHeap.Get(), srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart()
	);

	// =================================
	// DescriptorのSizeを取得
	// ================================
	const uint32_t descriptorSizeSRV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	const uint32_t descriptorSizeRTV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	const uint32_t descriptorSizeDSV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	GetCPUDescriptorHandle(rtvDescriptorHeap.Get(), descriptorSizeRTV, 0);

	// ==================================
	// ShaderResourceViewの生成
	// ==================================
	// metaDataをもとにSRVの設定を行う
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
	srvDesc.Texture2D.MipLevels = static_cast<UINT>(metadata.mipLevels);

	// SRVを作成するDescriptorHeapの場所を決める
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU = srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU = srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	// 先頭はImGuiが使うので1つ分ずらす
	textureSrvHandleCPU.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	textureSrvHandleGPU.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	// SRVを生成
	device->CreateShaderResourceView(textureResource.Get(), &srvDesc, textureSrvHandleCPU);


	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2 = {};
	srvDesc2.Format = metadata.format;
	srvDesc2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc2.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
	srvDesc2.Texture2D.MipLevels = static_cast<UINT>(metadata.mipLevels);

	// SRVを作成するDescriptorHeapの場所を決める
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU2 = GetCPUDescriptorHandle(srvDescriptorHeap.Get(), descriptorSizeSRV, 2);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU2 = GetGPUDescriptorHandle(srvDescriptorHeap.Get(), descriptorSizeSRV, 2);
	// SRVを生成
	device->CreateShaderResourceView(textureResource2.Get(), &srvDesc2, textureSrvHandleCPU2);

	// ======================================
	// ゲーム内の変数の初期化、宣言
	// ======================================
	bool useMonsterBall = true;

	// ============================================
	// ゲームループ
	// ============================================
	// ウィンドウのxボタンが押されるまでループ
	while (msg.message != WM_QUIT) {
		// Windowsメッセージが来ていたら最優先で処理させる
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
		else {
			//==================================
			// ImGui フレーム開始
			//==================================
			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();


			// ==================================
			// 画面をクリアする処理が含まれたコマンドリストの記録
			// ==================================
			// これから書き込むバックバッファのインデックスを取得
			UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

			// 次のフレーム用のコマンドリストを準備
			hr = commandAllocator->Reset();
			assert(SUCCEEDED(hr));
			hr = commandList->Reset(commandAllocator.Get(), nullptr);
			assert(SUCCEEDED(hr));

			// TransitionBarrierの設定
			D3D12_RESOURCE_BARRIER barrier{};

			// 今回のバリアはTransition
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			// Noneにしておく
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			// バリアを張るリソースを指定
			barrier.Transition.pResource = swapChainResources[backBufferIndex].Get();
			// 遷移前(現在)のResourcesState
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
			// 遷移後のResourcesState
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
			// TransitionBarrierを張る
			commandList->ResourceBarrier(1, &barrier);

			// 描画先のRTVとDSVを設定
			D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			commandList->OMSetRenderTargets(1, &rtvHandles[swapChain->GetCurrentBackBufferIndex()], false, &dsvHandle);

			// 指定した深度で画面をクリアする
			commandList->ClearDepthStencilView(
				dsvHandle,
				D3D12_CLEAR_FLAG_DEPTH,
				1.0f, // 深度を最大値でクリア
				0,    // ステンシルは使わないので0でクリア
				0, nullptr);

			//==================================
			// ゲームの処理 ↓↓
			//==================================
			transform.rotate.y += 0.03f;
			Matrix4x4 worldMatrix = MakeAffineMatrix(
				transform.scale,
				transform.rotate,
				transform.translate
			);

			wvpData->World = worldMatrix;
			wvpData->WVP = Matrix4x4::Multiply(
				Matrix4x4::Multiply(worldMatrix, viewMatrix),
				projectionMatrix);

			// マテリアルカラーを ImGui から編集
			ImGui::Begin("Material");
			ImGui::ColorEdit4("Color", materialColor); // 0〜1 の RGBA
			ImGui::DragFloat3("SpritePos", &transformSprite.translate.x, 1.0f);
			ImGui::End();

			// Sphereの操作UI
			ImGui::Begin("Sphere Control");
			ImGui::DragFloat3("Center", &sphere.center.x, 0.01f);
			ImGui::DragFloat("Radius", &sphere.radius, 0.01f);
			ImGui::DragFloat3("Rotation", &sphere.rotation.x, 0.1f);

			// 線
			ImGui::Separator();
			ImGui::Checkbox("Use MonsterBall Texture", &useMonsterBall);

			// UVのSRT
			ImGui::DragFloat2("UVTranslate", &uvTransformSprite.translate.x,0.01f,-10.0f,10.0f);
			ImGui::DragFloat2("UVScale", &uvTransformSprite.scale.x, 0.01f, -10.0f, 10.0f);
			ImGui::SliderAngle("UVRotate", &uvTransformSprite.rotate.z);

			ImGui::End();

			// 光源の調整
			ImGui::Begin("Light Control");
			ImGui::ColorEdit4("Light Color", &directionalLightData->color.x);
			ImGui::DragFloat3("Light Direction", &directionalLightData->direction.x, 0.1f);
			ImGui::DragFloat("Light Intensity", &directionalLightData->intensity, 0.1f, 0.0f, 10.0f);
			ImGui::End();

			// 開発用UIの処理
			//ImGui::ShowDemoWindow();

			ImGui::Render();

			// directionalLightData のdirectonを正規化して書き戻す
			directionalLightData->direction = Vector3::Normalize(directionalLightData->direction);

			materialData->color.x = materialColor[0];
			materialData->color.y = materialColor[1];
			materialData->color.z = materialColor[2];
			materialData->color.w = materialColor[3];

			// 描画先のRTVを指定
			/*commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, &dsvHandle);*/

			// 指定した色で画面をクリアする
			float clearColor[] = { 0.1f,0.25f,0.5f,1.0f }; // 青っぽい色 RGBA
			commandList->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0, nullptr);

			Matrix4x4 uvTransformMatrix = MakeScaleMatrix(uvTransformSprite.scale);
			uvTransformMatrix = Matrix4x4::Multiply(uvTransformMatrix, MakeRotateZMatrix(uvTransformSprite.rotate.z));
			uvTransformMatrix = Matrix4x4::Multiply(uvTransformMatrix, MakeTranslateMatrix(uvTransformSprite.translate));
			materialDataSprite->uvTransform = uvTransformMatrix;

			/* Spriteの更新 */
			// 行列計算
			Matrix4x4 worldMatrixSprite = MakeAffineMatrix(
				transformSprite.scale,
				transformSprite.rotate,
				transformSprite.translate
			);
			Matrix4x4 viewMatrixSprite = Matrix4x4::MakeIdentity4x4(); // 視点座標は原点
			Matrix4x4 projectionMatrixSprite = Matrix4x4::MakeOrthographicMatrix(
				0.0f, 0.0f,
				static_cast<float>(kClientWidth),  // 横幅
				static_cast<float>(kClientHeight), // 縦幅
				0.0f,  // ニアクリップ距離
				100.0f // ファークリップ距離
			);

			Matrix4x4 worldViewProjectionMatrixSprite = Matrix4x4::Multiply(
				worldMatrixSprite, Matrix4x4::Multiply(viewMatrixSprite, projectionMatrixSprite));

			*transformationMatrixDataSprite = worldViewProjectionMatrixSprite;

			// =======================================
			// Sphereの更新
			// =======================================
			sphere.rotation.y += 0.02f;

			// 半径をスケール、中心を平行移動としてワールド行列を作成
			Matrix4x4 worldMatrixSphere = MakeAffineMatrix(
				{ sphere.radius, sphere.radius, sphere.radius }, // Scale
				{ sphere.rotation.x,sphere.rotation.y, sphere.rotation.z }, // Rotate
				sphere.center                                    // Translate
			);

			// Sphere: World / WVP を両方更新
			wvpDataSphere->World = worldMatrixSphere;
			wvpDataSphere->WVP = Matrix4x4::Multiply(
				Matrix4x4::Multiply(worldMatrixSphere, viewMatrix),
				projectionMatrix
			);

			// =======================================================
			// コマンドを積む(描画に必要な情報を使って)
			// =======================================================
			commandList->RSSetViewports(1, &viewport);
			commandList->RSSetScissorRects(1, &scissorRect);
			commandList->SetGraphicsRootSignature(rootSignature.Get());
			commandList->SetPipelineState(graphicsPipelineState.Get());
			commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			// まず DescriptorHeap をバインドする（CBV/SRV/UAV 用）
			ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap.Get() };
			commandList->SetDescriptorHeaps(1, descriptorHeaps);

			// ==================================
			// CBVを設定する
			// ==================================
			// マテリアルCBufferの場所を設定
			commandList->SetGraphicsRootConstantBufferView(
				0, // ルートパラメータのインデックス
				materialResource.Get()->GetGPUVirtualAddress() // CBV用リソースのアドレス
			);

			// WVP用のCBufferの場所を設定
			commandList->SetGraphicsRootConstantBufferView(1, wvpResource.Get()->GetGPUVirtualAddress());

			// ライト用CBufferの場所を設定
			commandList->SetGraphicsRootConstantBufferView(
				3,
				directionalLightResource.Get()->GetGPUVirtualAddress()
			);

			// SRVのDescriptorTableの先頭を設定。2はrootParameters[2]で設定しているので2になる
			commandList->SetGraphicsRootDescriptorTable(2, useMonsterBall ? textureSrvHandleGPU2 : textureSrvHandleGPU);

			// 描画コマンド (DrawCall/ドローコール)3頂点で1つの図形を描画
			//commandList->DrawInstanced(6, 1, 0, 0); // 頂点3つで1つの図形を描画

			// ==================================
			// Sphere描画
			// =================================
			// 頂点バッファをセット
			commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSphere);
			// WVP用のCBufferの場所を設定 (Sphere用の行列リソースを指定)
			commandList->SetGraphicsRootConstantBufferView(1, wvpResourceSphere.Get()->GetGPUVirtualAddress());
			// 描画コマンド (頂点数分を描画)
			commandList->DrawInstanced(kSphereVertexCount, 1, 0, 0);

			// ==================================
			// Sprite描画
			// =================================
			commandList->IASetIndexBuffer(&indexBufferViewSprite);

			commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU); // SpriteはUvCheckerを使う

			// ライト用CBufferの場所を再設定（同じ値でOK）
			commandList->SetGraphicsRootConstantBufferView(
				3,
				directionalLightResource.Get()->GetGPUVirtualAddress()
			);

			commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSprite);
			// WVP用のCBufferの場所を設定
			commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceSprite.Get()->GetGPUVirtualAddress());

			commandList->SetGraphicsRootConstantBufferView(
				0, // ルートパラメータのインデックス
				materialResourceSprite.Get()->GetGPUVirtualAddress() // CBV用リソースのアドレス
			);

			// 描画コマンド
			commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

			// ==================================
			// ImGuiの描画
			// ==================================
		/*	ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap };
			commandList->SetDescriptorHeaps(1, descriptorHeaps);*/
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());

			// 画面に描く処理は全て終わり、画面に映すので状態を遷移
			// 今回はRenderTargetからPresentにする
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
			// TransitionBarrierを張る
			commandList->ResourceBarrier(1, &barrier);

			// コマンドリストの記録を終了
			hr = commandList->Close();
			assert(SUCCEEDED(hr));

			// ==================================
			// GUIにキックする
			// ==================================
			// GUIにコマンドリストの実行を行わせる
			ID3D12CommandList* commandlLists[] = { commandList.Get() };
			commandQueue->ExecuteCommandLists(1, commandlLists);

			// GPUとOSに画面の交換を行うように通知する
			swapChain->Present(1, 0);

			// GPUにコマンドの実行完了を通知するようにSignalを送る
			fenceValue++; // 値を更新
			hr = commandQueue->Signal(fence.Get(), fenceValue);

			if (fence->GetCompletedValue() < fenceValue) {
				fence->SetEventOnCompletion(fenceValue, fenceEvent);

				// イベントが来るまで待機
				WaitForSingleObject(fenceEvent, INFINITE);
			}


		}
	}

	// ==================================
	// 終了前に GPU の完了待ち（安全な破棄のため）
	// ==================================
	if (commandQueue && fence && fenceEvent) {
		fenceValue++;
		hr = commandQueue->Signal(fence.Get(), fenceValue);
		if (SUCCEEDED(hr) && fence->GetCompletedValue() < fenceValue) {
			hr = fence->SetEventOnCompletion(fenceValue, fenceEvent);
			if (SUCCEEDED(hr)) {
				WaitForSingleObject(fenceEvent, INFINITE);
			}
		}
	}

	// ==================================
	// ImGui の解放（描画系より先）
	// ==================================
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	return 0;
}
