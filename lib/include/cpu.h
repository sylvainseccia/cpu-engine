#pragma once

// Author
///////////

// Sylvain Seccia
// https://www.seccia.com

// Libraries
///////////////

#pragma comment(lib, "winmm.lib")

// Windows
///////////

#include <windows.h>

#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <string>
#include <vector>
#include <list>
#include <algorithm>
#include <map>
#include <thread>
#include <functional>
#include <cmath>
#include <emmintrin.h>
#ifdef _DEBUG
	#include <crtdbg.h>
#endif

// Config
///////////

#include "config.h"

// DirectX
////////////

#ifdef CONFIG_GPU
	#pragma comment(lib, "d2d1.lib")
	#include <d2d1.h>
#endif

#include <DirectXMath.h>
using namespace DirectX;
inline XMVECTOR CPU_XMRIGHT					= g_XMIdentityR0;
inline XMVECTOR CPU_XMUP					= g_XMIdentityR1;
inline XMVECTOR CPU_XMDIR					= g_XMIdentityR2;

// Engine
///////////

#undef near
#undef far
#undef min
#undef max
#undef DrawText
#undef SetJob

// Forward declarations
struct cpu_aabb;
struct cpu_camera;
struct cpu_obb;
struct cpu_ps_io;
struct cpu_tile;
struct cpu_triangle;
struct cpu_ray;
class cpu_engine;
class cpu_job;

// Types
using i16								= __int16;
using ui16								= unsigned __int16;
using i32								= __int32;
using ui32								= unsigned __int32;
using i64								= __int64;
using ui64								= unsigned __int64;
using CPU_PS_FUNC						= void(*)(cpu_ps_io& data);

// Memory
#ifdef _DEBUG
	#define DEBUG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
	#define new DEBUG_NEW
#endif

// Macro
#define APP								App::GetInstance()
#define CPU								cpu_engine::GetInstance()
#define CPU_RELEASE(p)					{ if ( (p) ) { (p)->Release(); (p) = nullptr; } }
#define CPU_DELPTR(p)					{ if ( (p) ) { delete (p); (p) = nullptr; } }
#define CPU_DELPTRS(p)					{ if ( (p) ) { delete [] (p); (p) = nullptr; } }
#define CPU_STR(v)						std::to_string(v)
#define CPU_CAPTION(v)					SetWindowText(cpu.GetHWND(), STR(v).c_str());
#define CPU_ID(s)						cpu::GetStateID<s>()
#define CPU_RGBX(r,g,b)					((ui32)(((byte)(r)|((ui16)((byte)(g))<<8))|(((ui16)(byte)(b))<<16))|0xFF000000)
#define CPU_RGBA(r,g,b,a)				((ui32)(((byte)(r)|((ui16)((byte)(g))<<8))|(((ui16)(byte)(b))<<16))|(((ui32)(byte)(a))<<24))
#define CPU_R(rgba)						((rgba)&0xFF)
#define CPU_G(rgba)						(((rgba)>>8)&0xFF)
#define CPU_B(rgba)						(((rgba)>>16)&0xFF)
#define CPU_A(rgba)						(((rgba)>>24)&0xFF)
#define CPU_JOBS(j)						{m_nextTile=0;for(size_t i=0;i<(j).size();i++)(j)[i].GetThread()->PostStartEvent(&(j)[i]);for(size_t i=0;i<(j).size();i++)(j)[i].GetThread()->WaitEndEvent();}
#define CPU_RUN							cpu::Run<cpu_engine, App>
#define CPU_CALLBACK_START(method)		CPU.GetCallback()->onStart.Set(this, &App::method)
#define CPU_CALLBACK_UPDATE(method)		CPU.GetCallback()->onUpdate.Set(this, &App::method)
#define CPU_CALLBACK_EXIT(method)		CPU.GetCallback()->onExit.Set(this, &App::method)
#define CPU_CALLBACK_RENDER(method)		CPU.GetCallback()->onRender.Set(this, &App::method)

// Float3
inline XMFLOAT3 CPU_RIGHT				= { 1.0f, 0.0f, 0.0f };
inline XMFLOAT3 CPU_UP					= { 0.0f, 1.0f, 0.0f };
inline XMFLOAT3 CPU_DIR					= { 0.0f, 0.0f, 1.0f };
inline XMFLOAT3 CPU_ZERO				= { 0.0f, 0.0f, 0.0f };
inline XMFLOAT3 CPU_ONE					= { 1.0f, 1.0f, 1.0f };
inline XMFLOAT3 CPU_WHITE				= { 1.0f, 1.0f, 1.0f };
inline XMFLOAT3 CPU_BLACK				= { 0.0f, 0.0f, 0.0f };
inline XMFLOAT3 CPU_RED					= { 1.0f, 0.0f, 0.0f };
inline XMFLOAT3 CPU_BLUE				= { 0.0f, 0.0f, 1.0f };
inline XMFLOAT3 CPU_GREEN				= { 0.0f, 1.0f, 0.0f };
inline XMFLOAT3 CPU_ORANGE				= { 1.0f, 0.5f, 0.0f };

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

// Pass
#define CPU_PASS_CLEAR_BEGIN			10
#define CPU_PASS_CLEAR_END				11
#define CPU_PASS_ENTITY_BEGIN			20
#define CPU_PASS_ENTITY_END				21
#define CPU_PASS_PARTICLE_BEGIN			30
#define CPU_PASS_PARTICLE_END			31
#define CPU_PASS_UI_BEGIN				40
#define CPU_PASS_UI_END					41
#define CPU_PASS_CURSOR_BEGIN			50
#define CPU_PASS_CURSOR_END				51

// Core
#include "png32.h"
#include "img32.h"
#include "global.h"
#include "Input.h"
#include "Thread.h"
#include "Function.h"

// Engine
#include "Manager.h"
#include "State.h"
#include "UI.h"
#include "Particle.h"
#include "Geometry.h"
#include "Shader.h"
#include "Entity.h"
#include "Multithreading.h"
#include "Engine.h"
