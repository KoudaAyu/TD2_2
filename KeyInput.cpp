#include "KeyInput.h"

#include<cassert>
#include <cmath>
#include <cstring>

// static インスタンスの定義
KeyInput* KeyInput::instance_ = nullptr;

KeyInput::KeyInput()
{
}

KeyInput::~KeyInput()
{
    if (KeyInput::instance_ == this)
    {
        KeyInput::instance_ = nullptr;
    }

    if (keyboard)
    {
        keyboard->Unacquire();
        keyboard->Release();
        keyboard = nullptr;
    }
    if (directInput)
    {
        directInput->Release();
        directInput = nullptr;
    }
}

void KeyInput::Initialize(WinApp* winApp)
{
    winApp_ = winApp;

    HRESULT result = DirectInput8Create(
        winApp->GetHInstance(), DIRECTINPUT_VERSION, IID_IDirectInput8,
        (void**)&directInput, nullptr);

    result = directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);

    result = keyboard->SetDataFormat(&c_dfDIKeyboard);

    result = keyboard->SetCooperativeLevel(winApp->GetHwnd(), DISCL_BACKGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);

    // 初期化
    ZeroMemory(keyState_, sizeof(keyState_));
    ZeroMemory(prevKeyState_, sizeof(prevKeyState_));
    for (int i = 0; i < XUSER_MAX_COUNT; ++i)
    {
        xinputConnected_[i] = false;
        leftSticks_[i] = { 0.f, 0.f };
        padButtons_[i] = 0;
        prevPadButtons_[i] = 0;
    }

    // グローバル参照を設定
    KeyInput::instance_ = this;
}

static inline float NormalizeThumb(SHORT value, SHORT deadzone)
{
    if (abs(value) < deadzone) return 0.0f;
    if (value < 0) return (float)value / 32768.0f;
    return (float)value / 32767.0f;
}

void KeyInput::Update()
{
    // 前フレームのキー状態を保存
    std::memcpy(prevKeyState_, keyState_, sizeof(keyState_));

    // キーボード情報の取得
    if (keyboard)
    {
        keyboard->Acquire();
        HRESULT hr = keyboard->GetDeviceState(sizeof(keyState_), keyState_);
        if (FAILED(hr))
        {
            ZeroMemory(keyState_, sizeof(keyState_));
        }
    }

    // XInput のポーリング（最大 4 コントローラ）
    for (DWORD i = 0; i < XUSER_MAX_COUNT; ++i)
    {
        // 前フレームのボタン状態を保存
        prevPadButtons_[i] = padButtons_[i];

        XINPUT_STATE state{};
        DWORD res = XInputGetState(i, &state);
        if (res == ERROR_SUCCESS)
        {
            xinputConnected_[i] = true;
            xinputStates_[i] = state;

            // ボタン状態更新
            padButtons_[i] = state.Gamepad.wButtons;

            // 左スティックを正規化（デッドゾーン処理）
            SHORT lx = state.Gamepad.sThumbLX;
            SHORT ly = state.Gamepad.sThumbLY;
            SHORT dead = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;

            float nx = NormalizeThumb(lx, dead);
            float ny = NormalizeThumb(ly, dead);

            leftSticks_[i].x = nx;
            leftSticks_[i].y = ny;
        }
        else
        {
            xinputConnected_[i] = false;
            padButtons_[i] = 0;
            leftSticks_[i] = { 0.f, 0.f };
        }
    }
}