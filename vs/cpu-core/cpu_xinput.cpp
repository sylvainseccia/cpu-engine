#include "pch.h"

cpu_xinput::cpu_xinput()
{
	pInput = nullptr;
	Reset();
}

cpu_xinput::~cpu_xinput()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void cpu_xinput::Initialize(cpu_input* pI)
{
	pInput = pI;
	SetDeadZones();
	ForceFeedback();
}

void cpu_xinput::SetDeadZones(float leftStick, float rightStick, float leftButton, float rightButton)
{
	deadZoneLeftStick = leftStick;
	deadZoneRightStick = rightStick;
	deadZoneLeftButton = leftButton;
	deadZoneRightButton = rightButton;
}

void cpu_xinput::Reset()
{
	previous.Reset();
	current.Reset();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool cpu_xinput::IsButton(int id)
{
	assert( id>=0 && id<CPU_XINPUT_COUNT );
	return current.buttons[id];
}

bool cpu_xinput::IsButtonPressed(int id)
{
	assert( id>=0 && id<CPU_XINPUT_COUNT );
	return current.buttons[id] && previous.buttons[id]==false;
}

bool cpu_xinput::IsButtonReleased(int id)
{
	assert( id>=0 && id<CPU_XINPUT_COUNT );
	return current.buttons[id]==false && previous.buttons[id];
}

bool cpu_xinput::IsButtonDouble(int id)
{
	assert( id>=0 && id<CPU_XINPUT_COUNT );
	return current.buttonDoubles[id];
}

bool cpu_xinput::IsAnyButton()
{
	for ( int i=0 ; i<CPU_XINPUT_COUNT ; i++ )
	{
		if ( IsButton(i) )
			return true;
	}
	return false;
}

bool cpu_xinput::IsAnyButtonPressed()
{
	for ( int i=0 ; i<CPU_XINPUT_COUNT ; i++ )
	{
		if ( IsButtonPressed(i) )
			return true;
	}
	return false;
}

bool cpu_xinput::IsAnyButtonReleased()
{
	for ( int i=0 ; i<CPU_XINPUT_COUNT ; i++ )
	{
		if ( IsButtonReleased(i) )
			return true;
	}
	return false;
}

bool cpu_xinput::IsStartPressed()
{
	return current.start && previous.start==false;
}

bool cpu_xinput::IsBackPressed()
{
	return current.back && previous.back==false;
}

bool cpu_xinput::IsLeft()
{
	return current.x<0;
}

bool cpu_xinput::IsLeftPressed()
{
	return current.x<0 && previous.x>=0;
}

bool cpu_xinput::IsLeftReleased()
{
	return current.x>=0 && previous.x<0;
}

bool cpu_xinput::IsRight()
{
	return current.x>0;
}

bool cpu_xinput::IsRightPressed()
{
	return current.x>0 && previous.x<=0;
}

bool cpu_xinput::IsRightReleased()
{
	return current.x<=0 && previous.x>0;
}

bool cpu_xinput::IsUp()
{
	return current.y<0;
}

bool cpu_xinput::IsUpPressed()
{
	return current.y<0 && previous.y>=0;
}

bool cpu_xinput::IsUpReleased()
{
	return current.y>=0 && previous.y<0;
}

bool cpu_xinput::IsDown()
{
	return current.y>0;
}

bool cpu_xinput::IsDownPressed()
{
	return current.y>0 && previous.y<=0;
}

bool cpu_xinput::IsDownReleased()
{
	return current.y<=0 && previous.y>0;
}

bool cpu_xinput::IsAnyLeft()
{
	return current.IsAnyLeft();
}

bool cpu_xinput::IsAnyRight()
{
	return current.IsAnyRight();
}

bool cpu_xinput::IsAnyUp()
{
	return current.IsAnyUp();
}

bool cpu_xinput::IsAnyDown()
{
	return current.IsAnyDown();
}

bool cpu_xinput::IsAnyLeftPressed()
{
	return current.IsAnyLeft() && previous.IsAnyLeft()==false;
}

bool cpu_xinput::IsAnyRightPressed()
{
	return current.IsAnyRight() && previous.IsAnyRight()==false;
}

bool cpu_xinput::IsAnyUpPressed()
{
	return current.IsAnyUp() && previous.IsAnyUp()==false;
}

bool cpu_xinput::IsAnyDownPressed()
{
	return current.IsAnyDown() && previous.IsAnyDown()==false;
}

XMFLOAT2 cpu_xinput::GetLeftStick()
{
	return current.leftStick;
}

XMFLOAT2 cpu_xinput::GetRightStick()
{
	return current.rightStick;
}

float cpu_xinput::GetLeftThumb()
{
	return current.leftThumb;
}

bool cpu_xinput::IsLeftThumb()
{
	return current.leftThumb>0.0f;
}

bool cpu_xinput::IsLeftThumbPressed()
{
	return current.leftThumb>0.0f && previous.leftThumb==0.0f;
}

bool cpu_xinput::IsLeftThumbReleased()
{
	return current.leftThumb==0.0f && previous.leftThumb>0.0f;
}

float cpu_xinput::GetRightThumb()
{
	return current.rightThumb;
}

bool cpu_xinput::IsRightThumb()
{
	return current.rightThumb>0.0f;
}

bool cpu_xinput::IsRightThumbPressed()
{
	return current.rightThumb>0.0f && previous.rightThumb==0.0f;
}

bool cpu_xinput::IsRightThumbReleased()
{
	return current.rightThumb==0.0f && previous.rightThumb>0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void cpu_xinput::ResetForceFeedback()
{
	// Values
	ffbTime = 0.0f;
	ffbDuration = 0.0f;
	ffbLeftMotor = 0.0f;
	ffbRightMotor = 0.0f;

	// Device
	XINPUT_VIBRATION vibration;
	ZeroMemory(&vibration, sizeof(XINPUT_VIBRATION));
	XInputSetState(0, &vibration);
}

void cpu_xinput::ForceFeedback(float leftMotor, float rightMotor, float duration)
{
	// Motors
	leftMotor = leftMotor<0.0f ? ffbLeftMotor : cpu::Clamp(leftMotor);
	rightMotor = rightMotor<0.0f ? ffbRightMotor : cpu::Clamp(rightMotor);

	// Stop
	if ( leftMotor<=0.0f && rightMotor<=0.0f )
	{
		if ( ffbTime==0.0f )
			return;
		ResetForceFeedback();
		return;
	}

	// Time
	ffbTime = cpuTime.total;
	ffbDuration = duration;

	// Skip
	if ( leftMotor==ffbLeftMotor && rightMotor==ffbRightMotor )
		return;

	// Motors
	ffbLeftMotor = leftMotor;
	ffbRightMotor = rightMotor;

	// Device
	XINPUT_VIBRATION vibration;
	ZeroMemory(&vibration, sizeof(XINPUT_VIBRATION));
	vibration.wLeftMotorSpeed = (int)(ffbLeftMotor*65535.0f);
	vibration.wRightMotorSpeed = (int)(ffbRightMotor*65535.0f);
	XInputSetState(0, &vibration);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void cpu_xinput::Update()
{
	// Previous
	previous = current;

	// Focus
	if ( pInput->focus==false )
	{
		Reset();
		return;
	}

	// New
	XINPUT_STATE state;
	if ( XInputGetState(0, &state)!=ERROR_SUCCESS )
	{
		Reset();
		return;
	}

	// Buttons
	current.buttons[CPU_XINPUT_A] = state.Gamepad.wButtons & XINPUT_GAMEPAD_A ? true : false;
	current.buttons[CPU_XINPUT_B] = state.Gamepad.wButtons & XINPUT_GAMEPAD_B ? true : false;
	current.buttons[CPU_XINPUT_X] = state.Gamepad.wButtons & XINPUT_GAMEPAD_X ? true : false;
	current.buttons[CPU_XINPUT_Y] = state.Gamepad.wButtons & XINPUT_GAMEPAD_Y ? true : false;
	current.buttons[CPU_XINPUT_LS] = state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER ? true : false;
	current.buttons[CPU_XINPUT_RS] = state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER ? true : false;
	current.start = state.Gamepad.wButtons & XINPUT_GAMEPAD_START ? true : false;
	current.back = state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK ? true : false;

	// Eight-direction axis
	if ( state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT )
		current.x = -1;
	else if ( state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT )
		current.x = 1;
	else
		current.x = 0;
	if ( state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP )
		current.y = -1;
	else if ( state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN )
		current.y = 1;
	else
		current.y = 0;

	// Left thumb
	current.leftThumb = (float)state.Gamepad.bLeftTrigger;
	if ( current.leftThumb<deadZoneLeftButton )
		current.leftThumb = 0.0f;
	else
		current.leftThumb = (current.leftThumb-deadZoneLeftButton)/(255.0f-deadZoneLeftButton);

	// Right thumb
	current.rightThumb = (float)state.Gamepad.bRightTrigger;
	if ( current.rightThumb<deadZoneRightButton )
		current.rightThumb = 0.0f;
	else
		current.rightThumb = (current.rightThumb-deadZoneRightButton)/(255.0f-deadZoneRightButton);

	// Left stick: X
	current.leftStick.x = (float)state.Gamepad.sThumbLX;
	if ( fabsf(current.leftStick.x)<deadZoneLeftStick )
		current.leftStick.x = 0.0f;
	else
	{
		if ( current.leftStick.x>0.0f )
			current.leftStick.x = (current.leftStick.x-deadZoneLeftStick)/(32767.0f-deadZoneLeftStick);
		else
			current.leftStick.x = (current.leftStick.x+deadZoneLeftStick)/(32768.0f-deadZoneLeftStick);
	}

	// Left stick: Y
	current.leftStick.y = (float)-state.Gamepad.sThumbLY;
	if ( fabsf(current.leftStick.y)<deadZoneLeftStick )
		current.leftStick.y = 0.0f;
	else
	{
		if ( current.leftStick.y>0.0f )
			current.leftStick.y = (current.leftStick.y-deadZoneLeftStick)/(32768.0f-deadZoneLeftStick);
		else
			current.leftStick.y = (current.leftStick.y+deadZoneLeftStick)/(32767.0f-deadZoneLeftStick);
	}

	// Right stick: X
	current.rightStick.x = (float)state.Gamepad.sThumbRX;
	if ( fabsf(current.rightStick.x)<deadZoneRightStick )
		current.rightStick.x = 0.0f;
	else
	{
		if ( current.rightStick.x>0.0f )
			current.rightStick.x = (current.rightStick.x-deadZoneLeftStick)/(32767.0f-deadZoneLeftStick);
		else
			current.rightStick.x = (current.rightStick.x+deadZoneLeftStick)/(32768.0f-deadZoneLeftStick);
	}
	
	// Right stick: Y
	current.rightStick.y = (float)-state.Gamepad.sThumbRY;
	if ( fabsf(current.rightStick.y)<deadZoneRightStick )
		current.rightStick.y = 0.0f;
	else
	{
		if ( current.rightStick.y>0.0f )
			current.rightStick.y = (current.rightStick.y-deadZoneLeftStick)/(32768.0f-deadZoneLeftStick);
		else
			current.rightStick.y = (current.rightStick.y+deadZoneLeftStick)/(32767.0f-deadZoneLeftStick);
	}

	// Double fire
	for ( int i=0 ; i<CPU_XINPUT_COUNT ; i++ )
	{
		if ( IsButtonPressed(i) )
		{
			current.buttonDoubles[i] = cpuTime.Since(current.buttonTimes[i])<0.25f;
			current.buttonTimes[i] = cpuTime.total;
		}
		else
			current.buttonDoubles[i] = false;
	}

	// Force Feedback
	if ( ffbTime && ffbDuration>0.0f && cpuTime.Since(ffbTime)>=ffbDuration )
		ForceFeedback();
}
