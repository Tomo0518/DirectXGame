// ==============================
// 標準ライブラリ
// ==============================
#include <string>   // std::wstring
#include <format>   // std::format
#include <cassert>  // assert

// ==============================
// Windows & COM
// ==============================
#include <Windows.h>      // HRESULT, SUCCEEDED, IID_PPV_ARGS など
#include <wrl/client.h>   // Microsoft::WRL::ComPtr

// ==============================
// DirectX Shader Compiler (DXC)
// ==============================
#include <dxcapi.h>       // IDxcBlob, IDxcCompiler3 など

// ライブラリのリンク指定（プロジェクト設定で行っていない場合）
#pragma comment(lib, "dxcompiler.lib")

// ==============================
// 自作ユーティリティ
// ==============================
// ログ出力関数 (Log) と 文字列変換 (ConvertString) を使う
#include "Logger.h"       
#include "ConvertString.h"


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