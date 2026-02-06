#pragma once

// AoS (Array of Structures) vs SoA (Structure of Arrays)

struct cpu_particle_data
{
public:
	cpu_particle_physics* pPhysics;

	int maxCount;
	int alive;

	void* blob;
	int size;

	float* px;
	float* py;
	float* pz;

	float* vx;
	float* vy;
	float* vz;

	float* age;
	float* duration;
	float* invDuration;

	float* r;
	float* g;
	float* b;
	byte* blend;

	// Render
	byte* tile;
	ui32* sort;
	ui16* sx;
	ui16* sy;
	float* sz;

public:
	cpu_particle_data();
	~cpu_particle_data();

	void Reset();

	void Create(int maxP);
	void Destroy();
	void UpdateAge();
	void UpdatePhysics(int min, int max);
	inline void ApplyBounds(float& px, float& py, float& pz, float& vx, float& vy, float& vz);
};
