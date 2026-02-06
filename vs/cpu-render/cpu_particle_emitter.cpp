#include "pch.h"

cpu_particle_emitter::cpu_particle_emitter()
{
	index = -1;
	sortedIndex = -1;
	dead = false;

	pData = nullptr;

	blend = CPU_PARTICLE_INTENSITY;

	rate = 1.0f;
	spawnRadius = 0.05f;

	pos = CPU_VEC3_ZERO;
	dir = CPU_VEC3_UP;
	colorMin = { 1.0f, 1.0f, 1.0f };
	colorMax = { 1.0f, 1.0f, 1.0f };
	durationMin = 0.5f;
	durationMax = 3.0f;
	speedMin = 0.7f;
	speedMax = 1.3f;
	spread = 0.5f;

	accum = 0.0f;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void cpu_particle_emitter::Update(int pixelCount)
{
	float dt = cpuTime.delta;
	cpu_particle_data& p = *pData;

	accum += rate * (float)pixelCount * dt;
	int n = (int)accum;
	accum -= (float)n;
	//n = MIN(n, maxEmitPerFrame);

	while ( n-->0 && p.alive<p.maxCount )
	{
		const int i = p.alive++;

		ui32 seed = (ui32)(i * 9781u + 0x9E3779B9u);

		p.px[i] = pos.x + cpu::RandSigned(seed) * spawnRadius;
		p.py[i] = pos.y + cpu::RandSigned(seed) * spawnRadius;
		p.pz[i] = pos.z + cpu::RandSigned(seed) * spawnRadius;

		float speed = speedMin + (speedMax - speedMin) * cpu::Rand01(seed);
		p.vx[i] = (dir.x + cpu::RandSigned(seed)*spread) * speed;
		p.vy[i] = (dir.y + cpu::RandSigned(seed)*spread) * speed;
		p.vz[i] = (dir.z + cpu::RandSigned(seed)*spread) * speed;

		p.age[i] = 0.0f;

		float rndDuration = cpu::Rand01(seed);
		p.duration[i] = durationMin + (durationMax - durationMin) * rndDuration;
		p.invDuration[i] = 1.0f / p.duration[i];

		float rndRatio = cpu::Rand01(seed);
		float rndIntensity = 0.8f + 0.2f * cpu::Rand01(seed);
		p.r[i] = std::lerp(colorMin.x, colorMax.x, rndRatio) * rndIntensity;
		p.g[i] = std::lerp(colorMin.y, colorMax.y, rndRatio) * rndIntensity;
		p.b[i] = std::lerp(colorMin.z, colorMax.z, rndRatio) * rndIntensity;

		p.blend[i] = blend;
	}
}
