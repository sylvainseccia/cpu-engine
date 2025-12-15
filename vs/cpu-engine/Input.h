#pragma once

struct cpu_input
{
private:
	enum
	{
		_NONE,
		_DOWN,
		_UP,
		_PUSH,
	};

private:
	byte keys[256];

public:
	cpu_input();

	bool IsKey(int key);
	bool IsKeyDown(int key);
	bool IsKeyUp(int key);
	void Update();
};
