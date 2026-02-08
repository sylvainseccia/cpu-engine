#include "pch.h"

cpu_aabb::cpu_aabb()
{
	Zero();
}

cpu_aabb::cpu_aabb(const cpu_obb& obb)
{
	*this = obb;
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

bool XM_CALLCONV cpu_aabb::ToScreen(cpu_rectangle& out, FXMMATRIX wvp, int width, int height)
{
	const float renderWidth = (float)width;
	const float renderHeight = (float)height;

	const XMVECTOR pts[8] =
	{
		XMVectorSet(min.x, min.y, min.z, 1.0f),
		XMVectorSet(max.x, min.y, min.z, 1.0f),
		XMVectorSet(max.x, max.y, min.z, 1.0f),
		XMVectorSet(min.x, max.y, min.z, 1.0f),
		XMVectorSet(min.x, min.y, max.z, 1.0f),
		XMVectorSet(max.x, min.y, max.z, 1.0f),
		XMVectorSet(max.x, max.y, max.z, 1.0f),
		XMVectorSet(min.x, max.y, max.z, 1.0f)
	};

	float minX =  FLT_MAX, minY =  FLT_MAX;
	float maxX = -FLT_MAX, maxY = -FLT_MAX;
	for ( int i=0 ; i<8 ; ++i )
	{
		XMVECTOR clip = XMVector4Transform(pts[i], wvp);

		float w = XMVectorGetW(clip);
		if ( w<CPU_EPSILON )
			w = CPU_EPSILON;

		float invW = 1.0f / w;
		float ndcX = XMVectorGetX(clip) * invW;
		float ndcY = XMVectorGetY(clip) * invW;
		ndcX = cpu::Clamp(ndcX, -1.0f, 1.0f);
		ndcY = cpu::Clamp(ndcY, -1.0f, 1.0f);

		float sx = (ndcX * 0.5f + 0.5f) * renderWidth;
		float sy = (-ndcY * 0.5f + 0.5f) * renderHeight;

		minX = std::min(minX, sx);
		minY = std::min(minY, sy);
		maxX = std::max(maxX, sx);
		maxY = std::max(maxY, sy);
	}

	int iMinX = cpu::Clamp(cpu::FloorToInt(minX), 0, width);
	int iMaxX = cpu::Clamp(cpu::CeilToInt(maxX), 0, width);
	int iMinY = cpu::Clamp(cpu::FloorToInt(minY), 0, height);
	int iMaxY = cpu::Clamp(cpu::CeilToInt(maxY), 0, height);
	if ( iMaxX<=iMinX || iMaxY<=iMinY )
		return false;

	out.minX = iMinX;
	out.maxX = iMaxX;
	out.minY = iMinY;
	out.maxY = iMaxY;
	return true;
}

bool cpu_aabb::Contains(const XMFLOAT3& p)
{
	return p.x>=min.x && p.x<max.x && p.y>=min.y && p.y<max.y && p.z>=min.z && p.z<max.z;
}
