#pragma once

// ===============================
// Windows API群
// ===============================
#include <d3d12.h>
#pragma comment(lib, "d3d12.lib")

// DXGI
#include <dxgi1_6.h>
#pragma comment(lib, "dxgi.lib")

// DXGI Debug
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")

// DXC
#include <dxcapi.h>
#pragma comment(lib, "dxcompiler.lib")

// =============================
// Asset群
// =============================

// -------- Lorder ------------
#include "LoadObjFile.h"

// =============================
// Math
// =============================
#include "Math.h"

// ==============================
// Renderer群
// ==============================

/*==== ImGui ====*/
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ==============================
// Renderer
// ==============================
#include "Renderer.h"

// ==============================
// RHI群
// ==============================
#include "D3D12Includes.h"

// Resource
#include "ResourcesIncludes.h"

// =============================
// D3D12
// =============================
#include "GraphicsCore.h"

// =============================
// Core
// =============================
#include "Core.h"

// =============================
// 外部ライブラリ
// =============================
#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"

class TomoEngine
{
};

/*
==============================
階層構造の案
==============================

Engine/
  Core/
	Base/                # assert, type aliases, build config
	Platform/
	  Windows/           # Win32 Window, message pump, timer
	Utility/
	  Logger/
	  String/            # ConvertString 等
	  FileSystem/
	  Serialization/     # json等（導入するなら）
  Math/
	2D/                  # Vector2, Rect 等
	3D/                  # Vector3, Vector4, Matrix4x4, Transform, Affine
  RHI/                   # Rendering Hardware Interface（描画API抽象）
	Common/              # 抽象IF (IRenderDevice 等)  ※必要になった時に
	D3D12/                # ここがDX12依存の隔離場所（重要）
	  Device/
	  Command/
	  SwapChain/
	  Descriptor/
	  Resources/
		ResourceObject/  # ← ID3D12Resource RAII, Com RAII
		Buffer/
		Texture/
	  Shader/
		Dxc/             # CompileShader, include handlerなど
	  Debug/
		Dred/            # GPUクラッシュ解析等（将来）
  Renderer/              # 描画パイプライン（Forward/Deferred等）
	Common/
	Sprite/
	Mesh/
	ImGui/               # ImGui描画のエンジン側統合（バックエンドはRHI依存）
  Scene/                 # Scene/Entity/Component（ECSならここ）
  Asset/                 # アセット管理（ロード・キャッシュ・ハンドル）
  Input/                 # キーボード/マウス/Pad
  Audio/                 # XAudio2等（必要なら）
  UI/                    # 自前UIを作るなら
  Debug/                 # エンジンのデバッグ表示/統計

*/