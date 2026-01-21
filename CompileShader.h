// Header
#pragma once
#include <string>
#include <dxcapi.h>
#include <wrl/client.h>


// 名前空間に入れておくと行儀が良いです
Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(
	const std::wstring& filePath,
	const wchar_t* profile,
	IDxcUtils* dxcUtils,
	IDxcCompiler3* dxcCompiler,
	IDxcIncludeHandler* includeHandler
);