#pragma once

struct cpu_object
{
	int index;
	int sortedIndex;
	bool dead;
	bool active;

	cpu_object();
	virtual ~cpu_object() = default;

	bool IsActive() { return dead==false && active; }
};
