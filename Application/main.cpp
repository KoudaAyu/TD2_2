#include"DirectXCom.h"
#include"ImGuiManager.h"
#include"ModelManager.h"
#include"TextureManager.h"
#include"SoundManager.h"
#include"SpriteCom.h"
#include"SrvManager.h"
#include"Object3dCom.h"
#include"WinApp.h"
#include"Sprite.h"


#include"Player.h"
#include"KeyInput.h"

#include"TitleScene.h"
#include"GameScene.h"
#include "SelectScene.h"

#include "Baziru3_Engine/Particle/ParticleManager.h"
#include "Baziru3_Engine/Audio/SoundManager.h"
#include "ClearScene.h"


using namespace StringUtility;

GameScene* gameScene = nullptr;
TitleScene* titleScene = nullptr;
SelectScene* selectScene = nullptr;
ClearScene* clearScene = nullptr;

Sprite* gTransitionOverlay = nullptr; // fullscreen black overlay used during deferred transitions

enum class Scene
{
	kUnknown = 0,

	kTitle,
	kSelect,
	kGame,
	kClear,
};


Scene scene = Scene::kUnknown;

Camera* camera = nullptr;
Object3dCom* objCom = nullptr;
SpriteCom* spriteCom = nullptr;
#ifdef USE_IMGUI
ImGuiManager* imguiManager = nullptr;
#endif

// Deferred transition helper
static bool gPendingTransition = false;
static Scene gPendingTarget = Scene::kUnknown;

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

	// Preload commonly used UI textures (avoid a texture load hitch during scene transitions)
	TextureManager::GetInstance()->LoadTexture("Resources/white.png");
	
	objCom = new Object3dCom();
	objCom->Initialize(dx);

	// SpriteCom を作成して Initialize 
	spriteCom = new SpriteCom();
	spriteCom->Initialize(dx);

	ModelManager::GetInstance()->Initialize(dx);

	ParticleManager::GetInstance()->Initialize(dx, srv, objCom);

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
	// In debug mode start at the Title scene for faster debugging
	scene = Scene::kTitle;
	titleScene = new TitleScene();
	titleScene->Initialize(camera, objCom,spriteCom);
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

		// Always use central scene management so transitions (including ClearScene) behave the same in debug and release
		UpdateScene();
		ChangePhase();

#ifdef USE_IMGUI
		// ImGuiフレーム終了（内部コマンド生成）
		imguiManager->End();
