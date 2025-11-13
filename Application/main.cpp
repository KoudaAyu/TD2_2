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

    // Camera (生成→位置設定→Update→デフォルト登録を一括)
    Camera* camera = objCom->CreateDefaultCamera({0,0,-5});

    // Object (モデル未読込なら読み込み→設定し Transform と Camera 即時反映)
    Object3d* obj = 
        Object3d::Create(objCom, "apple.obj", { {1,1,1},{0,0,0},{0,0,0} }, camera);

    while (!winApp->ProcessMessage())
    {
        // Update
        camera->Update();
        obj->Update();

        // Draw
        dx->PreDraw();
        srv->PreDraw();
        objCom->ApplyCommonRenderState(); // カリング疑い時は ApplyCommonRenderState(false);
        obj->Draw();
        dx->PostDraw();
    }

    // finalize
    delete obj;
    delete camera;
    ModelManager::GetInstance()->Finalize();
    TextureManager::GetInstance()->Finalize();
    delete objCom;
    delete srv;
    delete dx;
    winApp->Finalize();
    delete winApp;
    return 0;
}