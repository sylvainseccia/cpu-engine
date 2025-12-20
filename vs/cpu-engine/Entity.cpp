#include "stdafx.h"

cpu_transform::cpu_transform()
{
	worldUpdated = false;
	invWorldUpdated = false;
	Identity();
}

void cpu_transform::Identity()
{
	pos = CPU_ZERO;
	sca = CPU_ONE;
	ResetRotation();
}

void cpu_transform::UpdateWorld()
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
	worldUpdated = true;
	invWorldUpdated = false;
}

void cpu_transform::UpdateInvWorld()
{
	if ( worldUpdated==false )
		UpdateWorld();

	XMStoreFloat4x4(&invWorld, XMMatrixInverse(nullptr, XMLoadFloat4x4(&GetWorld())));
	invWorldUpdated = true;
}

XMFLOAT4X4& cpu_transform::GetWorld()
{
	if ( worldUpdated==false )
		UpdateWorld();
	return world;
}

XMFLOAT4X4& cpu_transform::GetInvWorld()
{
	if ( invWorldUpdated==false )
		UpdateInvWorld();
	return invWorld;
}

void cpu_transform::SetScaling(float scale)
{
	sca.x = scale;
	sca.y = scale;
	sca.z = scale;
}

void cpu_transform::Scale(float scale)
{
	sca.x *= scale;
	sca.y *= scale;
	sca.z *= scale;
}

void cpu_transform::SetPosition(float x, float y, float z)
{
	pos.x = x;
	pos.y = y;
	pos.z = z;
}

void cpu_transform::Move(float dist)
{
	pos.x += dir.x * dist;
	pos.y += dir.y * dist;
	pos.z += dir.z * dist;
}

void cpu_transform::OrbitAroundAxis(XMFLOAT3& center, XMFLOAT3& axis, float radius, float angle)
{
	XMVECTOR nAxis = XMVector3Normalize(XMLoadFloat3(&axis));
	float d = XMVectorGetX(XMVector3Dot(nAxis, XMUP));
	XMVECTOR ref = fabsf(d)>0.99f ? XMRIGHT : XMUP;
	XMVECTOR radialDir = XMVector3Normalize(XMVector3Cross(nAxis, ref));
	XMVECTOR radial = XMVectorScale(radialDir, radius);
	XMMATRIX r = XMMatrixRotationAxis(nAxis, angle);
	XMVECTOR rotatedRadial = XMVector3TransformNormal(radial, r);
	XMVECTOR position = XMVectorAdd(XMLoadFloat3(&center), rotatedRadial);
	pos.x = XMVectorGetX(position);
	pos.y = XMVectorGetY(position);
	pos.z = XMVectorGetZ(position);
}

void cpu_transform::ResetRotation()
{
	dir = { 0.0f, 0.0f, 1.0f };
	right = { 1.0f, 0.0f, 0.0f };
	up = { 0.0f, 1.0f, 0.0f };
	quat = { 0.0f, 0.0f, 0.0f, 1.0f };
	XMStoreFloat4x4(&rot, XMMatrixIdentity());
}

void cpu_transform::SetRotation(cpu_transform& transform)
{
	dir = transform.dir;
	right = transform.right;
	up = transform.up;
	quat = transform.quat;
	rot = transform.rot;
}

void cpu_transform::SetYPR(float yaw, float pitch, float roll)
{
	ResetRotation();
	AddYPR(yaw, pitch, roll);
}

void cpu_transform::AddYPR(float yaw, float pitch, float roll)
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

void cpu_transform::LookAt(float x, float y, float z)
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

void cpu_transform::LookTo(float ndx, float ndy, float ndz)
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

void cpu_transform::LookTo(XMFLOAT3& ndir)
{
	LookTo(ndir.x, ndir.y, ndir.z);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

cpu_entity::cpu_entity()
{
	index = -1;
	sortedIndex = -1;
	dead = false;
	pMesh = nullptr;
	pMaterial = nullptr;
	lifetime = 0.0f;
	tile = 0;
	radius = 0.0f;
	depth = CPU_DEPTH_READ | CPU_DEPTH_WRITE;
	visible = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

cpu_frustum::cpu_frustum()
{
	for ( int i=0 ; i<6 ; i++ )
		planes[i] = { 0.0f, 0.0f, 0.0f, 0.0f };
}

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

bool cpu_frustum::Intersect(const XMFLOAT3& center, float radius)
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

XMVECTOR XM_CALLCONV cpu_frustum::NormalizePlane(FXMVECTOR p)
{
	// p = (a,b,c,d)
	XMVECTOR n = XMVectorSetW(p, 0.0f);
	XMVECTOR len = XMVector3Length(n);
	return XMVectorDivide(p, len);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

cpu_camera::cpu_camera()
{
	fov = XM_PIDIV4;
	near = 1.0f;
	far = 100.0f;
}

void cpu_camera::UpdateProjection(float aspectRatio)
{
	XMStoreFloat4x4(&matProj, XMMatrixPerspectiveFovLH(fov, aspectRatio, near, far));
}

void cpu_camera::Update()
{
	XMMATRIX mat = XMLoadFloat4x4(&transform.GetInvWorld());
	XMStoreFloat4x4(&matView, mat);
	mat *= XMLoadFloat4x4(&matProj);
	XMStoreFloat4x4(&matViewProj, mat);

	frustum.FromViewProj(matViewProj);
}
