#include "pch.h"

cpu_frustum::cpu_frustum()
{
	for ( int i=0 ; i<6 ; i++ )
		planes[i] = { 0.0f, 0.0f, 0.0f, 0.0f };
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void cpu_frustum::FromViewProj(const XMFLOAT4X4& viewProj)
{
	XMVECTOR left   = XMVectorSet(viewProj._14 + viewProj._11, viewProj._24 + viewProj._21, viewProj._34 + viewProj._31, viewProj._44 + viewProj._41);
	XMVECTOR right  = XMVectorSet(viewProj._14 - viewProj._11, viewProj._24 - viewProj._21, viewProj._34 - viewProj._31, viewProj._44 - viewProj._41);
	XMVECTOR bottom = XMVectorSet(viewProj._14 + viewProj._12, viewProj._24 + viewProj._22, viewProj._34 + viewProj._32, viewProj._44 + viewProj._42);
	XMVECTOR top    = XMVectorSet(viewProj._14 - viewProj._12, viewProj._24 - viewProj._22, viewProj._34 - viewProj._32, viewProj._44 - viewProj._42);

	XMVECTOR nearP  = XMVectorSet(viewProj._13, viewProj._23, viewProj._33, viewProj._43);
	XMVECTOR farP   = XMVectorSet(viewProj._14-viewProj._13, viewProj._24-viewProj._23, viewProj._34-viewProj._33, viewProj._44-viewProj._43);

	XMStoreFloat4(&planes[0], NormalizePlane(left));
	XMStoreFloat4(&planes[1], NormalizePlane(right));
	XMStoreFloat4(&planes[2], NormalizePlane(bottom));
	XMStoreFloat4(&planes[3], NormalizePlane(top));
	XMStoreFloat4(&planes[4], NormalizePlane(nearP));
	XMStoreFloat4(&planes[5], NormalizePlane(farP));
}

bool cpu_frustum::Intersect(const cpu_sphere& sphere)
{
	XMVECTOR c = XMVectorSet(sphere.center.x, sphere.center.y, sphere.center.z, 1.0f);
	XMVECTOR r = XMVectorReplicate(sphere.radius);
	for ( int i=0 ; i<6 ; ++i )
	{
		XMVECTOR dist = XMVector4Dot(XMLoadFloat4(&planes[i]), c);
		if ( XMVector4Less(dist, XMVectorNegate(r)) )
			return false;
	}
	return true;
}

XMVECTOR XM_CALLCONV cpu_frustum::NormalizePlane(FXMVECTOR p)
{
	// p = (a,b,c,d)
	XMVECTOR n = XMVectorSetW(p, 0.0f);
	XMVECTOR len = XMVector3Length(n);
	return XMVectorDivide(p, len);
}
