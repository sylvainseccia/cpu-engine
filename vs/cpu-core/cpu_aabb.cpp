#include "pch.h"

cpu_aabb::cpu_aabb()
{
	Zero();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

cpu_aabb& cpu_aabb::operator=(const cpu_obb& obb)
{
	XMVECTOR c  = XMLoadFloat3(&obb.center);

	// mask pour abs sur float: 0x7fffffff
	const XMVECTOR signMask = XMVectorReplicateInt(0x7fffffff);
	XMVECTOR a0 = XMVectorAndInt(XMLoadFloat3(&obb.axis[0]), signMask);
	XMVECTOR a1 = XMVectorAndInt(XMLoadFloat3(&obb.axis[1]), signMask);
	XMVECTOR a2 = XMVectorAndInt(XMLoadFloat3(&obb.axis[2]), signMask);

	// extents = |axis0|*half.x + |axis1|*half.y + |axis2|*half.z
	XMVECTOR e = XMVectorAdd(XMVectorAdd(XMVectorScale(a0, obb.half.x), XMVectorScale(a1, obb.half.y)), XMVectorScale(a2, obb.half.z));
	XMVECTOR vmin = XMVectorSubtract(c, e);
	XMVECTOR vmax = XMVectorAdd(c, e);

	XMStoreFloat3(&min, vmin);
	XMStoreFloat3(&max, vmax);
	return *this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void cpu_aabb::Zero()
{
	min = { 0.0f, 0.0f, 0.0f };
	max = { 0.0f, 0.0f, 0.0f };
}

bool XM_CALLCONV cpu_aabb::ToScreen(cpu_rectangle& out, FXMMATRIX wvp, float renderWidth, float renderHeight)
{
	const float renderX = 0.0f;
	const float renderY = 0.0f;
	float minX =  FLT_MAX, minY =  FLT_MAX;
	float maxX = -FLT_MAX, maxY = -FLT_MAX;

	const float xmin = min.x;
	const float ymin = min.y;
	const float zmin = min.z;
	const float xmax = max.x;
	const float ymax = max.y;
	const float zmax = max.z;

	const XMVECTOR pts[8] =
	{
		XMVectorSet(xmin, ymin, zmin, 1.0f),
		XMVectorSet(xmax, ymin, zmin, 1.0f),
		XMVectorSet(xmax, ymax, zmin, 1.0f),
		XMVectorSet(xmin, ymax, zmin, 1.0f),
		XMVectorSet(xmin, ymin, zmax, 1.0f),
		XMVectorSet(xmax, ymin, zmax, 1.0f),
		XMVectorSet(xmax, ymax, zmax, 1.0f),
		XMVectorSet(xmin, ymax, zmax, 1.0f)
	};

	for ( int i=0 ; i<8 ; ++i )
	{
		XMVECTOR clip = XMVector4Transform(pts[i], wvp);

		float w = XMVectorGetW(clip);
		if ( w<=0.00001f )
			continue; // derrière caméra

		float invW = 1.0f / w;
		float ndcX = XMVectorGetX(clip) * invW;
		float ndcY = XMVectorGetY(clip) * invW;

		float sx = renderX + (ndcX * 0.5f + 0.5f) * renderWidth;
		float sy = renderY + (-ndcY * 0.5f + 0.5f) * renderHeight;

		minX = std::min(minX, sx);
		minY = std::min(minY, sy);
		maxX = std::max(maxX, sx);
		maxY = std::max(maxY, sy);
	}

	// Outside
	if ( minX>maxX || minY>maxY )
		return false;

	minX = cpu::Clamp(minX, renderX, renderX+renderWidth);
	maxX = cpu::Clamp(maxX, renderX, renderX+renderWidth);
	minY = cpu::Clamp(minY, renderY, renderY+renderHeight);
	maxY = cpu::Clamp(maxY, renderY, renderY+renderHeight);
	out.min = { minX, minY };
	out.max = { maxX, maxY };
	return true;
}

bool cpu_aabb::Contains(const XMFLOAT3& p)
{
	return p.x>=min.x && p.x<=max.x && p.y>=min.y && p.y<=max.y && p.z>=min.z && p.z<=max.z;
}
