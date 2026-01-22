#pragma once

struct cpu_pixel
{
public:
	int x, y;
	float depth;

	XMFLOAT3 albedo;	// unlit
	XMFLOAT3 color;		// lit
	XMFLOAT2 uv;

	XMFLOAT3 normal;
	XMFLOAT3 pos;
};
