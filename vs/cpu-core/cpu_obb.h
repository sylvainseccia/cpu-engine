#pragma once

struct cpu_obb
{
public:
	XMFLOAT3 center;
	XMFLOAT3 axis[3];
	XMFLOAT3 half;

public:
	cpu_obb();
	cpu_obb(const cpu_aabb& aabb);

	cpu_obb& operator=(const cpu_aabb& aabb);

	void Zero();
	void XM_CALLCONV Transform(FXMMATRIX m);
	XMMATRIX GetMatrix();
};
