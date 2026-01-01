#pragma once

// Sylvain Seccia
// https://www.seccia.com

// Library
#ifdef _DEBUG
	#pragma comment(lib, "../x64/Debug/cpu-core.lib")
#else
	#pragma comment(lib, "../x64/Release/cpu-core.lib")
#endif

// Include
#include "../cpu-core/cpu-core.h"

// Forward declarations
struct cpu_camera;
struct cpu_ps_io;

// Types
using CPU_PS_FUNC						= void(*)(cpu_ps_io& data);

// Light
#define CPU_LIGHTING_UNLIT				0
#define CPU_LIGHTING_GOURAUD			1
#define CPU_LIGHTING_LAMBERT			2

// Text
#define CPU_TEXT_LEFT					0
#define CPU_TEXT_CENTER					1
#define CPU_TEXT_RIGHT					2

// Clear
#define CPU_CLEAR_NONE					0
#define CPU_CLEAR_COLOR					1
#define CPU_CLEAR_SKY					2

// Depth
#define CPU_DEPTH_NONE					0
#define CPU_DEPTH_READ					1
#define CPU_DEPTH_WRITE					2
#define CPU_DEPTH_RW					4

// Engine
#include "cpu_global_render.h"
#include "cpu_tile.h"
//#include "cpu_thread_job.h"
//#include "cpu_job.h"
#include "cpu_transform.h"
#include "cpu_texture.h"
#include "cpu_sprite.h"
#include "cpu_font.h"
#include "cpu_light.h"
#include "cpu_frustum.h"
#include "cpu_camera.h"
#include "cpu_particle_physics.h"
#include "cpu_particle_data.h"
#include "cpu_particle_emitter.h"
#include "cpu_material.h"
#include "cpu_vertex_out.h"
#include "cpu_pixel.h"
#include "cpu_ps_io.h"
#include "cpu_draw.h"
#include "cpu_rt.h"
#include "cpu_device.h"
