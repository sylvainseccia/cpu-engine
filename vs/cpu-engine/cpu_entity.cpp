#include "pch.h"

cpu_entity::cpu_entity()
{
	index = -1;
	sortedIndex = -1;
	dead = false;
	pMesh = nullptr;
	pMaterial = nullptr;
	lifetime = 0.0f;
	tile = 0;
	depth = CPU_DEPTH_READ | CPU_DEPTH_WRITE;
	visible = true;
	clipped = false;
}
