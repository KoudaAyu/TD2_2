#include "Controller.h"
#include <cmath>

Controller::Controller(unsigned int index)
    : m_index(index)
{
    ZeroMemory(&m_state, sizeof(m_state));
    ZeroMemory(&m_prevState, sizeof(m_prevState));
    m_connected = false;
}

void Controller::Update()
{
    m_prevState = m_state;
    DWORD result = XInputGetState((DWORD)m_index, &m_state);
    m_connected = (result == ERROR_SUCCESS);
}

bool Controller::IsConnected() const { return m_connected; }

bool Controller::IsButtonDown(WORD button) const
{
    if (!m_connected) return false;
    return (m_state.Gamepad.wButtons & button) != 0;
}

bool Controller::WasButtonDown(WORD button) const
{
    if (!m_connected) return false;
    return (m_prevState.Gamepad.wButtons & button) != 0;
}

bool Controller::WasButtonPressedThisFrame(WORD button) const
{
    return IsButtonDown(button) && !WasButtonDown(button);
}

bool Controller::WasButtonReleasedThisFrame(WORD button) const
{
    return !IsButtonDown(button) && WasButtonDown(button);
}

float Controller::GetLeftTrigger() const
{
    if (!m_connected) return 0.0f;
    return static_cast<float>(m_state.Gamepad.bLeftTrigger) / 255.0f;
}

float Controller::GetRightTrigger() const
{
    if (!m_connected) return 0.0f;
    return static_cast<float>(m_state.Gamepad.bRightTrigger) / 255.0f;
}

Controller::Stick Controller::GetLeftStick(float deadzone) const
{
    return NormalizeStick(m_state.Gamepad.sThumbLX, m_state.Gamepad.sThumbLY, deadzone);
}

Controller::Stick Controller::GetRightStick(float deadzone) const
{
    return NormalizeStick(m_state.Gamepad.sThumbRX, m_state.Gamepad.sThumbRY, deadzone);
}

void Controller::SetVibration(float leftMotor, float rightMotor)
{
    if (!m_connected) return;
    XINPUT_VIBRATION vib{};
    leftMotor = Clamp01(leftMotor);
    rightMotor = Clamp01(rightMotor);
    vib.wLeftMotorSpeed = static_cast<WORD>(leftMotor * 65535.0f);
    vib.wRightMotorSpeed = static_cast<WORD>(rightMotor * 65535.0f);
    XInputSetState((DWORD)m_index, &vib);
}

const XINPUT_STATE& Controller::RawState() const { return m_state; }

unsigned int Controller::Index() const { return m_index; }
void Controller::SetIndex(unsigned int idx) { m_index = idx; }

float Controller::Clamp01(float v) { return (v < 0.0f) ? 0.0f : ((v > 1.0f) ? 1.0f : v); }

Controller::Stick Controller::NormalizeStick(SHORT rawX, SHORT rawY, float deadzone)
{
    Stick out{0.0f, 0.0f};

    if (rawX == 0 && rawY == 0) return out;

    float fx = static_cast<float>(rawX);
    float fy = static_cast<float>(rawY);

    float mag = std::sqrt(fx * fx + fy * fy);
    if (mag <= deadzone) return out;

    float maxMag = 32767.0f;
    float normalizedX = fx / maxMag;
    float normalizedY = fy / maxMag;

    float scaledMag = (mag - deadzone) / (maxMag - deadzone);
    float unitX = normalizedX / (mag / maxMag);
    float unitY = normalizedY / (mag / maxMag);

    out.x = unitX * scaledMag;
    out.y = unitY * scaledMag;
    return out;
}
