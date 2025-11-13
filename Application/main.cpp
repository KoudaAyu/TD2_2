#include"WinApp.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
	WinApp* winApp = new WinApp();
	winApp->Initialize();

	assert(winApp->GetHInstance() != nullptr);
	assert(winApp->GetHwnd() != nullptr);

	while (true)
	{
		if (winApp->ProcessMessage())
		{
			// ゲームループを抜ける
			break;
		}

	}
}