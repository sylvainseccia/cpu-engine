#pragma once

class Input
{
public:
	enum
	{
		_NONE,
		_DOWN,
		_UP,
		_PUSH,
	};

public:
	Input();
	virtual ~Input();

	bool IsKey(int key);
	bool IsKeyDown(int key);
	bool IsKeyUp(int key);
	void Update();

protected:
	byte m_keys[256];
};
