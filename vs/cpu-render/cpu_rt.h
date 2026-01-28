#pragma once

struct cpu_rt : public cpu_object
{
public:
	int width;
	int height;
	float aspectRatio;
	int pixelCount;
	int stride;
	float widthHalf;
	float heightHalf;
	std::vector<ui32> colorBuffer;
	bool depth;
	std::vector<float> depthBuffer;

public:
	cpu_rt();

	void Create(int width, int height, bool useDepth = true);
	void Destroy();
};
