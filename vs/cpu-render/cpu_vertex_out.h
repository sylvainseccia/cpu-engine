#pragma once

struct cpu_vertex_out
{
public:
	XMFLOAT3 worldPos;
	XMFLOAT4 clipPos;
	XMFLOAT3 worldNormal;
	float invW;

	XMFLOAT3 albedo;
	float intensity;
	XMFLOAT2 uv;		// original uv
	XMFLOAT2 ouv;		// over uv (uv * invW)
};
