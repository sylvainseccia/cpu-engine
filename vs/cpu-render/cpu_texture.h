#pragma once

struct cpu_texture
{
public:
	byte* bgra;
	int width;
	int height;
	int count;
	int size;

private:
	static float lut[256];

public:
	cpu_texture();
	~cpu_texture();

	bool Load(const char* path);
	void Close();
	void Sample(XMFLOAT3& outColor, float x, float y);

	static void Init();

private:
	int WrapPow2(int i, int sizePow2);
	int FastFloorToInt(float x);
};
