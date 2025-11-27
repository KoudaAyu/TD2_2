#pragma once

#define DIRECTIONPUT_VECTOR         0x0800 // 定数名は変更しません
#include<cmath>
#include<dinput.h>
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
#include <Windows.h> // HINSTANCE, HWND, HRESULT のために必要です
#include <cassert>   // assert のために必要です
#include"WinApp.h"
#include <Xinput.h>
#pragma comment(lib, "Xinput.lib") // XInput をリンク

// ZeroMemory のために必要です

class KeyInput
{
public:
    // コンストラクタ: メンバー変数を初期化
    KeyInput();

    // デストラクタ: DirectInput リソースを解放（非常に重要！）
    ~KeyInput();

    // 初期化メソッド
    void Initialize(WinApp* winApp);

    // 更新メソッド
    void Update();

    // キー状態取得
    bool PushKey(int dik_code) const;
    // キー押し始め（トリガー）
    bool TriggerKey(int dik_code) const;

    // 左スティック情報を返す構造体 (正規化された -1..1)
    struct Stick { float x; float y; };

    // 指定コントローラの左スティックを取得（デフォルトは 0）
    Stick GetLeftStick(int controllerIndex = 0) const;

    // スティックがしきい値以上に動いているか
    bool IsLeftStickMoved(float threshold = 0.2f, int controllerIndex = 0) const;

    // パッドボタン状態
    bool IsPadButtonPressed(WORD button, int controllerIndex = 0) const;
    bool TriggerPadButton(WORD button, int controllerIndex = 0) const;

    // グローバルアクセス用インスタンス取得
    static KeyInput* GetInstance();

private:
    // DirectInput オブジェクトのポインタ
    IDirectInput8* directInput = nullptr;
    // キーボードデバイスのポインタ
    IDirectInputDevice8* keyboard = nullptr;
    // 全キーの現在の状態を保持する配列
    BYTE keyState_[256]{};
    // 前フレームのキー状態（トリガー判定用）
    BYTE prevKeyState_[256]{};

    // XInput 状態保持
    XINPUT_STATE xinputStates_[XUSER_MAX_COUNT]{};
    bool xinputConnected_[XUSER_MAX_COUNT]{};
    Stick leftSticks_[XUSER_MAX_COUNT]{};

    // パッドボタン状態（現在・前フレーム）
    WORD padButtons_[XUSER_MAX_COUNT]{};
    WORD prevPadButtons_[XUSER_MAX_COUNT]{};

    // コピーコンストラクタと代入演算子を禁止
    KeyInput(const KeyInput&) = delete;
    KeyInput& operator=(const KeyInput&) = delete;

    //WindowsAPI
    WinApp* winApp_ = nullptr;

    // 単一インスタンス参照（定義は .cpp）
    static KeyInput* instance_;
};

// インライン実装
inline bool KeyInput::PushKey(int dik_code) const
{
    if (dik_code >= 0 && dik_code < 256)
    {
        return (keyState_[dik_code] & 0x80) != 0;
    }
    return false;
}

inline bool KeyInput::TriggerKey(int dik_code) const
{
    if (dik_code >= 0 && dik_code < 256)
    {
        bool now = (keyState_[dik_code] & 0x80) != 0;
        bool prev = (prevKeyState_[dik_code] & 0x80) != 0;
        return now && !prev;
    }
    return false;
}

inline KeyInput::Stick KeyInput::GetLeftStick(int controllerIndex) const
{
    if (controllerIndex < 0 || controllerIndex >= XUSER_MAX_COUNT) return { 0.f, 0.f };
    return leftSticks_[controllerIndex];
}

inline bool KeyInput::IsLeftStickMoved(float threshold, int controllerIndex) const
{
    if (controllerIndex < 0 || controllerIndex >= XUSER_MAX_COUNT) return false;
    const Stick& s = leftSticks_[controllerIndex];
    float mag = sqrtf(s.x * s.x + s.y * s.y);
    return mag >= threshold;
}

inline bool KeyInput::IsPadButtonPressed(WORD button, int controllerIndex) const
{
    if (controllerIndex < 0 || controllerIndex >= XUSER_MAX_COUNT) return false;
    return (padButtons_[controllerIndex] & button) != 0;
}

inline bool KeyInput::TriggerPadButton(WORD button, int controllerIndex) const
{
    if (controllerIndex < 0 || controllerIndex >= XUSER_MAX_COUNT) return false;
    return ((padButtons_[controllerIndex] & button) != 0) &&
        ((prevPadButtons_[controllerIndex] & button) == 0);
}

inline KeyInput* KeyInput::GetInstance()
{
    return instance_;
}