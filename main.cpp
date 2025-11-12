#include "WinApp.h"
#include <Windows.h>

// 自作h
#include "Camera.h"
#include "D3DResourceLeakChecker.h"
#include "DebugCamera.h"
#include "DirectXCom.h"
#include "ImGuiManager.h"
#include "KeyInput.h"
#include "Matrix4x4.h"
#include "Model.h"
#include "ModelManager.h"
#include "Sprite.h"
#include "SpriteCom.h"
#include "Transform.h"
#include "Vector.h"

#include "GameScene.h"
#include"SelectScene.h"
#include "TitleScene.h"
#include"Tutorial.h"

#include <chrono> //時間を扱うライブラリ
#include <cstdint>
#include <filesystem> //ファイルやディレクトリに関する操作を行うライブラリ
#include <format>     //文字列のフォーマットを行うライブラリ
#include <fstream>    //ファイルにかいたり読んだりするライブラリ
#include <string>     //文字列を扱うライブラリ
#include <strsafe.h>

#include <cassert>
#include <d3d12.h>
#include <dxgi1_6.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

// Comptr
#include <wrl.h>

// Debug用
#include <dbghelp.h>
#pragma comment(lib, "Dbghelp.lib")

// ファイル関係 / サウンド関係
#include <sstream>

// ReportLiveObjects
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")

// Textureの転送
#include "externals/DirectXTex/d3dx12.h"
#include <vector>

#include "externals/DirectXTex/DirectXTex.h"
#include <DirectXMath.h>
#include <cmath>

// Imgui使用のため
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"

// Sound
#include "Object3d.h"
#include "Object3dCom.h"
#include "SoundManager.h"
#include "SrvManager.h"
#include "TextureManager.h"
using namespace StringUtility;

enum class Scene
{
	kUnknown = 0,
	kTitle,
	kSelect,
	kTutorial,
	kGame,
};

Scene scene = Scene::kUnknown;

TitleScene* titleScene = nullptr;
Tutorial* tutorialScene = nullptr;
SelectScene* selectScene = nullptr;
GameScene* gameScene = nullptr;

KeyInput keyInput;
SoundManager* soundManager = nullptr;

WinApp* winApp = nullptr;
Object3dCom* object3dCom = nullptr;
SpriteCom* spriteCom = nullptr;
DirectXCom* directXCom = nullptr;
SrvManager* srvManager = nullptr;


void ChangeScene();

void UpdateScene();

void DrawScene();

