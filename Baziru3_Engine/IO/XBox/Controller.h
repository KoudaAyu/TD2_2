#pragma once

#include <Windows.h>
#include <Xinput.h>

class Controller
{
public:
    struct Stick
    {
        float x{0.0f};
        float y{0.0f};
    };

    Controller(unsigned int index = 0);

    void Update();

    bool IsConnected() const;

    bool IsButtonDown(WORD button) const;
    bool WasButtonDown(WORD button) const;
    bool WasButtonPressedThisFrame(WORD button) const;
    bool WasButtonReleasedThisFrame(WORD button) const;

    float GetLeftTrigger() const;
    float GetRightTrigger() const;

    Stick GetLeftStick(float deadzone = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) const;
    Stick GetRightStick(float deadzone = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) const;

    void SetVibration(float leftMotor, float rightMotor);

    const XINPUT_STATE& RawState() const;

    unsigned int Index() const;
    void SetIndex(unsigned int idx);

private:
    static float Clamp01(float v);
    static Stick NormalizeStick(SHORT rawX, SHORT rawY, float deadzone);

    unsigned int m_index{0};
    XINPUT_STATE m_state{};
    XINPUT_STATE m_prevState{};
    bool m_connected{false};
};

