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

    // Camera
    Camera* cam = new Camera();
    cam->SetTranslate({ 0, 0, -5 });
    cam->Update();
    objCom->SetDefaultCamera(cam);

    // Load model
    auto* mm = ModelManager::GetInstance();
    mm->LoadModel("apple.obj");
    Model* mdl = mm->FindModel("apple.obj");
    assert(mdl);
    assert(mdl->GetVertexCount() > 0);

    // Object
    Object3d* obj = new Object3d();
    obj->Initialize(objCom);
    obj->SetModel(mdl);
    obj->SetTranslate({ 0,0,0 });
    obj->Update();

    while (!winApp->ProcessMessage())
    {
        // Update
        cam->Update();
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
    delete cam;
    ModelManager::GetInstance()->Finalize();
    TextureManager::GetInstance()->Finalize();
    delete objCom;
    delete srv;
    delete dx;
    winApp->Finalize();
    delete winApp;
    return 0;
}