#include "pch.h"

namespace cpu
{

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Initialize()
{
	cpu_window::Initialize();
}

void Uninitialize()
{
	cpu_window::Uninitialize();
	cpu_img32::Free();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ui32 SwapRB(ui32 color)
{
	return (color & 0xFF00FF00u) | ((color & 0x000000FFu) << 16) | ((color & 0x00FF0000u) >> 16);
}

ui32 ToRGB(XMFLOAT3& color)
{
	return CPU_RGBX(Clamp((int)(color.x*255.0f), 0, 255), Clamp((int)(color.y*255.0f), 0, 255), Clamp((int)(color.z*255.0f), 0, 255));
}

ui32 ToRGB(float r, float g, float b)
{
	return CPU_RGBX(Clamp((int)(r*255.0f), 0, 255), Clamp((int)(g*255.0f), 0, 255), Clamp((int)(b*255.0f), 0, 255));
}

ui32 ToRGB(int r, int g, int b)
{
	return CPU_RGBX(Clamp(r, 0, 255), Clamp(g, 0, 255), Clamp(b, 0, 255));
}

ui32 ToBGR(XMFLOAT3& color)
{
	return CPU_RGBX(Clamp((int)(color.z*255.0f), 0, 255), Clamp((int)(color.y*255.0f), 0, 255), Clamp((int)(color.x*255.0f), 0, 255));
}

ui32 ToBGR(float r, float g, float b)
{
	return CPU_RGBX(Clamp((int)(b*255.0f), 0, 255), Clamp((int)(g*255.0f), 0, 255), Clamp((int)(r*255.0f), 0, 255));
}

XMFLOAT3 ToColor(int r, int g, int b)
{
	XMFLOAT3 color;
	color.x = Clamp(r, 0, 255)/255.0f;
	color.y = Clamp(g, 0, 255)/255.0f;
	color.z = Clamp(b, 0, 255)/255.0f;
	return color;
}

XMFLOAT3 ToColorFromRGB(ui32 rgb)
{
	XMFLOAT3 color;
	color.x = (rgb & 0xFF)/255.0f;
	color.y = ((rgb>>8) & 0xFF)/255.0f;
	color.z = ((rgb>>16) & 0xFF)/255.0f;
	return color;
}

XMFLOAT3 ToColorFromBGR(ui32 bgr)
{
	XMFLOAT3 color;
	color.x = ((bgr>>16) & 0xFF)/255.0f;
	color.y = ((bgr>>8) & 0xFF)/255.0f;
	color.z = (bgr & 0xFF)/255.0f;
	return color;
}

float Lerp(float a, float b, float s)
{
	return a + (b-a) * s;
}

void Lerp(float& out, float a, float b, float s)
{
	out = a + (b-a) * s;
}

void Lerp(XMFLOAT2& out, const XMFLOAT2& a, const XMFLOAT2& b, float t)
{
	out.x = Lerp(a.x, b.x, t);
	out.y = Lerp(a.y, b.y, t);
}

void Lerp(XMFLOAT3& out, const XMFLOAT3& a, const XMFLOAT3& b, float t)
{
	out.x = Lerp(a.x, b.x, t);
	out.y = Lerp(a.y, b.y, t);
	out.z = Lerp(a.z, b.z, t);
}

void Lerp(XMFLOAT4& out, const XMFLOAT4& a, const XMFLOAT4& b, float t)
{
	out.x = Lerp(a.x, b.x, t);
	out.y = Lerp(a.y, b.y, t);
	out.z = Lerp(a.z, b.z, t);
	out.w = Lerp(a.w, b.w, t);
}

ui32 LerpColor(ui32 c0, ui32 c1, float t)
{
	int r0 = (int)( c0        & 255);
	int g0 = (int)((c0 >>  8) & 255);
	int b0 = (int)((c0 >> 16) & 255);
	int r1 = (int)( c1        & 255);
	int g1 = (int)((c1 >>  8) & 255);
	int b1 = (int)((c1 >> 16) & 255);
	float it = 1.0f - t;
	int r = (int)(r0 * it + r1 * t);
	int g = (int)(g0 * it + g1 * t);
	int b = (int)(b0 * it + b1 * t);
	return (ui32)r | ((ui32)g << 8) | ((ui32)b << 16) | 0xFF000000;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

float Clamp(float v)
{
	if ( v<0.0f )
		return 0.0f;
	if ( v>1.0f )
		return 1.0f;
	return v;
}

float Clamp(float v, float min, float max)
{
	if ( v<min )
		return min;
	if ( v>max )
		return max;
	return v;
}

int Clamp(int v, int min, int max)
{
	if ( v<min )
		return min;
	if ( v>max )
		return max;
	return v;
}

int FloorToInt(float v)
{
	int i = (int)v;
	return (float)i>v ? i-1 : i;
}

int CeilToInt(float v)
{
	int i = (int)v;
	return (float)i<v ? i+1 : i;
}

int RoundToInt(float v)
{
	int i = (int)v;
	return v>=0.0f ? ((float)i+0.5f<=v ? i+1 : i) : ((float)i-0.5f>=v ? i-1 : i);
}

ui32 WangHash(ui32 x)
{
	x = (x ^ 61u) ^ (x >> 16);
	x *= 9u;
	x = x ^ (x >> 4);
	x *= 0x27d4eb2du;
	x = x ^ (x >> 15);
	return x;
}

float Rand01(ui32& seed)
{
	seed = WangHash(seed);
	return (seed & 0x00FFFFFF) / 16777216.0f;
}

float RandSigned(ui32& seed)
{
	return Rand01(seed)*2.0f - 1.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

bool RayObb(cpu_ray& ray, cpu_obb& box, XMFLOAT3* pOutHit, float* pOutT)
{
	const float eps = 1e-8f;

	XMVECTOR C = XMLoadFloat3(&box.center);
	XMVECTOR U = XMLoadFloat3(&box.axis[0]);
	XMVECTOR V = XMLoadFloat3(&box.axis[1]);
	XMVECTOR W = XMLoadFloat3(&box.axis[2]);

	// U = XMVector3Normalize(U); V = XMVector3Normalize(V); W = XMVector3Normalize(W);

	XMVECTOR O  = XMLoadFloat3(&ray.pos);
	XMVECTOR D  = XMLoadFloat3(&ray.dir);
	XMVECTOR OC = XMVectorSubtract(O, C);

	// Projection du rayon dans l'espace local OBB
	float oX = XMVectorGetX(XMVector3Dot(OC, U));
	float oY = XMVectorGetX(XMVector3Dot(OC, V));
	float oZ = XMVectorGetX(XMVector3Dot(OC, W));
	float dX = XMVectorGetX(XMVector3Dot(D, U));
	float dY = XMVectorGetX(XMVector3Dot(D, V));
	float dZ = XMVectorGetX(XMVector3Dot(D, W));

	// Slab sur AABB local [-half.x,half.x] x [-half.y,half.y] x [-half.z,half.z]
	float tmin = 0.0f;
	float tmax = 1e30f;

	auto slab = [&](float o, float d, float e) -> bool
	{
		if ( fabsf(d)<eps )
			return o>=-e && o<=e;

		float invD = 1.0f / d;
		float t1 = (-e - o) * invD;
		float t2 = ( e - o) * invD;
		if ( t1>t2 )
		{
			float tmp = t1;
			t1 = t2;
			t2 = tmp;
		}
		tmin = std::max(tmin, t1);
		tmax = std::min(tmax, t2);
		return (tmin <= tmax);
	};

	if ( slab(oX, dX, box.half.x)==false )
		return false;
	if ( slab(oY, dY, box.half.y)==false )
		return false;
	if ( slab(oZ, dZ, box.half.z)==false )
		return false;
	if ( tmax<0.0f )
		return false;

	float tHit = tmin>=0.0f ? tmin : 0.0f;
	if ( pOutHit )
	{
		XMVECTOR P = XMVectorAdd(O, XMVectorScale(D, tHit));
		XMStoreFloat3(pOutHit, P);
	}
	if ( pOutT )
		*pOutT = tHit;

	return true;
}

bool RaySphere(cpu_ray& ray, XMFLOAT3& center, float radius, XMFLOAT3& outHit, float* pOutT)
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
	else if ( t1>=0.0f )
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
	if ( pOutT )
		*pOutT = t;
	return true;
}

// Retourne true si intersection.
// outHit = point d’intersection (premier point rencontré pour t >= 0).
// outT = paramètre t (optionnel).
// outBary = barycentriques (u,v,w) optionnel, utile pour interpoler (normal, uv, etc.).
bool RayTriangle(cpu_ray& ray, cpu_triangle& tri, XMFLOAT3& outHit, float* pOutT , XMFLOAT3* pOutBary, bool cullBackFace)
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
		if ( pOutT )
			*pOutT = t;
		if ( pOutBary )
		{
			// u' = u/det, v' = v/det, w = 1-u'-v'
			const float invDet = 1.0f / det;
			const float uu = u * invDet;
			const float vv = v * invDet;
			pOutBary->x = uu;
			pOutBary->y = vv;
			pOutBary->z = 1.0f - uu - vv;
		}
		return true;
	}
	else
	{
		if ( fabsf(det)<=eps )
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
		if ( pOutT )
			*pOutT = t;
		if ( pOutBary )
		{
			pOutBary->x = u;
			pOutBary->y = v;
			pOutBary->z = 1.0f - u - v;
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

bool ObbObb(cpu_obb& a, cpu_obb& b)
{
	constexpr float EPS = 1e-6f;

	// R[i][j] = dot(A.axis[i], B.axis[j])
	float R[3][3], AbsR[3][3];

	for ( int i=0 ; i<3 ; ++i )
	{
		for ( int j=0 ; j<3 ; ++j )
		{
			R[i][j] = a.axis[i].x * b.axis[j].x + a.axis[i].y * b.axis[j].y + a.axis[i].z * b.axis[j].z;
			AbsR[i][j] = fabsf(R[i][j]) + EPS;
		}
	}

	// t = centreB - centreA (world)
	float tW[3] =
	{
		b.center.x - a.center.x,
		b.center.y - a.center.y,
		b.center.z - a.center.z
	};

	// t exprimé dans la base de A
	float t[3] =
	{
		tW[0]*a.axis[0].x + tW[1]*a.axis[0].y + tW[2]*a.axis[0].z,
		tW[0]*a.axis[1].x + tW[1]*a.axis[1].y + tW[2]*a.axis[1].z,
		tW[0]*a.axis[2].x + tW[1]*a.axis[2].y + tW[2]*a.axis[2].z
	};

	const float aa[3] = { a.half.x, a.half.y, a.half.z };
	const float bb[3] = { b.half.x, b.half.y, b.half.z };

	float ra, rb;

	// 1) Axes de A
	for ( int i=0 ; i<3 ; ++i  )
	{
		ra = aa[i];
		rb = bb[0]*AbsR[i][0] + bb[1]*AbsR[i][1] + bb[2]*AbsR[i][2];
		if ( fabsf(t[i])>ra+rb )
			return false;
	}

	// 2) Axes de B
	for ( int j=0 ; j<3 ; ++j )
	{
		ra = aa[0]*AbsR[0][j] + aa[1]*AbsR[1][j] + aa[2]*AbsR[2][j];
		rb = bb[j];

		float tOnBj = t[0]*R[0][j] + t[1]*R[1][j] + t[2]*R[2][j];
		if ( fabsf(tOnBj)>ra+rb )
			return false;
	}

	// 3) Axes croisés Ai x Bj (9 tests)
	ra = aa[1]*AbsR[2][0] + aa[2]*AbsR[1][0];
	rb = bb[1]*AbsR[0][2] + bb[2]*AbsR[0][1];
	if ( fabsf(t[2]*R[1][0]-t[1]*R[2][0])>ra+rb )
		return false;

	ra = aa[1]*AbsR[2][1] + aa[2]*AbsR[1][1];
	rb = bb[0]*AbsR[0][2] + bb[2]*AbsR[0][0];
	if ( fabsf(t[2]*R[1][1]-t[1]*R[2][1])>ra+rb )
		return false;

	ra = aa[1]*AbsR[2][2] + aa[2]*AbsR[1][2];
	rb = bb[0]*AbsR[0][1] + bb[1]*AbsR[0][0];
	if ( fabsf(t[2]*R[1][2]-t[1]*R[2][2])>ra+rb )
		return false;

	ra = aa[0]*AbsR[2][0] + aa[2]*AbsR[0][0];
	rb = bb[1]*AbsR[1][2] + bb[2]*AbsR[1][1];
	if ( fabsf(t[0]*R[2][0]-t[2]*R[0][0])>ra+rb )
		return false;

	ra = aa[0]*AbsR[2][1] + aa[2]*AbsR[0][1];
	rb = bb[0]*AbsR[1][2] + bb[2]*AbsR[1][0];
	if ( fabsf(t[0]*R[2][1]-t[2]*R[0][1])>ra+rb )
		return false;

	ra = aa[0]*AbsR[2][2] + aa[2]*AbsR[0][2];
	rb = bb[0]*AbsR[1][1] + bb[1]*AbsR[1][0];
	if ( fabsf(t[0]*R[2][2]-t[2]*R[0][2])>ra+rb )
		return false;

	ra = aa[0]*AbsR[1][0] + aa[1]*AbsR[0][0];
	rb = bb[1]*AbsR[2][2] + bb[2]*AbsR[2][1];
	if ( fabsf(t[1]*R[0][0]-t[0]*R[1][0])>ra+rb )
		return false;

	ra = aa[0]*AbsR[1][1] + aa[1]*AbsR[0][1];
	rb = bb[0]*AbsR[2][2] + bb[2]*AbsR[2][0];
	if ( fabsf(t[1]*R[0][1]-t[0]*R[1][1])>ra+rb )
		return false;

	ra = aa[0]*AbsR[1][2] + aa[1]*AbsR[0][2];
	rb = bb[0]*AbsR[2][1] + bb[1]*AbsR[2][0];
	if ( fabsf(t[1]*R[0][2]-t[0]*R[1][2])>ra+rb )
		return false;

	return true;
}

XMFLOAT3 SphericalPoint(float r, float theta, float phi)
{
	float st = sinf(theta);
	float ct = cosf(theta);
	float sp = sinf(phi);
	float cp = cosf(phi);
	return XMFLOAT3(r * st * cp, r * ct, r * st * sp);
}

RECT ComputeAspectFitRect(int contentW, int contentH, int winW, int winH)
{
	RECT r{0,0,0,0};
	if ( contentW<=0 || contentH<=0 || winW<=0 || winH<=0 )
		return r;

	const float sx = (float)winW / (float)contentW;
	const float sy = (float)winH / (float)contentH;
	const float s = sx<sy ? sx : sy;

	const int drawW = (int)std::lround(contentW * s);
	const int drawH = (int)std::lround(contentH * s);

	r.left = (winW - drawW) / 2;
	r.top = (winH - drawH) / 2;
	r.right = r.left + drawW;
	r.bottom = r.top + drawH;
	return r;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

byte* LoadFile(const char* path, int& size)
{
	byte* data = nullptr;
	FILE* file;
	if ( fopen_s(&file, path, "rb") )
		return data;
	fseek(file, 0, SEEK_END);
	size = ftell(file);
	fseek(file, 0, SEEK_SET);
	if ( size>0 )
	{
		data = new byte[size];
		fread(data, 1, size, file);
	}
	fclose(file);
	return data;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

}
