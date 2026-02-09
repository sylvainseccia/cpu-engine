#pragma once

struct cpu_vinput
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
	cpu_input* pInput;
	byte keys[256];

public:
	cpu_vinput();
	~cpu_vinput();

	void Initialize(cpu_input* pI);
	void Reset();

	bool IsKey(int key);
	bool IsKeyPressed(int key);
	bool IsKeyReleased(int key);

	void Update();
};
