#pragma once

struct cpu_vertex
{
public:
	XMFLOAT3 pos;
	XMFLOAT3 color;
	XMFLOAT3 normal;
	XMFLOAT2 uv;

public:
	cpu_vertex();

	void Identity();
};
