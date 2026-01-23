#pragma once

struct cpu_frustum
{
public:
	XMFLOAT4 planes[6]; // left, right, bottom, top, near, far

public:
	cpu_frustum();

	void FromViewProj(const XMFLOAT4X4& viewProj);
	bool Intersect(const cpu_sphere& sphere);
	XMVECTOR XM_CALLCONV NormalizePlane(FXMVECTOR p);
};
