#pragma once

struct cpu_entity : public cpu_object
{
public:
	cpu_mesh* pMesh;
	cpu_transform transform;
	XMFLOAT3 view;
	cpu_material* pMaterial;
	float lifetime;
	ui64 tile;
	cpu_sphere sphere;
	cpu_aabb aabb;
	cpu_obb obb;
	cpu_rectangle box;
	bool clipped;
	byte depth;
	bool visible;

public:
	cpu_entity();

	void UpdateWorld(cpu_camera* pCamera, int width, int height);
	void Clip(cpu_camera* pCamera, int* pStats = nullptr);
};
