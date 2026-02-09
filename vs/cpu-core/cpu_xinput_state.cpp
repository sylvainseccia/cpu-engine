#include "pch.h"

cpu_xinput_state::cpu_xinput_state()
{
	Reset();
}

cpu_xinput_state::~cpu_xinput_state()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void cpu_xinput_state::Reset()
{
	memset(this, 0, sizeof(cpu_xinput_state));
}

bool cpu_xinput_state::IsButton(int id)
{
	assert( id>=0 && id<CPU_XINPUT_COUNT );
	return buttons[id];
}

bool cpu_xinput_state::IsAnyButton()
{
	for ( int i=0 ; i<CPU_XINPUT_COUNT ; i++ )
	{
		if ( buttons[i] )
			return true;
	}
	return false;
}

bool cpu_xinput_state::IsAnyLeft()
{
	return x<0 || leftStick.x<0.0f || rightStick.x<0.0f;
}

bool cpu_xinput_state::IsAnyRight()
{
	return x>0 || leftStick.x>0.0f || rightStick.x>0.0f;
}

bool cpu_xinput_state::IsAnyUp()
{
	return y<0 || leftStick.y<0.0f || rightStick.y<0.0f;
}

bool cpu_xinput_state::IsAnyDown()
{
	return y>0 || leftStick.y>0.0f || rightStick.y>0.0f;
}

bool cpu_xinput_state::IsLeft()
{
	return x<0;
}

bool cpu_xinput_state::IsRight()
{
	return x>0;
}

bool cpu_xinput_state::IsUp()
{
	return y<0;
}

bool cpu_xinput_state::IsDown()
{
	return y>0;
}

bool cpu_xinput_state::IsLeftThumb()
{
	return leftThumb>0.0f;
}

bool cpu_xinput_state::IsRightThumb()
{
	return rightThumb>0.0f;
}

bool cpu_xinput_state::IsLeftStickLeft()
{
	return leftStick.x<0.0f;
}

bool cpu_xinput_state::IsLeftStickRight()
{
	return leftStick.x>0.0f;
}

bool cpu_xinput_state::IsLeftStickUp()
{
	return leftStick.y<0.0f;
}

bool cpu_xinput_state::IsLeftStickDown()
{
	return leftStick.y>0.0f;
}

bool cpu_xinput_state::IsRightStickLeft()
{
	return rightStick.x<0.0f;
}

bool cpu_xinput_state::IsRightStickRight()
{
	return rightStick.x>0.0f;
}

bool cpu_xinput_state::IsRightStickUp()
{
	return rightStick.y<0.0f;
}

bool cpu_xinput_state::IsRightStickDown()
{
	return rightStick.y>0.0f;
}
