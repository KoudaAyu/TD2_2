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

using namespace StringUtility;

GameScene* gameScene = nullptr;
TitleScene* titleScene = nullptr;

enum class Scene
{
	kUnknown = 0,

	kTitle,
	kGame,
};


Scene scene = Scene::kUnknown;

Camera* camera = nullptr;
Object3dCom* objCom = nullptr;
SpriteCom* spriteCom = nullptr;

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

	// Use global objCom (avoid local shadowing)
	objCom = new Object3dCom();
	objCom->Initialize(dx);

	// SpriteCom を作成して Initialize (global)
	spriteCom = new SpriteCom();
	spriteCom->Initialize(dx);

	ModelManager::GetInstance()->Initialize(dx);

	// 入力
	KeyInput* keyInput = new KeyInput();
	keyInput->Initialize(winApp); // グローバルインスタンス設定

	// Camera (生成→位置設定→Update→デフォルト登録を一括) Use global camera (avoid shadowing)
	camera = objCom->CreateDefaultCamera({ 0,0,-5 });
	// 画面サイズでアスペクトを設定し、FOVを少し広げる
	camera->SetAspectRatio(static_cast<float>(dx->GetClientWidth()) / static_cast<float>(dx->GetClientHeight()));
	camera->SetFovY(0.8f); // 広めにして横方向が見えるように



	//GameScene* gameScene = new GameScene();
	//gameScene->Initialize(camera, objCom);

	scene = Scene::kTitle;
	titleScene = new TitleScene();
	titleScene->Initialize(spriteCom);

	while (!winApp->ProcessMessage())
	{
		// Input Update (毎フレーム最初に呼ぶ)
		keyInput->Update();

		// Update
		camera->Update();
		/*gameScene->Update();*/
		ChangePhase();
		UpdateScene();

		// Draw
		dx->PreDraw();
		srv->PreDraw();
		objCom->ApplyCommonRenderState(); // カリング疑い時は ApplyCommonRenderState(false);
		/*gameScene->Draw();*/
		DrawScene();
		dx->PostDraw();
	}

	// finalize

	delete gameScene;
	delete titleScene;
	delete camera;
	delete keyInput; // 入力破棄
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
			scene = Scene::kGame;
			gameScene = new GameScene();
			gameScene->Initialize(camera, objCom);
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
	}
}

void UpdateScene()
{
	switch (scene)
	{
	case Scene::kTitle:
		titleScene->Update();
		break;
	case Scene::kGame:
		gameScene->Update();
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
	case Scene::kGame:
		gameScene->Draw();
		break;
	}
}
