#pragma once

struct cpu_xinput
{
private:
	cpu_input* pInput;

	float deadZoneLeftStick;
	float deadZoneRightStick;
	float deadZoneLeftButton;
	float deadZoneRightButton;

	float ffbTime;
	float ffbDuration;
	float ffbLeftMotor;
	float ffbRightMotor;

	cpu_xinput_state previous;
	cpu_xinput_state current;

public:
	const cpu_xinput_state& state = current;

public:
	cpu_xinput();
	~cpu_xinput();

	void Initialize(cpu_input* pI);
	void SetWindow(cpu_window* pWnd);
	void SetDeadZones(float leftStick = 7849.0f, float rightStick = 8689.0f, float leftButton = 30.0f, float rightButton = 30.0f);
	void Reset();

	bool IsButton(int id = CPU_XINPUT_A);
	bool IsButtonPressed(int id = CPU_XINPUT_A);
	bool IsButtonReleased(int id = CPU_XINPUT_A);
	bool IsButtonDouble(int id = CPU_XINPUT_A);
	bool IsAnyButton();
	bool IsAnyButtonPressed();
	bool IsAnyButtonReleased();
	bool IsStartPressed();
	bool IsBackPressed();
	bool IsLeft();
	bool IsLeftPressed();
	bool IsLeftReleased();
	bool IsRight();
	bool IsRightPressed();
	bool IsRightReleased();
	bool IsUp();
	bool IsUpPressed();
	bool IsUpReleased();
	bool IsDown();
	bool IsDownPressed();
	bool IsDownReleased();
	bool IsAnyLeft();
	bool IsAnyRight();
	bool IsAnyUp();
	bool IsAnyDown();
	bool IsAnyLeftPressed();
	bool IsAnyRightPressed();
	bool IsAnyUpPressed();
	bool IsAnyDownPressed();
	XMFLOAT2 GetLeftStick();
	XMFLOAT2 GetRightStick();
	float GetLeftThumb();
	bool IsLeftThumb();
	bool IsLeftThumbPressed();
	bool IsLeftThumbReleased();
	float GetRightThumb();
	bool IsRightThumb();
	bool IsRightThumbPressed();
	bool IsRightThumbReleased();

	void ResetForceFeedback();
	void ForceFeedback(float leftMotor = 0.0f, float rightMotor = 0.0f, float duration = 0.0f);
	bool IsForceFeedback();

	void Update();
};
