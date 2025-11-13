#include"DirectXCom.h"
#include"ImGuiManager.h"
#include"ModelManager.h"
#include"TextureManager.h"
#include"SoundManager.h"
#include"SpriteCom.h"
#include"SrvManager.h"
#include"Object3dCom.h"
#include"WinApp.h"

using namespace StringUtility;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
	WinApp* winApp = new WinApp();
	winApp->Initialize();

	assert(winApp->GetHInstance() != nullptr);
	assert(winApp->GetHwnd() != nullptr);

	DirectXCom* directXCom = new DirectXCom();
	directXCom->Initialize(winApp);
	HRESULT hr = directXCom->GetHr();


	//SRVマネージャの初期化
	SrvManager* srvManager = new SrvManager();
	srvManager->Initialize(directXCom);

#ifdef USE_IMGUI
	ImGuiManager* imguiManager = nullptr;
	imguiManager = new ImGuiManager();
	imguiManager->Initialize(winApp, directXCom, srvManager);
#endif

	TextureManager::GetInstance()->Initialize(directXCom, srvManager);

	// スプライト共通部の初期化
	SpriteCom* spriteCom = new SpriteCom;
	spriteCom->Initialize(directXCom);

	// 3Dオブジェクト共通部の初期化
	Object3dCom* object3dCom = new Object3dCom();
	object3dCom->Initialize(directXCom);

	// 3Dモデルマネージャーの初期化
	ModelManager::GetInstance()->Initialize(directXCom);

	// サウンドマネージャの初期化（シーンより先）
	SoundManager* soundManager = new SoundManager();
	if (!soundManager->Initialize())
	{
		assert(0 && "XAudio2初期化失敗");
		return -1;
	}


	while (true)
	{
		if (winApp->ProcessMessage())
		{
			// ゲームループを抜ける
			break;
		}

#ifdef _DEBUG
		imguiManager->Begin();

		// 開発用UIの処理、実際に開発用のUIを出す場合はここをゲーム固有の処理に置き換え
		ImGui::ShowDemoWindow();

		ImGui::Begin("Windows"); // ImGui書くならここから
#endif
		//シーンの処理
#ifdef _DEBUG
		ImGui::End();
		ImGui::Render();
#endif

#ifdef _DEBUG
		imguiManager->End();
#endif

		//描画前提処理

		directXCom->PreDraw();

		srvManager->PreDraw();

		object3dCom->ApplyCommonRenderState();

		spriteCom->ApplyCommonRenderState();

		//描画処理

#ifdef _DEBUG
		imguiManager->Draw();
#endif
		//描画後処理
		directXCom->PostDraw();
	}

#ifdef _DEBUG
	imguiManager->Finalize();
#endif

	soundManager->Finalize();
	delete soundManager;

	delete spriteCom;

	delete object3dCom;

	TextureManager::GetInstance()->Finalize();

	delete srvManager;
	winApp->Finalize();

	// 3Dモデルマネージャーの終了
	ModelManager::GetInstance()->Finalize();

	delete directXCom;
#ifdef _DEBUG
	delete imguiManager;
#endif
	// Windowsの解放
	delete winApp;

	return 0;
}