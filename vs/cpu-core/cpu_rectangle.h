#pragma once

struct cpu_rectangle
{
public:
	int minX;	// inclusive
	int maxX;	// exclusive

	int minY;	// inclusive
	int maxY;	// exclusive

public:
	cpu_rectangle();

	void Zero();
	bool IsEmpty();
};
