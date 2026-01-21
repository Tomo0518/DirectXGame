#include <Windows.h>
#include "TomoEngine.h"
#include "GraphicsCore.h"
#include "Game.h"
#include "Window.h"
#include "D3DResourceLeakChecker.h"
#include "InputManager.h"

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

    // リソースリークチェッカー
    D3DResourceLeakChecker leakChecker;

    {
        // ===============================
        // 1. システム初期化
        // ===============================
        Window::GetInstance()->CreateGameWindow(L"CG2");

        // GraphicsCore初期化 (ウィンドウサイズに合わせて)
        GraphicsCore::GetInstance()->Initialize(Window::GetInstance()->GetHwnd(), 1280, 720);

		// 入力システム初期化
        InputManager::GetInstance()->Initialize(Window::GetInstance()->GetHwnd());

        // ===============================
        // 2. ゲームクラスの生成・初期化
        // ===============================
        std::unique_ptr<Game> game = std::make_unique<Game>();
        game->Initialize();

        // ===============================
        // 3. メインループ
        // ===============================
        MSG msg{};
        while (msg.message != WM_QUIT) {
            if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            else {
                // 更新
                game->Update();

                // 描画 (内部でGraphicsContextを使用)
                game->Render();

                GraphicsCore::GetInstance()->Present();
            }
        }

        // ===============================
        // 4. 終了処理
        // ===============================
        game->Shutdown();

        // エンジン終了
        GraphicsCore::GetInstance()->Shutdown();
        Window::GetInstance()->Shutdown();
    }

    return 0;
}