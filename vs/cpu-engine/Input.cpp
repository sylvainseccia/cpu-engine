#include "stdafx.h"

Input::Input()
{
	memset(m_keys, 0, 256);
}

Input::~Input()
{
}

bool Input::IsKey(int key)
{
	return m_keys[key]==_DOWN || m_keys[key]==_PUSH;
}

bool Input::IsKeyUp(int key)
{
	return m_keys[key]==_UP;
}

bool Input::IsKeyDown(int key)
{
	return m_keys[key]==_DOWN;
}

void Input::Update()
{
	for ( int i=1 ; i<255 ; i++ )
	{
		if ( GetAsyncKeyState(i)<0 )
		{
			switch ( m_keys[i] )
			{
			case _NONE:
			case _UP:
				m_keys[i] = _DOWN;
				break;
			case _DOWN:
				m_keys[i] = _PUSH;
				break;
			}
		}
		else
		{
			switch ( m_keys[i] )
			{
			case _PUSH:
			case _DOWN:
				m_keys[i] = _UP;
				break;
			case _UP:
				m_keys[i] = _NONE;
				break;
			}
		}
	}
}
