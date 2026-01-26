#include "pch.h"

void cpu_vertex_out::Lerp(const cpu_vertex_out& a, const cpu_vertex_out& b, float t)
{
	// World
	cpu::Lerp(worldPos, a.worldPos, b.worldPos, t);

	// Clip
	cpu::Lerp(clipPos, a.clipPos, b.clipPos, t);

	// Normal
	cpu::Lerp(worldNormal, a.worldNormal, b.worldNormal, t);

	// Albedo
	cpu::Lerp(albedo, a.albedo, b.albedo, t);

	// Intensity
	cpu::Lerp(intensity, a.intensity, b.intensity, t);

	// UV
	float wA = a.clipPos.w;
	float wB = b.clipPos.w;
	float uA = a.uv.x * wA;
	float vA = a.uv.y * wA;
	float uB = b.uv.x * wB;
	float vB = b.uv.y * wB;
	float u = uA + (uB - uA) * t;
	float v = vA + (vB - vA) * t;
	float w = clipPos.w;
	const float eps = 1e-8f;
	float invW = fabsf(w)>eps ? (1.0f/w) : 0.0f;
	uv.x = u * invW;
	uv.y = v * invW;
}
