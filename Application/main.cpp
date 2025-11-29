#include"DirectXCom.h"
#include"ImGuiManager.h"
#include"ModelManager.h"
#include"TextureManager.h"
#include"SoundManager.h"
#include"SpriteCom.h"
#include"SrvManager.h"
#include"Object3dCom.h"
#include"WinApp.h"


#include"Player.h"
#include"KeyInput.h"

#include"TitleScene.h"
#include"GameScene.h"
#include "SelectScene.h"
#include "TutorialScene.h"

using namespace StringUtility;

GameScene* gameScene = nullptr;
TitleScene* titleScene = nullptr;
SelectScene* selectScene = nullptr;
TutorialScene* tutorialScene = nullptr;

enum class Scene
{
	kUnknown = 0,

	kTitle,
	kSelect,
	kGame,
	kTutorial,
};


Scene scene = Scene::kUnknown;

Camera* camera = nullptr;
Object3dCom* objCom = nullptr;
SpriteCom* spriteCom = nullptr;
#ifdef USE_IMGUI
ImGuiManager* imguiManager = nullptr;
#endif

void ChangePhase();

void UpdateScene();

void DrawScene();



int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
	WinApp* winApp = new WinApp();
	winApp->Initialize();

	DirectXCom* dx = new DirectXCom();
	dx->Initialize(winApp);

	SrvManager* srv = new SrvManager();
	srv->Initialize(dx);

	TextureManager::GetInstance()->Initialize(dx, srv);

	
	objCom = new Object3dCom();
	objCom->Initialize(dx);

	// SpriteCom を作成して Initialize 
	spriteCom = new SpriteCom();
	spriteCom->Initialize(dx);

	ModelManager::GetInstance()->Initialize(dx);

	// 入力
	KeyInput* keyInput = new KeyInput();
	keyInput->Initialize(winApp);

	
	camera = objCom->CreateDefaultCamera({ 0,0,-5 });
	// 画面サイズでアスペクトを設定し、FOVを少し広げる
	camera->SetAspectRatio(static_cast<float>(dx->GetClientWidth()) / static_cast<float>(dx->GetClientHeight()));
	camera->SetFovY(0.8f); // 広めにして横方向が見えるように

#ifdef USE_IMGUI
	// ImGui初期化
	imguiManager = new ImGuiManager();
	imguiManager->Initialize(winApp, dx, srv);
#endif

#ifdef _DEBUG
	scene = Scene::kGame;
	gameScene = new GameScene();
	gameScene->Initialize(camera,objCom,spriteCom);
	/*scene = Scene::kTitle;
	titleScene = new TitleScene();
	titleScene->Initialize(spriteCom);*/
#else
	scene = Scene::kTitle;
	titleScene = new TitleScene();
	titleScene->Initialize(spriteCom);
#endif



	while (!winApp->ProcessMessage())
	{
		// Input Update (毎フレーム最初に呼ぶ)
		keyInput->Update();

#ifdef USE_IMGUI
		// ImGuiフレーム開始（これより後に ImGui::Begin 等を呼べる)
		imguiManager->Begin();
#endif

		// Update
		camera->Update();
#ifdef _DEBUG
		gameScene->Update();
		/*ChangePhase();
		UpdateScene();*/
#else
		ChangePhase();
		UpdateScene();
#endif

#ifdef USE_IMGUI
		// ImGuiフレーム終了（内部コマンド生成）
		imguiManager->End();
#endif

		// Draw
		dx->PreDraw();
		srv->PreDraw();
		objCom->ApplyCommonRenderState(); // カリング疑い時は ApplyCommonRenderState(false);
#ifdef _DEBUG
		gameScene->Draw();
		//DrawScene();
#else
		DrawScene();
#endif
#ifdef USE_IMGUI
		// ImGui描画（3D描画の後に）
		imguiManager->Draw();
#endif
		dx->PostDraw();
	}

	// finalize

	delete gameScene;
	delete titleScene;
	delete selectScene;
	delete tutorialScene;
	delete camera;
	delete keyInput; // 入力破棄
#ifdef USE_IMGUI
	if (imguiManager) { imguiManager->Finalize(); delete imguiManager; }
#endif
	ModelManager::GetInstance()->Finalize();
	TextureManager::GetInstance()->Finalize();
	delete objCom;
	delete spriteCom;
	delete srv;
	delete dx;
	winApp->Finalize();
	delete winApp;
	return 0;
}

void ChangePhase()
{
	switch (scene)
	{
	case Scene::kTitle:
		if (titleScene->IsFinish())
		{
			delete titleScene;
			titleScene = nullptr;
			scene = Scene::kSelect;
			selectScene = new SelectScene();
			selectScene->Initialize(spriteCom, objCom, camera);
		}
		break;

	case Scene::kSelect:
		if (selectScene->IsFinish())
		{
		
			SelectScene::Choice choice = selectScene->GetChoice();
			delete selectScene;
			selectScene = nullptr;
			if (choice == SelectScene::Choice::kGame)
			{
				scene = Scene::kGame;
				gameScene = new GameScene();
				gameScene->Initialize(camera, objCom, spriteCom);
			}
			else
			{
				scene = Scene::kTutorial;
				tutorialScene = new TutorialScene();
				tutorialScene->Initialize(camera, objCom, spriteCom);
			}
		}
		break;

	case Scene::kGame:
		if (gameScene->IsFinish())
		{
			delete gameScene;
			gameScene = nullptr;
			scene = Scene::kTitle;
			titleScene = new TitleScene();
			titleScene->Initialize(spriteCom);
		}

		break;

	case Scene::kTutorial:
		if (tutorialScene->IsFinish())
		{
			delete tutorialScene;
			tutorialScene = nullptr;
			scene = Scene::kTitle;
			titleScene = new TitleScene();
			titleScene->Initialize(spriteCom);
		}

		break;
	}
}

void UpdateScene()
{
	switch (scene)
	{
	case Scene::kTitle:
		titleScene->Update();
		break;
	case Scene::kSelect:
		selectScene->Update();
		break;
	case Scene::kGame:
		gameScene->Update();
		break;
	case Scene::kTutorial:
		tutorialScene->Update();
		break;

	}
}

void DrawScene()
{
	switch (scene)
	{
	case Scene::kTitle:
		titleScene->Draw();
		break;
	case Scene::kSelect:
		selectScene->Draw();
		break;
	case Scene::kGame:
		gameScene->Draw();
		break;
	case Scene::kTutorial:
		tutorialScene->Draw();
		break;
	}
}
