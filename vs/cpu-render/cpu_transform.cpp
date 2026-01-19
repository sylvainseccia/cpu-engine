#include "pch.h"

cpu_transform::cpu_transform()
{
	worldUpdated = false;
	invWorldUpdated = false;
	Identity();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
	float d = XMVectorGetX(XMVector3Dot(nAxis, CPU_XMUP));
	XMVECTOR ref = fabsf(d)>0.99f ? CPU_XMRIGHT : CPU_XMUP;
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

void cpu_transform::SetRotationFromAxes()
{
	rot._11 = right.x;
	rot._12 = right.y;
	rot._13 = right.z;
	rot._14 = 0.0f;
	rot._21 = up.x;
	rot._22 = up.y;
	rot._23 = up.z;
	rot._24 = 0.0f;
	rot._31 = dir.x;
	rot._32 = dir.y;
	rot._33 = dir.z;
	rot._34 = 0.0f;
	rot._41 = 0.0f;
	rot._42 = 0.0f;
	rot._43 = 0.0f;
	rot._44 = 1.0f;
	XMStoreFloat4(&quat, XMQuaternionNormalize(XMQuaternionRotationMatrix(XMLoadFloat4x4(&rot))));
}

void cpu_transform::SetRotationFromMatrix()
{
	XMStoreFloat4(&quat, XMQuaternionNormalize(XMQuaternionRotationMatrix(XMLoadFloat4x4(&rot))));
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

void cpu_transform::SetRotationFromQuaternion()
{
	XMStoreFloat4x4(&rot, XMMatrixRotationQuaternion(XMLoadFloat4(&quat)));
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

	SetRotationFromQuaternion();
}

void cpu_transform::LookAt(float x, float y, float z)
{
	XMVECTOR vA = XMLoadFloat3(&pos);
	XMVECTOR vB = XMVectorSet(x, y, z, 0.0f);
	XMVECTOR vDir = XMVectorSubtract(vB, vA);
	const XMMATRIX cam = XMMatrixTranspose(XMMatrixLookAtLH(XMVectorZero(), vDir, CPU_XMUP));
	XMStoreFloat4x4(&rot, cam);
	SetRotationFromMatrix();
}

void cpu_transform::LookTo(float ndx, float ndy, float ndz)
{
	XMVECTOR vDir = XMVectorSet(ndx, ndy, ndz, 0.0f);
	XMMATRIX cam = XMMatrixTranspose(XMMatrixLookToLH(XMVectorZero(), vDir, CPU_XMUP));
	XMStoreFloat4x4(&rot, cam);
	SetRotationFromMatrix();
}

void cpu_transform::LookTo(XMFLOAT3& ndir)
{
	LookTo(ndir.x, ndir.y, ndir.z);
}
