#include "stdafx.h"

TRANSFORM::TRANSFORM()
{
	Identity();
}

void TRANSFORM::Identity()
{
	pos = { 0.0f, 0.0f, 0.0f };
	sca = { 1.0f, 1.0f, 1.0f };
	ResetRotation();
}

void TRANSFORM::Update()
{
	XMVECTOR s = XMLoadFloat3(&sca);
	XMVECTOR p = XMLoadFloat3(&pos);

	XMVECTOR sx = XMVectorSplatX(s);
	XMVECTOR sy = XMVectorSplatY(s);
	XMVECTOR sz = XMVectorSplatZ(s);

	XMMATRIX w = XMLoadFloat4x4(&rot);
	w.r[0] = XMVectorMultiply(w.r[0], sx);
	w.r[1] = XMVectorMultiply(w.r[1], sy);
	w.r[2] = XMVectorMultiply(w.r[2], sz);
	w.r[3] = XMVectorSetW(p, 1.0f);

	XMStoreFloat4x4(&world, w);
}

void TRANSFORM::SetScaling(float scale)
{
	sca.x = scale;
	sca.y = scale;
	sca.z = scale;
}

void TRANSFORM::SetPosition(float x, float y, float z)
{
	pos.x = x;
	pos.y = y;
	pos.z = z;
}

void TRANSFORM::Move(float dist)
{
	pos.x += dir.x * dist;
	pos.y += dir.y * dist;
	pos.z += dir.z * dist;
}

void TRANSFORM::ResetRotation()
{
	dir = { 0.0f, 0.0f, 1.0f };
	right = { 1.0f, 0.0f, 0.0f };
	up = { 0.0f, 1.0f, 0.0f };
	quat = { 0.0f, 0.0f, 0.0f, 1.0f };
	XMStoreFloat4x4(&rot, XMMatrixIdentity());
}

void TRANSFORM::SetRotation(TRANSFORM& transform)
{
	dir = transform.dir;
	right = transform.right;
	up = transform.up;
	quat = transform.quat;
	rot = transform.rot;
}

void TRANSFORM::SetYPR(float yaw, float pitch, float roll)
{
	ResetRotation();
	AddYPR(yaw, pitch, roll);
}

void TRANSFORM::AddYPR(float yaw, float pitch, float roll)
{
	XMVECTOR axisDir = XMLoadFloat3(&dir);
	XMVECTOR axisRight = XMLoadFloat3(&right);
	XMVECTOR axisUp = XMLoadFloat3(&up);

	XMVECTOR qRot = XMLoadFloat4(&quat);
	if ( roll )
		qRot = XMQuaternionMultiply(qRot, XMQuaternionRotationAxis(axisDir, roll));
	if ( pitch )
		qRot = XMQuaternionMultiply(qRot, XMQuaternionRotationAxis(axisRight, pitch));
	if ( yaw )
		qRot = XMQuaternionMultiply(qRot, XMQuaternionRotationAxis(axisUp, yaw));

	qRot = XMQuaternionNormalize(qRot);
	XMStoreFloat4(&quat, qRot);

	XMMATRIX mRot = XMMatrixRotationQuaternion(qRot);
	XMStoreFloat4x4(&rot, mRot);

	right.x = rot._11;
	right.y = rot._12;
	right.z = rot._13;
	up.x = rot._21;
	up.y = rot._22;
	up.z = rot._23;
	dir.x = rot._31;
	dir.y = rot._32;
	dir.z = rot._33;
}

void TRANSFORM::LookAt(float x, float y, float z)
{
	XMVECTOR vA = XMLoadFloat3(&pos);
	XMVECTOR vB = XMVectorSet(x, y, z, 0.0f);
	XMVECTOR vDir = XMVectorSubtract(vB, vA);
	const XMMATRIX cam = XMMatrixTranspose(XMMatrixLookAtLH(XMVectorZero(), vDir, XMUP));
	XMStoreFloat4x4(&rot, cam);

	XMStoreFloat4(&quat, XMQuaternionNormalize(XMQuaternionRotationMatrix(cam)));

	right.x = rot._11;
	right.y = rot._12;
	right.z = rot._13;
	up.x = rot._21;
	up.y = rot._22;
	up.z = rot._23;
	dir.x = rot._31;
	dir.y = rot._32;
	dir.z = rot._33;
}

void TRANSFORM::LookTo(float ndx, float ndy, float ndz)
{
	XMVECTOR vDir = XMVectorSet(ndx, ndy, ndz, 0.0f);
	XMMATRIX cam = XMMatrixTranspose(XMMatrixLookToLH(XMVectorZero(), vDir, XMUP));
	XMStoreFloat4x4(&rot, cam);

	XMStoreFloat4(&quat, XMQuaternionNormalize(XMQuaternionRotationMatrix(cam)));

	right.x = rot._11;
	right.y = rot._12;
	right.z = rot._13;
	up.x = rot._21;
	up.y = rot._22;
	up.z = rot._23;
	dir.x = rot._31;
	dir.y = rot._32;
	dir.z = rot._33;
}

void TRANSFORM::LookTo(XMFLOAT3& ndir)
{
	LookTo(ndir.x, ndir.y, ndir.z);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ENTITY::ENTITY()
{
	index = -1;
	sortedIndex = -1;
	dead = false;
	pMesh = nullptr;
	pMaterial = nullptr;
	lifetime = 0.0f;
	tile = 0;
	radius = 0.0f;
	depth = DEPTH_READ | DEPTH_WRITE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

FRUSTUM::FRUSTUM()
{
	for ( int i=0 ; i<6 ; i++ )
		planes[i] = { 0.0f, 0.0f, 0.0f, 0.0f };
}

void FRUSTUM::FromViewProj(const XMFLOAT4X4& viewProj)
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

bool FRUSTUM::Intersect(const XMFLOAT3& center, float radius)
{
	XMVECTOR c = XMVectorSet(center.x, center.y, center.z, 1.0f);
	XMVECTOR r = XMVectorReplicate(radius);
	for ( int i=0 ; i<6 ; ++i )
	{
		XMVECTOR dist = XMVector4Dot(XMLoadFloat4(&planes[i]), c);
		if ( XMVector4Less(dist, XMVectorNegate(r)) )
			return false;
	}
	return true;
}

XMVECTOR XM_CALLCONV FRUSTUM::NormalizePlane(FXMVECTOR p)
{
	// p = (a,b,c,d)
	XMVECTOR n = XMVectorSetW(p, 0.0f);
	XMVECTOR len = XMVector3Length(n);
	return XMVectorDivide(p, len);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CAMERA::CAMERA()
{
	fov = XM_PIDIV4;
	near = 1.0f;
	far = 100.0f;
}

void CAMERA::UpdateProjection(float aspectRatio)
{
	XMStoreFloat4x4(&matProj, XMMatrixPerspectiveFovLH(fov, aspectRatio, near, far));
}

void CAMERA::Update()
{
	transform.Update();

	XMMATRIX mat = XMMatrixInverse(nullptr, XMLoadFloat4x4(&transform.world));
	XMStoreFloat4x4(&matView, mat);
	mat *= XMLoadFloat4x4(&matProj);
	XMStoreFloat4x4(&matViewProj, mat);

	frustum.FromViewProj(matViewProj);
}