static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception)
{
	// 時刻を取得して、時刻を名前に入れたファイルを作成。Dumpsディレクトリ以下に出力
	SYSTEMTIME time;
	GetSystemTime(&time);
	wchar_t filePath[MAX_PATH] = { 0 };
	CreateDirectory(L"./Dumps", nullptr);
	StringCchPrintfW(filePath, MAX_PATH, L"./Dumps/%04d-%02d%02d-%02d%02d.dmp",
		time.wYear, time.wMonth, time.wDay, time.wHour,
		time.wMinute);
	HANDLE dumpFileHandle =
		CreateFile(filePath, GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

	// processId(このexeのId)とクラッシュ(例外)の発生したthreadIdを取得
	DWORD processId = GetCurrentProcessId();
	DWORD threadId = GetCurrentThreadId();

	// 設定情報を入力
	MINIDUMP_EXCEPTION_INFORMATION minidumpInformation{};
	minidumpInformation.ThreadId = threadId; // クラッシュしたスレッドのID
	minidumpInformation.ExceptionPointers = exception; // 例外情報
	minidumpInformation.ClientPointers = TRUE; // クライアントポインタを使用する

	// Dumpの出力を行う。MiniDumpNormalは最小限の情報を出力する
	MiniDumpWriteDump(GetCurrentProcess(), processId, dumpFileHandle,
		MiniDumpNormal, &minidumpInformation, nullptr, nullptr);

	// ほかに関連付けられているSEH例外ハンドラがあったら実行。通常時はプロセスを終了
	return EXCEPTION_EXECUTE_HANDLER;
}

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
	D3DResourceLeakChecker leakChecker; // リソースリークチェック用のオブジェクト

	CoInitializeEx(0, COINIT_MULTITHREADED);

	// 誰も補足しなかった場合に(Unhandled)、補足する関数を登録
	SetUnhandledExceptionFilter(ExportDump);

	// ログファイル関係
	// ログのディレクトリを用意
	std::filesystem::create_directories("logs");

	// 現在時刻を取得(UTC時刻)
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

	// ログファイルの名前にコンマ何秒はいらないため、削って秒にする
	std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>
		nowSecound = std::chrono::time_point_cast<std::chrono::seconds>(now);

	// 日本時間(PCの設定時間に変換)
	std::chrono::zoned_time localTime{ std::chrono::current_zone(), nowSecound };

	// formatを使って年月日_時分秒の形式にする
	std::string datString = std::format("{:%Y%m%d_%H%M%S}", localTime);

	// 時刻を使ってファイル名を決定
	std::string logFilePath = std::string("logs/") + datString + ".log";

	// ファイルを作って書き込み準備
	std::ofstream logStream(logFilePath);

	// ポインタ

	// WindowsAPIの初期化
	winApp = new WinApp();
	winApp->Initialize();

	assert(winApp->GetHInstance() != nullptr);
	assert(winApp->GetHwnd() != nullptr);

	keyInput.Initialize(winApp);

#pragma region 基盤システムの初期化



	directXCom = new DirectXCom();
	directXCom->Initialize(winApp);
	HRESULT hr = directXCom->GetHr();


	//SRVマネージャの初期化
	srvManager = new SrvManager();
	srvManager->Initialize(directXCom);


	// ImGui
#ifdef USE_IMGUI
	ImGuiManager* imguiManager = nullptr;
	imguiManager = new ImGuiManager();
	imguiManager->Initialize(winApp, directXCom, srvManager);
#endif

	// 初期化
	TextureManager::GetInstance()->Initialize(directXCom, srvManager);

	// スプライト共通部の初期化
	spriteCom = new SpriteCom;
	spriteCom->Initialize(directXCom);

	// 3Dオブジェクト共通部の初期化
	object3dCom = new Object3dCom();
	object3dCom->Initialize(directXCom);

	// 3Dモデルマネージャーの初期化
	ModelManager::GetInstance()->Initialize(directXCom);

	// サウンドマネージャの初期化（シーンより先）
	soundManager = new SoundManager();
	if (!soundManager->Initialize())
	{
		assert(0 && "XAudio2初期化失敗");
		return -1;
	}

#pragma endregion

#pragma region シーンの初期化

	scene = Scene::kTitle;

	titleScene = new TitleScene();
	titleScene->SetWinApp(winApp);
	titleScene->Initialize(object3dCom, spriteCom);

#pragma endregion

	// ウィンドウのxボタンが押されるまでループ
	while (true)
	{

		// Windowsのメッセージ処理
		if (winApp->ProcessMessage())
		{
			// ゲームループを抜ける
			break;
		}

		keyInput.Update();

#ifdef _DEBUG
		imguiManager->Begin();

		// 開発用UIの処理、実際に開発用のUIを出す場合はここをゲーム固有の処理に置き換え
		ImGui::ShowDemoWindow();

		ImGui::Begin("Windows"); // ImGui書くならここから
#endif

		/*ImGui::Checkbox("DrawSprite", &drawSprite);

		ImGui::DragFloat2("UVTranslate", &positionUv.x, 1.0f);*/
		/*ImGui::DragFloat2("BoardTranslate", &positionBoard.x, 1.0f);*/

		/*ImGui::DragFloat3("camera", &positionCamera.x, 0.1f);*/
#ifdef _DEBUG
		ImGui::End(); // ここまで

		// ImGui内部コマンドを生成する
		ImGui::Render();
#endif

		ChangeScene();
		UpdateScene();

#ifdef _DEBUG
		imguiManager->End();
#endif

		// 描画前処理
		directXCom->PreDraw();

		srvManager->PreDraw();

		// 3Dオブジェクトの描画準備
		object3dCom->ApplyCommonRenderState();

		spriteCom->ApplyCommonRenderState();

		DrawScene();

#ifdef _DEBUG
		imguiManager->Draw();
#endif

		directXCom->PostDraw();
	}

	//// ImGui終了処理
#ifdef _DEBUG
	imguiManager->Finalize();
#endif

	Logger::Log("Application terminating.");

	std::wstring wstringValue = L"Hello, DirectX!";
	Logger::Log(ConvertString(std::format(L"WSTRING{}\n", wstringValue)));

	// 出力ウィンドウへの文字出力
	OutputDebugStringA("Hello, DirextX!\n");

	// 先にシーンを解放（サブシステムに依存しているため）
	delete titleScene; titleScene = nullptr;
	delete selectScene; selectScene = nullptr;
	delete tutorialScene; tutorialScene = nullptr;
	delete gameScene; gameScene = nullptr;

	// サブシステムの終了
	soundManager->Finalize();
	delete soundManager; soundManager = nullptr;

	delete spriteCom;

	delete object3dCom;

	TextureManager::GetInstance()->Finalize();

	delete srvManager;

	// WindowsAPIの終了処理
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

void ChangeScene()
{
	switch (scene)
	{
	case Scene::kTitle:
		if (titleScene && titleScene->IsFinished())
		{
			// シーン変更
			scene = Scene::kSelect;
			// 旧シーンの解放
			delete titleScene;
			titleScene = nullptr;

			// 新シーンの生成
			selectScene = new SelectScene();
			selectScene->Initialize(object3dCom, spriteCom);

		}

		break;

	case Scene::kSelect:
		if (selectScene && selectScene->IsSelectGameScene())
		{
			// シーン変更
			scene = Scene::kGame;
			// 旧シーンの解放
			delete selectScene;
			selectScene = nullptr;
			// 新シーンの生成
			gameScene = new GameScene();
			gameScene->SetWinApp(winApp);
			gameScene->Initialize(object3dCom, spriteCom);
		}
		else if (selectScene &&
			selectScene->IsSelectTutorialScene())
		{
			// シーン変更
			scene = Scene::kTutorial;
			// 旧シーンの解放
			delete selectScene;
			selectScene = nullptr;
			// 新シーンの生成
			tutorialScene = new Tutorial();
			tutorialScene->SetWinApp(winApp);
			tutorialScene->Initialize(object3dCom, spriteCom);
		}
		else if (selectScene &&
			selectScene->IsSelectTitleScene())
		{
			// シーン変更
			scene = Scene::kTitle;
			delete selectScene;
			selectScene = nullptr;
			titleScene = new TitleScene();
			titleScene->SetWinApp(winApp);
			titleScene->Initialize(object3dCom, spriteCom);
		}
		break;
	case Scene::kTutorial:
          if (tutorialScene && tutorialScene->IsFinished()) {
              
                // シーン変更
                scene = Scene::kSelect;
                // 旧シーンの解放
                delete tutorialScene;
                tutorialScene = nullptr;

                // 新シーンの生成
                selectScene = new SelectScene();
                selectScene->Initialize(object3dCom, spriteCom);
          }
          
		break;
	case Scene::kGame:

		if (gameScene && gameScene->IsFinished())
		{
			scene = Scene::kTitle;
			delete gameScene;
			gameScene = nullptr;
			titleScene = new TitleScene();
			titleScene->SetWinApp(winApp);
			titleScene->Initialize(object3dCom,
				spriteCom);
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

	case Scene::kTutorial:
		tutorialScene->Update();
		break;

	case Scene::kGame:
#ifdef _DEBUG
		gameScene->GetDebugCamera()->Update();
#endif
		gameScene->GetCamera()->Update();
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

	case Scene::kTutorial:
		tutorialScene->Draw();
		break;

	case Scene::kSelect:
		selectScene->Draw();
		break;
	case Scene::kGame:
		gameScene->Draw();
		break;
	}
}
