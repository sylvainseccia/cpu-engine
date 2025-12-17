#include "stdafx.h"

cpu_particle_physics::cpu_particle_physics()
{
	gx = 0.0f;
	gy = -1.0f;
	gz = 0.0f;
	drag = 0.0f;
	maxSpeed = 0.0f;
	minX = 0.0f;
	minY = 0.0f;
	minZ = 0.0f;
	maxX = 0.0f;
	maxY = 0.0f;
	maxZ = 0.0f;
	useBounds = false;
	bounce = 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

cpu_particle_data::cpu_particle_data()
{
	Reset();
}

cpu_particle_data::~cpu_particle_data()
{
	Destroy();
}

void cpu_particle_data::Reset()
{
	maxCount = 0;
	alive = 0;
	blob = nullptr;
	size = 0;
	px = nullptr;
	py = nullptr;
	pz = nullptr;
	vx = nullptr;
	vy = nullptr;
	vz = nullptr;
	age = nullptr;
	duration = nullptr;
	seed = nullptr;
	r = nullptr;
	g = nullptr;
	b = nullptr;
}

void cpu_particle_data::Create(int maxP)
{
	Destroy();
	maxCount = maxP;

	int countF = maxP * sizeof(float);
	int countU = maxP * sizeof(ui32);
	size	= 3 * countF   // px py pz
			+ 3 * countF   // vx vy vz
			+ 3 * countF   // age duration invDuration
			+ 1 * countU   // seed
			+ 3 * countF;  // r g b


	blob = _aligned_malloc(size, 32); // SIMD: 32 or 64
	byte* ptr = (byte*)blob;

	px = (float*)ptr; ptr += countF;
	py = (float*)ptr; ptr += countF;
	pz = (float*)ptr; ptr += countF;

	vx = (float*)ptr; ptr += countF;
	vy = (float*)ptr; ptr += countF;
	vz = (float*)ptr; ptr += countF;

	age = (float*)ptr; ptr += countF;
	duration = (float*)ptr; ptr += countF;
	invDuration = (float*)ptr; ptr += countF;

	seed = (ui32*)ptr; ptr += countU;

	r = (float*)ptr; ptr += countF;
	g = (float*)ptr; ptr += countF;
	b = (float*)ptr; ptr += countF;
}

void cpu_particle_data::Destroy()
{
	if ( blob )
		_aligned_free(blob);
	Reset();
}

void cpu_particle_data::Update(float dt, const cpu_particle_physics& phys)
{
	if ( alive<=0 )
		return;

	const float drag = phys.drag> 0.0f ? phys.drag : 0.0f;
	const float dragFactor = drag>0.0f ? 1.0f/(1.0f+drag*dt) : 1.0f;
	const float maxSpeed = phys.maxSpeed;

	int i = 0;
	while ( i<alive )
	{
		// Age / kill
		age[i] += dt;
		if ( age[i]>=duration[i] )
		{
			const int last = alive - 1;
			px[i] = px[last];
			py[i] = py[last];
			pz[i] = pz[last];
			vx[i] = vx[last];
			vy[i] = vy[last];
			vz[i] = vz[last];
			age[i] = age[last];
			duration[i] = duration[last];
			invDuration[i] = invDuration[last];
			r[i] = r[last];
			g[i] = g[last];
			b[i] = b[last];
			seed[i] = seed[last];
			--alive;
			continue;
		}
		
		// Dissipe l'énergie
		float x = vx[i] * dragFactor;
		float y = vy[i] * dragFactor;
		float z = vz[i] * dragFactor;

		// Gravity
		x += phys.gx * dt;
		y += phys.gy * dt;
		z += phys.gz * dt;

		// Speed
		if ( maxSpeed>0.0f )
		{
			const float v2 = x*x + y*y + z*z;
			const float m2 = maxSpeed * maxSpeed;
			if ( v2>m2 )
			{
				const float invLen = maxSpeed / sqrtf(v2);
				x *= invLen;
				y *= invLen;
				z *= invLen;
			}
		}

		// Euler (semi-implicite)
		px[i] += x * dt;
		py[i] += y * dt;
		pz[i] += z * dt;
		vx[i] = x;
		vy[i] = y;
		vz[i] = z;

		// Bound
		if ( phys.useBounds )
			ApplyBounds(px[i], py[i], pz[i], vx[i], vy[i], vz[i], phys);

		++i;
	}
}

void cpu_particle_data::ApplyBounds(float& px, float& py, float& pz, float& vx, float& vy, float& vz, const cpu_particle_physics& phys)
{
	if ( px<phys.minX )
	{
		px = phys.minX;
		vx = -vx * phys.bounce;
	}
	if ( px>phys.maxX )
	{
		px = phys.maxX;
		vx = -vx * phys.bounce;
	}
	if ( py<phys.minY )
	{
		py = phys.minY;
		vy = -vy * phys.bounce;
	}
	if ( py>phys.maxY )
	{
		py = phys.maxY;
		vy = -vy * phys.bounce;
	}
	if ( pz<phys.minZ )
	{
		pz = phys.minZ;
		vz = -vz * phys.bounce;
	}
	if ( pz>phys.maxZ )
	{
		pz = phys.maxZ;
		vz = -vz * phys.bounce;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

cpu_particle_emitter::cpu_particle_emitter()
{
	index = -1;
	sortedIndex = -1;
	dead = false;

	rate = 100.0f;
	durationMin = 0.5f;
	durationMax = 3.0f;
	pos = ZERO;
	dir = UP;
	colorMin = { 1.0f, 1.0f, 1.0f };
	colorMax = { 1.0f, 1.0f, 1.0f };
	spread = 0.5f;
	speedMin = 0.7f;
	speedMax = 1.3f;
	spawnRadius = 0.05f;

	accum = 0.0f;
};

void cpu_particle_emitter::Update(cpu_particle_data& p, float dt)
{
	// TODO: exposer les variables

	accum += rate * dt;
	int n = (int)accum;
	accum -= (float)n;

	while ( n-->0 && p.alive<p.maxCount )
	{
		const int i = p.alive++;

		ui32 seed = (ui32)(i * 9781u + 0x9E3779B9u);

		float rx = RandSigned(seed);
		float ry = RandSigned(seed);
		float rz = RandSigned(seed);
		p.px[i] = pos.x + rx * spawnRadius;
		p.py[i] = pos.y + ry * spawnRadius;
		p.pz[i] = pos.z + rz * spawnRadius;

		float dvx = RandSigned(seed) * spread;
		float dvy = RandSigned(seed) * spread;
		float dvz = RandSigned(seed) * spread;

		float speed = speedMin + (speedMax - speedMin) * Rand01(seed);
		p.vx[i] = (dir.x + dvx) * speed;
		p.vy[i] = (dir.y + dvy) * speed;
		p.vy[i] = (dir.y + dvy) * speed;
		p.vz[i] = (dir.z + dvz) * speed;

		p.age[i] = 0.0f;

		float rand = Rand01(seed);
		p.duration[i] = durationMin + (durationMax - durationMin) * rand;
		p.invDuration[i] = 1.0f / p.duration[i];

		float t = Rand01(seed);
		float intensity = 0.8f + 0.2f * Rand01(seed);
		p.r[i] = std::lerp(colorMin.x, colorMax.x, t) * intensity;
		p.g[i] = std::lerp(colorMin.y, colorMax.y, t) * intensity;
		p.b[i] = std::lerp(colorMin.z, colorMax.z, t) * intensity;

		p.seed[i] = seed;
	}
}
