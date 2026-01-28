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
	cpu_window* pWindow;

public:
	static cpu_input& GetInstance();

	void Update();

	void Reset();
	void SetWindow(cpu_window* pWnd);
	bool IsKey(int key);
	bool IsKeyDown(int key);
	bool IsKeyUp(int key);

private:
	cpu_input();
	~cpu_input();
};
