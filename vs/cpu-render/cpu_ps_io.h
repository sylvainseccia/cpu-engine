#pragma once

struct cpu_ps_io
{
public:
	// Input
	cpu_material* pMaterial;
	cpu_pixel p;
	void* values;

	// Output
	XMFLOAT3 color;
	bool discard;
};
