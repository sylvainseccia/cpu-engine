#pragma once

struct cpu_draw
{
public:
	XMFLOAT3 tri[3];
	cpu_vertex_out* vo[3];
	cpu_material* pMaterial;
	cpu_tile* pTile;
	byte depth;
};
