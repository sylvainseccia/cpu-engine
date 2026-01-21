#include "pch.h"

cpu_particle_emitter::cpu_particle_emitter()
{
	index = -1;
	sortedIndex = -1;
	dead = false;

	pData = nullptr;

	density = 1.0f;
	spawnRadius = 0.05f;

	pos = CPU_ZERO;
	dir = CPU_UP;
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

void cpu_particle_emitter::Update(XMFLOAT4X4& matViewProj, int width, int height)
{
	float dt = cpuTime.delta;
	cpu_particle_data& p = *pData;

	XMVECTOR c = XMVectorSet(pos.x, pos.y, pos.z, 1.0f);
	XMVECTOR cClip = XMVector4Transform(c, XMLoadFloat4x4(&matViewProj));
	XMFLOAT4 c4; 
	XMStoreFloat4(&c4, cClip);

	float area_px = 0.0f;
	if ( c4.w>1e-6f )
	{
		const float invWc = 1.0f / c4.w;
		const float ndcCx = c4.x * invWc;
		const float ndcCy = c4.y * invWc;

		XMVECTOR r = XMVectorSet(pos.x+spawnRadius, pos.y, pos.z, 1.0f);
		XMVECTOR rClip = XMVector4Transform(r, XMLoadFloat4x4(&matViewProj));
		XMFLOAT4 r4;
		XMStoreFloat4(&r4, rClip);
		if ( r4.w>1e-6f )
		{
			const float invWr = 1.0f / r4.w;
			const float ndcRx = r4.x * invWr;
			const float ndcRy = r4.y * invWr;

			const float halfW = width * 0.5f;
			const float halfH = height * 0.5f;

			const float pxC = (ndcCx + 1.0f) * halfW;
			const float pyC = (1.0f - ndcCy) * halfH;

			const float pxR = (ndcRx + 1.0f) * halfW;
			const float pyR = (1.0f - ndcRy) * halfH;

			const float dx = pxR - pxC;
			const float dy = pyR - pyC;

			const float r_px = dx * dx + dy * dy;
			area_px = XM_PI * r_px;
			area_px = std::min(area_px, width*height*0.25f);
		}
	}
	if ( area_px<=0.0f )
		return;

	accum += density * area_px * dt;
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

		float rand = cpu::Rand01(seed);
		p.duration[i] = durationMin + (durationMax - durationMin) * rand;
		p.invDuration[i] = 1.0f / p.duration[i];

		float t = cpu::Rand01(seed);
		float intensity = 0.8f + 0.2f * cpu::Rand01(seed);
		p.r[i] = std::lerp(colorMin.x, colorMax.x, t) * intensity;
		p.g[i] = std::lerp(colorMin.y, colorMax.y, t) * intensity;
		p.b[i] = std::lerp(colorMin.z, colorMax.z, t) * intensity;

		//p.seed[i] = seed;
	}
}