#endif

		// Draw
		dx->PreDraw();
		srv->PreDraw();
		objCom->ApplyCommonRenderState(); // カリング疑い時は ApplyCommonRenderState(false);

		DrawScene();

		// If a pending deferred transition is requested, present the current frame now (so fade/black is visible),
		// then perform the heavy scene construction while the player sees the black frame.
		bool presentedEarly = false;
		if (gPendingTransition)
		{
			// present now
			// Ensure the overlay exists and will be drawn: if not created in ChangePhase, create it here as fallback
			if (!gTransitionOverlay && spriteCom)
			{
				int sw = spriteCom->GetDirectXCom()->GetClientWidth();
				int sh = spriteCom->GetDirectXCom()->GetClientHeight();
				// Use a guaranteed-existing UI texture for black overlay to avoid Texture load assert
				gTransitionOverlay = spriteCom->CreateSprite("Resources/UI/SPACEUI.png", { 0.0f, 0.0f }, { static_cast<float>(sw), static_cast<float>(sh) }, 0.0f, { 0.0f, 0.0f }, false, false);
				if (gTransitionOverlay) { gTransitionOverlay->SetColor({0.0f,0.0f,0.0f,1.0f}); gTransitionOverlay->Update(); }
			}

			// present current backbuffer (DrawScene already drew overlay if present)
			dx->PostDraw();
			presentedEarly = true;

			// perform the pending transition synchronously (heavy init will happen now while black frame shown)
			if (gPendingTarget == Scene::kGame)
			{
				// Delete selectScene and create gameScene while black is on screen
				if (selectScene)
				{
					delete selectScene;
					selectScene = nullptr;
				}
				// create and initialize GameScene (may be heavy)
				gameScene = new GameScene();
				gameScene->Initialize(camera, objCom, spriteCom);
				scene = Scene::kGame;
			}
			else if (gPendingTarget == Scene::kSelect)
			{
				// Delete titleScene and create selectScene while black is on screen
				if (titleScene)
				{
					delete titleScene;
					titleScene = nullptr;
				}
				// create and initialize SelectScene (may be heavy)
				selectScene = new SelectScene();
				selectScene->Initialize(spriteCom, objCom, camera);
				scene = Scene::kSelect;
			}

			// clear pending
			gPendingTransition = false;
			gPendingTarget = Scene::kUnknown;

			// remove overlay now that heavy init is done (it will be recreated next time as needed)
			if (gTransitionOverlay) { delete gTransitionOverlay; gTransitionOverlay = nullptr; }
		}

		// Normal PostDraw if we haven't already presented early
		if (!presentedEarly) dx->PostDraw();
	}

	// finalize

	delete gameScene;
	delete titleScene;
	delete selectScene;
	delete clearScene;
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
			// Request a deferred transition to Select. The heavy init will be run while we present
			// a fullscreen white overlay for one frame so the TitleScene doesn't flash.
			gPendingTransition = true;
			gPendingTarget = Scene::kSelect;

			// create a fullscreen white overlay so next frame draws white while we initialize
			if (!gTransitionOverlay && spriteCom) {
				int sw = spriteCom->GetDirectXCom()->GetClientWidth();
				int sh = spriteCom->GetDirectXCom()->GetClientHeight();
				gTransitionOverlay = spriteCom->CreateSprite("Resources/white.png", { 0.0f, 0.0f }, { static_cast<float>(sw), static_cast<float>(sh) }, 0.0f, { 0.0f, 0.0f }, false, false);
				if (gTransitionOverlay) { gTransitionOverlay->SetColor({1.0f,1.0f,1.0f,1.0f}); gTransitionOverlay->Update(); }
			}

			// actual deletion/creation of scenes will happen in the pending-transition handler in the main loop
		}
		break;

	case Scene::kSelect:
		if (selectScene->IsFinish())
		{
			// Always go to Game with deferred transition
			gPendingTransition = true;
			gPendingTarget = Scene::kGame;


			// create a fullscreen black overlay so next frame draws black
			if (!gTransitionOverlay && spriteCom) {
				int sw = spriteCom->GetDirectXCom()->GetClientWidth();
				int sh = spriteCom->GetDirectXCom()->GetClientHeight();
				// Use a guaranteed-existing UI texture for black overlay to avoid Texture load assert
				gTransitionOverlay = spriteCom->CreateSprite("white.png", { 0.0f, 0.0f }, { static_cast<float>(sw), static_cast<float>(sh) }, 0.0f, { 0.0f, 0.0f }, false, false);
				if (gTransitionOverlay) { gTransitionOverlay->SetColor({0.0f,0.0f,0.0f,1.0f}); gTransitionOverlay->Update(); }
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
			titleScene->Initialize(camera, objCom, spriteCom);
		}

		// detect boss clear and switch to clear scene
		if (gameScene && gameScene->IsCleared())
		{
			// transition to clear scene
			if (!clearScene)
			{
				clearScene = new ClearScene();
			}
			clearScene->Initialize(camera, objCom, spriteCom);
			// tear down game scene
			delete gameScene;
			gameScene = nullptr;
			scene = Scene::kClear;
		}

		break;

	case Scene::kClear:
		if (clearScene && clearScene->IsFinish())
		{
			delete clearScene;
			clearScene = nullptr;
			scene = Scene::kTitle;
			titleScene = new TitleScene();
			titleScene->Initialize(camera, objCom, spriteCom);
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
	case Scene::kClear:
		if (clearScene) clearScene->Update();
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
	case Scene::kClear:
		if (clearScene) clearScene->Draw();
		break;
	}

	// draw overlay if pending
	if (gPendingTransition && gTransitionOverlay) {
		gTransitionOverlay->Draw();
	}
}
