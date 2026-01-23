#pragma once

struct cpu_sphere
{
public:
	XMFLOAT3 center;
	float radius;

public:
	cpu_sphere();

	cpu_sphere& operator=(const cpu_aabb& aabb);
	cpu_sphere& operator=(const cpu_obb& obb);

	void Zero();
	void XM_CALLCONV Transform(FXMMATRIX m);
};
