#pragma once

struct cpu_particle_emitter : public cpu_object
{
public:
	cpu_particle_data* pData;

	byte blend;				// blending method

	float rate;				// particles per second
	float spawnRadius;		// volume d'émission

	XMFLOAT3 pos;			// world position
	XMFLOAT3 dir;			// normalized direction
	XMFLOAT3 colorMin;		// color range
	XMFLOAT3 colorMax;		// color range
	float durationMin;		// duration range
	float durationMax;		// duration range
	float speedMin;			// speed range
	float speedMax;			// speed range
	float spread;			// dispersion directionnelles
							// 0		=> jet laser, pluie parfaitement verticale, rayon
							// 0.05-0.2 => fumée canalisée, vapeur, souffle
							// 0.3-0.7	=> feu, poussière, étincelles
							// >1		=> explosion, chaos

private:
	float accum;

public:
	cpu_particle_emitter();

	void Update(int pixelCount);
};
