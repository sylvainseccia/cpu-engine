#pragma once

struct cpu_aabb
{
	XMFLOAT3 min;
	XMFLOAT3 max;

	cpu_aabb();
	cpu_aabb(const cpu_obb& obb);

	cpu_aabb& operator=(const cpu_obb& obb);

	void Zero();
	bool XM_CALLCONV ToScreen(cpu_rectangle& out, FXMMATRIX wvp, float renderWidth, float renderHeight);
	bool Contains(const XMFLOAT3& p);
};
