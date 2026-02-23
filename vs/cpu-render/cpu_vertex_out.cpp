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
	cpu::Lerp(uv, a.uv, b.uv, t);
}
