#pragma once

struct cpu_xinput_state
{
public:
	bool start;
	bool back;
	bool buttons[CPU_XINPUT_COUNT];
	float buttonTimes[CPU_XINPUT_COUNT];
	bool buttonDoubles[CPU_XINPUT_COUNT];
	int x; // pad: -1, 0, 1
	int y; // pad: -1, 0, 1
	float leftThumb;
	float rightThumb;
	XMFLOAT2 leftStick;
	XMFLOAT2 rightStick;

public:
	cpu_xinput_state();
	~cpu_xinput_state();

	void Reset();
	bool IsButton(int id = CPU_XINPUT_A);
	bool IsAnyButton();
	bool IsAnyLeft();
	bool IsAnyRight();
	bool IsAnyUp();
	bool IsAnyDown();
	bool IsLeft();
	bool IsRight();
	bool IsUp();
	bool IsDown();
	bool IsLeftThumb();
	bool IsRightThumb();
	bool IsLeftStickLeft();
	bool IsLeftStickRight();
	bool IsLeftStickUp();
	bool IsLeftStickDown();
	bool IsRightStickLeft();
	bool IsRightStickRight();
	bool IsRightStickUp();
	bool IsRightStickDown();
};
