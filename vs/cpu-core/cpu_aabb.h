#pragma once

struct cpu_aabb
{
	XMFLOAT3 min; // inclusive
	XMFLOAT3 max; // exclusive

	cpu_aabb();
	cpu_aabb(const cpu_obb& obb);

	cpu_aabb& operator=(const cpu_obb& obb);

	void Zero();
	bool XM_CALLCONV ToScreen(cpu_rectangle& out, FXMMATRIX wvp, int width, int height);
	bool Contains(const XMFLOAT3& p);
};
