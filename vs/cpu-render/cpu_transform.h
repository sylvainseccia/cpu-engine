#pragma once

struct cpu_transform
{
public:
	// Position
	XMFLOAT3 pos;

	// Scaling
	XMFLOAT3 sca;

	// Rotation
	XMFLOAT3 dir;
	XMFLOAT3 right;
	XMFLOAT3 up;
	XMFLOAT4 quat;
	XMFLOAT4X4 rot;

private:
	// World
	bool worldUpdated;
	XMFLOAT4X4 world;
	bool invWorldUpdated;
	XMFLOAT4X4 invWorld;
	
public:
	cpu_transform();

	void Identity();
	void UpdateWorld();
	void UpdateInvWorld();
	void ResetFlags() { worldUpdated = invWorldUpdated = false; }
	XMFLOAT4X4& GetWorld();
	XMFLOAT4X4& GetInvWorld();
	void SetScaling(float scale);
	void Scale(float scale);
	void SetPosition(float x, float y, float z);
	void Move(float dist);
	void OrbitAroundAxis(XMFLOAT3& center, XMFLOAT3& axis, float radius, float angle);
	void ResetRotation();
	void SetRotation(cpu_transform& transform);
	void SetRotationFromAxes();
	void SetRotationFromMatrix();
	void SetRotationFromQuaternion();
	void SetYPR(float yaw, float pitch = 0.0f, float roll = 0.0f);
	void AddYPR(float yaw, float pitch = 0.0f, float roll = 0.0f);
	void LookAt(float x, float y, float z);
	void LookTo(float ndx, float ndy, float ndz);
	void LookTo(XMFLOAT3& ndir);
};
