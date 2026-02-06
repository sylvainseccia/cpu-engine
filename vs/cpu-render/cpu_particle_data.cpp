#include "pch.h"

cpu_particle_data::cpu_particle_data()
{
	pPhysics = nullptr;
	Reset();
}

cpu_particle_data::~cpu_particle_data()
{
	Destroy();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
	r = nullptr;
	g = nullptr;
	b = nullptr;
	blend = nullptr;
	tile = nullptr;
	sort = nullptr;
	sx = nullptr;
	sy = nullptr;
	sz = nullptr;
}

void cpu_particle_data::Create(int maxP)
{
	Destroy();
	maxCount = maxP;

	int count8 = maxP;
	int count16 = maxP * 2;
	int count32 = maxP * 4;
	size	= 3 * count32		// px py pz
			+ 3 * count32		// vx vy vz
			+ 3 * count32		// age duration invDuration
			+ 3 * count32		// r g b
			+ 1 * count8		// blend
			+ 1 * count8		// tile
			+ 1 * count32		// sort
			+ 2 * count32		// sx sy
			+ 1 * count32;		// sz

	blob = _aligned_malloc(size, 32); // SIMD: 32 or 64
	byte* ptr = (byte*)blob;

	px = (float*)ptr; ptr += count32;
	py = (float*)ptr; ptr += count32;
	pz = (float*)ptr; ptr += count32;

	vx = (float*)ptr; ptr += count32;
	vy = (float*)ptr; ptr += count32;
	vz = (float*)ptr; ptr += count32;

	age = (float*)ptr; ptr += count32;
	duration = (float*)ptr; ptr += count32;
	invDuration = (float*)ptr; ptr += count32;

	r = (float*)ptr; ptr += count32;
	g = (float*)ptr; ptr += count32;
	b = (float*)ptr; ptr += count32;
	blend = (byte*)ptr; ptr += count8;

	tile = (byte*)ptr; ptr += count8;
	sort = (ui32*)ptr; ptr += count32;
	sx = (ui16*)ptr; ptr += count32;
	sy = (ui16*)ptr; ptr += count32;
	sz = (float*)ptr; ptr += count32;
}

void cpu_particle_data::Destroy()
{
	if ( blob )
		_aligned_free(blob);
	Reset();
}

void cpu_particle_data::UpdateAge()
{
	if ( alive<=0 )
		return;

	const float dt = cpuTime.delta;
	int i = 0;
	while ( i<alive )
	{
		age[i] += dt;
		if ( age[i]>=duration[i] )
		{
			const int last = alive - 1;
			if ( i<last )
			{
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
				blend[i] = blend[last];
				tile[i] = tile[last];
				sort[i] = sort[last];
				sx[i] = sx[last];
				sy[i] = sy[last];
				sz[i] = sz[last];
			}
			--alive;
			continue;
		}
		++i;
	}
}

void cpu_particle_data::UpdatePhysics(int min, int max)
{
	const float dt = cpuTime.delta;
	const float drag = pPhysics->drag>0.0f ? pPhysics->drag : 0.0f;
	const float dragFactor = drag>0.0f ? 1.0f/(1.0f+drag*dt) : 1.0f;
	const float maxSpeed = pPhysics->maxSpeed;

	for ( int i=min ; i<max ; i++ )
	{
		// Dissipe l'énergie
		float x = vx[i] * dragFactor;
		float y = vy[i] * dragFactor;
		float z = vz[i] * dragFactor;

		// Gravity
		x += pPhysics->gx * dt;
		y += pPhysics->gy * dt;
		z += pPhysics->gz * dt;

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

		// Euler (semi-implicit)
		px[i] += x * dt;
		py[i] += y * dt;
		pz[i] += z * dt;
		vx[i] = x;
		vy[i] = y;
		vz[i] = z;

		// Bound
		if ( pPhysics->useBounds )
			ApplyBounds(px[i], py[i], pz[i], vx[i], vy[i], vz[i]);
	}
}

void cpu_particle_data::ApplyBounds(float& px, float& py, float& pz, float& vx, float& vy, float& vz)
{
	if ( px<pPhysics->minX )
	{
		px = pPhysics->minX;
		vx = -vx * pPhysics->bounce;
	}
	if ( px>pPhysics->maxX )
	{
		px = pPhysics->maxX;
		vx = -vx * pPhysics->bounce;
	}
	if ( py<pPhysics->minY )
	{
		py = pPhysics->minY;
		vy = -vy * pPhysics->bounce;
	}
	if ( py>pPhysics->maxY )
	{
		py = pPhysics->maxY;
		vy = -vy * pPhysics->bounce;
	}
	if ( pz<pPhysics->minZ )
	{
		pz = pPhysics->minZ;
		vz = -vz * pPhysics->bounce;
	}
	if ( pz>pPhysics->maxZ )
	{
		pz = pPhysics->maxZ;
		vz = -vz * pPhysics->bounce;
	}
}
