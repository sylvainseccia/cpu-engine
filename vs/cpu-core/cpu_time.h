#pragma once

struct cpu_time
{
private:
	DWORD sysTime;
	DWORD fpsTime;
	int fpsCount;
	int fpsResult;
	float totalTime;
	float deltaTime;

public:
	const int& fps = fpsResult;
	const float& total = totalTime;
	const float& delta = deltaTime;

public:
	static cpu_time& GetInstance();

	bool Update();

	void Reset();
	float Since(float t);
	float Since(float t, float duration);

private:
    cpu_time();
    ~cpu_time();
};
