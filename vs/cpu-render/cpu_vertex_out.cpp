#include "pch.h"

void cpu_vertex_out::Lerp(const cpu_vertex_out& a, const cpu_vertex_out& b, float t)
{
	cpu::Lerp(worldPos, a.worldPos, b.worldPos, t);
	cpu::Lerp(clipPos, a.clipPos, b.clipPos, t);
	cpu::Lerp(worldNormal, a.worldNormal, b.worldNormal, t);
	cpu::Lerp(albedo, a.albedo, b.albedo, t);
	cpu::Lerp(intensity, a.intensity, b.intensity, t);
	cpu::Lerp(uv, a.uv, b.uv, t);
	cpu::Lerp(invW, a.invW, b.invW, t);
}
