#include "pch.h"

cpu_obb::cpu_obb()
{
	Zero();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

cpu_obb& cpu_obb::operator=(const cpu_aabb& aabb)
{
	const float xmin = aabb.min.x, ymin = aabb.min.y, zmin = aabb.min.z;
	const float xmax = aabb.max.x, ymax = aabb.max.y, zmax = aabb.max.z;

	const float sx = (aabb.max.x-aabb.min.x) * 0.5f;
	const float sy = (aabb.max.y-aabb.min.y) * 0.5f;
	const float sz = (aabb.max.z-aabb.min.z) * 0.5f;

	center = { aabb.min.x+sx, aabb.min.y+sy, aabb.min.z+sz };
	axis[0] = { 1.f, 0.f, 0.f };
	axis[1] = { 0.f, 1.f, 0.f };
	axis[2] = { 0.f, 0.f, 1.f };
	half = { sx, sy, sz };

	return *this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void cpu_obb::Zero()
{
	center = {};
	axis[0] = {};
	axis[1] = {};
	axis[2] = {};
	half = {};
}

void XM_CALLCONV cpu_obb::Transform(FXMMATRIX m)
{
	XMVECTOR c = XMLoadFloat3(&center);
	c = XMVector3TransformCoord(c, m);
	XMStoreFloat3(&center, c);

	const float halfLocal[3] = { half.x, half.y, half.z };
	for ( int i=0 ; i<3 ; ++i )
	{
		XMVECTOR a = XMLoadFloat3(&axis[i]);

		// transform direction (no translation)
		XMVECTOR ta = XMVector3TransformNormal(a, m);

		// scale magnitude along that axis
		float s = XMVectorGetX(XMVector3Length(ta));

		// normalize axis (guard scale=0)
		if ( s>0.0f )
			ta = XMVectorScale(ta, 1.0f/s);
		else
			ta = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);

		XMStoreFloat3(&axis[i], ta);

		// half-length scales by s
		if ( i==0 )
			half.x = halfLocal[i] * s;
		if ( i==1 )
			half.y = halfLocal[i] * s;
		if ( i==2 )
			half.z = halfLocal[i] * s;
	}
}

XMMATRIX cpu_obb::GetMatrix()
{
	XMVECTOR s = XMVectorSet(2.0f * half.x, 2.0f * half.y, 2.0f * half.z, 0.0f);
	XMVECTOR sx = XMVectorSplatX(s);
	XMVECTOR sy = XMVectorSplatY(s);
	XMVECTOR sz = XMVectorSplatZ(s);

	XMMATRIX w;
	w.r[0] = XMVectorSet(axis[0].x, axis[0].y, axis[0].z, 0.0f);
	w.r[1] = XMVectorSet(axis[1].x, axis[1].y, axis[1].z, 0.0f);
	w.r[2] = XMVectorSet(axis[2].x, axis[2].y, axis[2].z, 0.0f);

	w.r[0] = XMVectorMultiply(w.r[0], sx);
	w.r[1] = XMVectorMultiply(w.r[1], sy);
	w.r[2] = XMVectorMultiply(w.r[2], sz);

	XMVECTOR p = XMLoadFloat3(&center);
	w.r[3] = XMVectorSetW(p, 1.0f);
	return w;
}

//XMMATRIX cpu_obb::GetMatrix()
//{
//	XMMATRIX r =
//		XMMATRIX(
//			axis[0].x, axis[0].y, axis[0].z, 0.0f,
//			axis[1].x, axis[1].y, axis[1].z, 0.0f,
//			axis[2].x, axis[2].y, axis[2].z, 0.0f,
//			0.0f, 0.0f, 0.0f, 1.0f
//		);
//
//	XMMATRIX s = XMMatrixScaling(2.0f*half.x, 2.0f*half.y, 2.0f*half.z);
//	XMMATRIX t = XMMatrixTranslation(center.x, center.y, center.z);
//	return s * r * t;
//}
