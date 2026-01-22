#pragma once

struct cpu_mesh
{
public:
	std::vector<cpu_triangle> triangles;
	float radius;
	cpu_aabb aabb;

public:
	cpu_mesh();
	~cpu_mesh() = default;

	void Clear();
	void AddTriangle(cpu_triangle& tri);
	void AddTriangle(XMFLOAT3& a, XMFLOAT3& b, XMFLOAT3& c, XMFLOAT3& color);
	void AddTriangle(XMFLOAT3& a, XMFLOAT3& b, XMFLOAT3& c, XMFLOAT2& auv, XMFLOAT2& buv, XMFLOAT2& cuv, XMFLOAT3& color);
	void AddFace(XMFLOAT3& a, XMFLOAT3& b, XMFLOAT3& c, XMFLOAT3& d, XMFLOAT3& color);
	void Optimize();
	void CalculateNormals();
	void CalculateBox();
	void CreatePlane(float width = 1.0f, float height = 1.0f, XMFLOAT3 color = CPU_WHITE);
	void CreateCube(float halfSize = 0.5f, XMFLOAT3 color = CPU_WHITE);
	void CreateCircle(float radius = 0.5f, int count = 6, XMFLOAT3 color = CPU_WHITE);
	void CreateSphere(float radius = 0.5f, int stacks = 5, int slices = 5, XMFLOAT3 color1 = CPU_WHITE, XMFLOAT3 color2 = CPU_WHITE);
	void CreateSpaceship();
};
