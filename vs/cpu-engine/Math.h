#pragma once

namespace cpu_math
{

static inline XMFLOAT3 Sub3(const XMFLOAT3& a, const XMFLOAT3& b) { return {a.x-b.x, a.y-b.y, a.z-b.z}; }
static inline XMFLOAT3 Add3(const XMFLOAT3& a, const XMFLOAT3& b) { return {a.x+b.x, a.y+b.y, a.z+b.z}; }
static inline XMFLOAT3 Mul3(const XMFLOAT3& a, float s) { return {a.x*s, a.y*s, a.z*s}; }
static inline float Dot3(const XMFLOAT3& a, const XMFLOAT3& b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
static inline XMFLOAT3 Cross3(const XMFLOAT3& a, const XMFLOAT3& b) { return { a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x }; }

bool SphereSphere(XMFLOAT3& c1, float r1, XMFLOAT3& c2, float r2);
bool RaySphere(cpu_ray& ray, XMFLOAT3& center, float radius, XMFLOAT3& outHit, float* pOutT = nullptr);
bool RayAabb(cpu_ray& ray, cpu_aabb& box, XMFLOAT3* pOutHit = nullptr, float* outT = nullptr);
bool RayAabb(cpu_ray& ray, cpu_aabb& box, float& outTEnter, float& outTExit);
bool RayTriangle(cpu_ray& ray, cpu_triangle& tri, XMFLOAT3& outHit, float* outT = nullptr, XMFLOAT3* outBary = nullptr, bool cullBackFace = false);
bool AabbAabb(cpu_aabb& a, cpu_aabb& b);
bool AabbAabbInclusive(cpu_aabb& a, cpu_aabb& b);

}
