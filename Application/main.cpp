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

using namespace StringUtility;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
	WinApp* winApp = new WinApp();
	winApp->Initialize();

	DirectXCom* dx = new DirectXCom();
	dx->Initialize(winApp);

	SrvManager* srv = new SrvManager();
	srv->Initialize(dx);

	TextureManager::GetInstance()->Initialize(dx, srv);

	Object3dCom* objCom = new Object3dCom();
	objCom->Initialize(dx);

	ModelManager::GetInstance()->Initialize(dx);

	// 入力
	KeyInput* keyInput = new KeyInput();
	keyInput->Initialize(winApp); // グローバルインスタンス設定

	// Camera (生成→位置設定→Update→デフォルト登録を一括)
	Camera* camera = objCom->CreateDefaultCamera({ 0,0,-5 });
	// 画面サイズでアスペクトを設定し、FOVを少し広げる
	camera->SetAspectRatio(static_cast<float>(dx->GetClientWidth()) / static_cast<float>(dx->GetClientHeight()));
	camera->SetFovY(0.8f); // 広めにして横方向が見えるように

	// Object (モデル未読込なら読み込み→設定し Transform と Camera 即時反映)
	Object3d* obj =
		Object3d::Create(objCom, "apple.obj", { {1,1,1},{0,0,0},{0,0,0} }, camera);

	// Player 用 Object3d と Player インスタンス生成（中心付近に配置し視野内に入れる）
	Object3d* playerModel =
		Object3d::Create(objCom, "apple.obj", { {1,1,1},{0,0,0},{0.3f,0,0} }, camera);
	Player* player = new Player();
	player->Initialize(playerModel, camera, { 0.3f, 0.0f, 0.0f }, objCom);

	while (!winApp->ProcessMessage())
	{
		// Input Update (毎フレーム最初に呼ぶ)
		keyInput->Update();

		// Update
		camera->Update();
		obj->Update();
		player->Update();

		// Draw
		dx->PreDraw();
		srv->PreDraw();
		objCom->ApplyCommonRenderState(); // カリング疑い時は ApplyCommonRenderState(false);
		//obj->Draw();
		player->Draw();
		dx->PostDraw();
	}

	// finalize
	delete player;
	delete playerModel;
	delete obj;
	delete camera;
	delete keyInput; // 入力破棄
	ModelManager::GetInstance()->Finalize();
	TextureManager::GetInstance()->Finalize();
	delete objCom;
	delete srv;
	delete dx;
	winApp->Finalize();
	delete winApp;
	return 0;
}