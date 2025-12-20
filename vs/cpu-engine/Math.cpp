#include "stdafx.h"

namespace cpu_math
{

bool SphereSphere(XMFLOAT3& c1, float r1, XMFLOAT3& c2, float r2)
{
	const float dx = c2.x - c1.x;
	const float dy = c2.y - c1.y;
	const float dz = c2.z - c1.z;
	const float distSq = dx*dx + dy*dy + dz*dz;
	const float r = r1 + r2;
	return distSq <= r*r;
}

bool RayAabb(cpu_ray& ray, cpu_aabb& box, XMFLOAT3* pOutHit, float* pOutT)
{
	// Si origine dans la box : premier point = origine (t=0)
	if ( box.Contains(ray.pos) )
	{
		if ( pOutHit )
			*pOutHit = ray.pos;
		if ( pOutT )
			*pOutT = 0.0f;
		return true;
	}

	// Slabs : on calcule l'intervalle [tmin, tmax] de recouvrement sur les 3 axes.
	float tmin = 0.0f;
	float tmax = FLT_MAX;

	auto slab = [&](float ro, float rd, float bmin, float bmax) -> bool
	{
		// Rayon parallèle à l’axe -> doit être dans le slab pour intersecter
		const float eps = 1e-12f;
		if ( fabsf(rd)<eps )
			return ro>=bmin && ro<=bmax;

		float inv = 1.0f / rd;
		float t1 = (bmin - ro) * inv;
		float t2 = (bmax - ro) * inv;
		if ( t1>t2 )
			std::swap(t1, t2);

		// Intersection d'intervalles
		tmin = t1>tmin ? t1 : tmin;
		tmax = t2<tmax ? t2 : tmax;
		return tmax>=tmin;
	};

	if ( slab(ray.pos.x, ray.dir.x, box.min.x, box.max.x)==false )
		return false;
	if ( slab(ray.pos.y, ray.dir.y, box.min.y, box.max.y)==false )
		return false;
	if ( slab(ray.pos.z, ray.dir.z, box.min.z, box.max.z)==false )
		return false;

	if ( pOutHit==nullptr && pOutT==nullptr )
		return true;

	// On veut le premier point avec t >= 0
	// Si tmin < 0, ça veut dire "entrée derrière", mais comme on a exclu le cas inside,
	// c'est rare; on clamp à 0 pour "premier contact forward".
	float t = tmin>=0.0f ? tmin : 0.0f;

	if ( pOutHit )
	{
		pOutHit->x = ray.pos.x + ray.dir.x * t;
		pOutHit->y = ray.pos.y + ray.dir.y * t;
		pOutHit->z = ray.pos.z + ray.dir.z * t;
	}

	if ( pOutT )
		*pOutT = t;
	return true;
}

// Retourne true si le rayon intersecte l'AABB.
// outTEnter = premier t d'entrée (>=0 si l'origine est dehors; 0 si l'origine est dedans)
// outTExit  = (optionnel) t de sortie.
bool RayAabb(cpu_ray& ray, cpu_aabb& box, float& outTEnter, float& outTExit)
{
	float tmin = 0.0f;
	float tmax = FLT_MAX;

	auto slab = [&](float ro, float rd, float bmin, float bmax) -> bool
	{
		const float eps = 1e-12f;
		if ( fabsf(rd)<eps )
		{
			// Parallèle à cet axe : il faut être dans le slab
			return ro>=bmin && ro<=bmax;
		}

		const float inv = 1.0f / rd;
		float t1 = (bmin - ro) * inv;
		float t2 = (bmax - ro) * inv;
		if ( t1>t2 )
			std::swap(t1, t2);

		tmin = t1>tmin ? t1 : tmin;
		tmax = t2<tmax ? t2 : tmax;

		return tmax>=tmin;
	};

	if ( slab(ray.pos.x, ray.dir.x, box.min.x, box.max.x)==false )
		return false;
	if ( slab(ray.pos.y, ray.dir.y, box.min.y, box.max.y)==false )
		return false;
	if ( slab(ray.pos.z, ray.dir.z, box.min.z, box.max.z)==false )
		return false;

	// Si la sortie est derrière, pas de hit "forward"
	if ( tmax<0.0f )
		return false;

	const float tEnter = tmin>=0.0f ? tmin : 0.0f;
	outTEnter = tEnter;
	outTExit = tmax;
	return true;
}

bool RaySphere(cpu_ray& ray, XMFLOAT3& center, float radius, XMFLOAT3& outHit, float* outT)
{
	// On résout ||(ro + t rd) - C||^2 = r^2
	// a t^2 + 2b t + c = 0, avec:
	// a = dot(rd, rd)
	// b = dot(oc, rd) où oc = ro - C
	// c = dot(oc, oc) - r^2
	const XMFLOAT3 oc = Sub3(ray.pos, center);

	const float a = Dot3(ray.dir, ray.dir);
	const float b = Dot3(oc, ray.dir);
	const float c = Dot3(oc, oc) - radius * radius;

	// Direction nulle / dégénérée
	const float eps = 1e-12f;
	if ( a<=eps )
		return false;

	// Discriminant: (2b)^2 - 4ac = 4(b^2 - a c)
	const float disc = b*b - a*c;
	if ( disc<0.0f )
		return false;

	const float sqrtDisc = sqrtf(disc);

	// Racines: t = (-b ± sqrtDisc) / a
	// On veut le plus petit t >= 0
	float t0 = (-b - sqrtDisc) / a;
	float t1 = (-b + sqrtDisc) / a;

	// Trier t0 <= t1
	if ( t0>t1 )
	{
		float tmp = t0;
		t0 = t1;
		t1 = tmp;
	}

	float t = -1.0f;

	if ( t0>=0.0f )
	{
		t = t0; // entrée
	}
	else if (t1 >= 0.0f)
	{
		// Le rayon démarre dans la sphère ou l'entrée est derrière :
		// si tu veux "premier point forward" strict, tu peux choisir t1.
		// MAIS tu as demandé "point le plus proche" : c'est l'origine (t=0).
		t = 0.0f;
	}
	else
	{
		return false; // les deux intersections sont derrière
	}

	outHit = Add3(ray.pos, Mul3(ray.dir, t));
	if ( outT )
		*outT = t;
	return true;
}

// Retourne true si intersection.
// outHit = point d’intersection (premier point rencontré pour t >= 0).
// outT = paramètre t (optionnel).
// outBary = barycentriques (u,v,w) optionnel, utile pour interpoler (normal, uv, etc.).
bool RayTriangle(cpu_ray& ray, cpu_triangle& tri, XMFLOAT3& outHit, float* outT , XMFLOAT3* outBary, bool cullBackFace)
{
	const XMFLOAT3& v0 = tri.v[0].pos;
	const XMFLOAT3& v1 = tri.v[1].pos;
	const XMFLOAT3& v2 = tri.v[2].pos;

	// Edges
	const XMFLOAT3 e1 = Sub3(v1, v0);
	const XMFLOAT3 e2 = Sub3(v2, v0);

	// Begin calculating determinant
	const XMFLOAT3 pvec = Cross3(ray.dir, e2);
	const float det = Dot3(e1, pvec);

	// Culling / parallel check
	const float eps = 1e-8f;

	if ( cullBackFace )
	{
		if ( det<=eps )
			return false; // backface ou parallèle
		const XMFLOAT3 tvec = Sub3(ray.pos, v0);
		const float u = Dot3(tvec, pvec);
		if ( u<0.0f || u>det )
			return false;

		const XMFLOAT3 qvec = Cross3(tvec, e1);
		const float v = Dot3(ray.dir, qvec);
		if ( v<0.0f || (u+v)>det )
			return false;

		const float t = Dot3(e2, qvec) / det;
		if ( t<0.0f )
			return false;

		outHit = Add3(ray.pos, Mul3(ray.dir, t));
		if ( outT )
			*outT = t;
		if ( outBary )
		{
			// u' = u/det, v' = v/det, w = 1-u'-v'
			const float invDet = 1.0f / det;
			const float uu = u * invDet;
			const float vv = v * invDet;
			outBary->x = uu;
			outBary->y = vv;
			outBary->z = 1.0f - uu - vv;
		}
		return true;
	}
	else
	{
		if ( std::fabs(det)<=eps )
			return false; // parallèle
		const float invDet = 1.0f / det;

		const XMFLOAT3 tvec = Sub3(ray.pos, v0);
		const float u = Dot3(tvec, pvec) * invDet;
		if ( u<0.0f || u>1.0f )
			return false;

		const XMFLOAT3 qvec = Cross3(tvec, e1);
		const float v = Dot3(ray.dir, qvec) * invDet;
		if ( v<0.0f || (u+v)>1.0f )
			return false;

		const float t = Dot3(e2, qvec) * invDet;
		if ( t<0.0f )
			return false;

		outHit = Add3(ray.pos, Mul3(ray.dir, t));
		if ( outT )
			*outT = t;
		if ( outBary )
		{
			outBary->x = u;
			outBary->y = v;
			outBary->z = 1.0f - u - v;
		}
		return true;
	}
}

bool AabbAabb(cpu_aabb& a, cpu_aabb& b)
{
	return a.min.x<b.max.x && a.max.x>b.min.x && a.min.y<b.max.y && a.max.y>b.min.y && a.min.z<b.max.z && a.max.z>b.min.z;
}

bool AabbAabbInclusive(cpu_aabb& a, cpu_aabb& b)
{
	return a.min.x<=b.max.x && a.max.x>=b.min.x && a.min.y<=b.max.y && a.max.y>=b.min.y && a.min.z<=b.max.z && a.max.z>=b.min.z;
}

}
