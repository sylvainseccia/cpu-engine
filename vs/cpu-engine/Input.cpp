#include "stdafx.h"

cpu_input::cpu_input()
{
	memset(keys, 0, 256);
}

bool cpu_input::IsKey(int key)
{
	return keys[key]==_DOWN || keys[key]==_PUSH;
}

bool cpu_input::IsKeyUp(int key)
{
	return keys[key]==_UP;
}

bool cpu_input::IsKeyDown(int key)
{
	return keys[key]==_DOWN;
}

void cpu_input::Update()
{
	for ( int i=1 ; i<255 ; i++ )
	{
		if ( GetAsyncKeyState(i)<0 )
		{
			switch ( keys[i] )
			{
			case _NONE:
			case _UP:
				keys[i] = _DOWN;
				break;
			case _DOWN:
				keys[i] = _PUSH;
				break;
			}
		}
		else
		{
			switch ( keys[i] )
			{
			case _PUSH:
			case _DOWN:
				keys[i] = _UP;
				break;
			case _UP:
				keys[i] = _NONE;
				break;
			}
		}
	}
}
