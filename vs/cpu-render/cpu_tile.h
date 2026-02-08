#pragma once

struct cpu_tile
{
public:
	int left;
	int top;
	int right;
	int bottom;
	int row;
	int col;

	// Entity
	int statsDrawnTriangleCount;

	// Particle
	std::vector<int> particleLocalCounts;
	int particleCount;
	int particleOffset;
	int particleOffsetTemp;

public:
	void Reset();
};
