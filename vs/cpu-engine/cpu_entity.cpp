#include "pch.h"

cpu_entity::cpu_entity()
{
	index = -1;
	sortedIndex = -1;
	dead = false;
	pMesh = nullptr;
	pMaterial = nullptr;
	lifetime = 0.0f;
	tile = 0;
	depth = CPU_DEPTH_READ | CPU_DEPTH_WRITE;
	visible = true;
	clipped = false;
}

void cpu_entity::UpdateWorld(cpu_camera* pCamera, int width, int height)
{
	if ( dead )
		return;

	// World
	XMMATRIX matWorld = XMLoadFloat4x4(&transform.GetWorld());

	// Bounding volumes
	if ( pMesh )
	{
		// cpu_obb
		obb = pMesh->aabb;
		obb.Transform(matWorld);

		// cpu_aabb
		aabb = obb;

		// cpu_sphere
		sphere = obb;

		// Rectangle (screen)
		XMMATRIX matWVP = matWorld;
		matWVP *= XMLoadFloat4x4(&pCamera->matViewProj);
		pMesh->aabb.ToScreen(box, matWVP, width, height);
	}

	// View
	XMMATRIX matView = XMLoadFloat4x4(&pCamera->matView);
	XMVECTOR pos = XMLoadFloat3(&transform.pos);
	pos = XMVector3TransformCoord(pos, matView);
	XMStoreFloat3(&view, pos);
}

void cpu_entity::Clip(cpu_camera* pCamera, int* pStats)
{
	if ( dead || visible==false || pMesh==nullptr )
	{
		clipped = true;
		return;
	}
	if ( pCamera->frustum.Intersect(sphere) )
	{
		clipped = false;
		return;
	}
	clipped = true;
	if ( pStats )
		(*pStats)++;
}
