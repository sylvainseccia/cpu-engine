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
#include <immintrin.h> // AVX2
#ifdef _DEBUG
	#include <crtdbg.h>
#endif
//#include <cstdint>


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
inline XMVECTOR XMRIGHT					= g_XMIdentityR0;
inline XMVECTOR XMUP					= g_XMIdentityR1;
inline XMVECTOR XMDIR					= g_XMIdentityR2;

// Engine
///////////

#undef near
#undef far
#undef min
#undef max
#undef DrawText

// Forward declarations
struct cpu_aabb;
struct cpu_camera;
struct cpu_obb;
struct cpu_ps_io;
struct cpu_tile;
class cpu_engine;

// Types
using i16								= __int16;
using ui16								= unsigned __int16;
using i32								= __int32;
using ui32								= unsigned __int32;
using i64								= __int64;
using ui64								= unsigned __int64;
using PS_FUNC							= void(*)(cpu_ps_io& data);

// Memory
#ifdef _DEBUG
	#define DEBUG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
	#define new DEBUG_NEW
#endif

// Macro
#define RELPTR(p)						{ if ( (p) ) { (p)->Release(); (p) = nullptr; } }
#define DELPTR(p)						{ if ( (p) ) { delete (p); (p) = nullptr; } }
#define DELPTRS(p)						{ if ( (p) ) { delete [] (p); (p) = nullptr; } }
#define I(p)							p::GetInstance()
#define CAPTION(v)						SetWindowText(cpu.GetHWND(), std::to_string(v).c_str());
#define ID(s)							GetStateID<s>()
#define RGBX(r,g,b)						((ui32)(((byte)(r)|((ui16)((byte)(g))<<8))|(((ui16)(byte)(b))<<16))|0xFF000000)
#define RGBA(r,g,b,a)					((ui32)(((byte)(r)|((ui16)((byte)(g))<<8))|(((ui16)(byte)(b))<<16))|(((ui32)(byte)(a))<<24))
#define R(rgba)							((rgba)&0xFF)
#define G(rgba)							(((rgba)>>8)&0xFF)
#define B(rgba)							(((rgba)>>16)&0xFF)
#define A(rgba)							(((rgba)>>24)&0xFF)
#define JOBS(j)							{m_nextTile=0;for(size_t i=0;i<(j).size();i++)(j)[i].PostStartEvent();for(size_t i=0;i<(j).size();i++)(j)[i].WaitEndEvent();}

// Special
#define cpu								cpu_engine::GetInstanceRef()
#define input							cpu_engine::GetInstance()->GetInput()
#define dtime							cpu_engine::GetInstance()->GetDeltaTime()
#define ttime							cpu_engine::GetInstance()->GetTotalTime()
#define since(t)						(cpu_engine::GetInstance()->GetTime()-t)

// Float3
inline XMFLOAT3 RIGHT					= { 1.0f, 0.0f, 0.0f };
inline XMFLOAT3 UP						= { 0.0f, 1.0f, 0.0f };
inline XMFLOAT3 DIR						= { 0.0f, 0.0f, 1.0f };
inline XMFLOAT3 ZERO					= { 0.0f, 0.0f, 0.0f };
inline XMFLOAT3 ONE						= { 1.0f, 1.0f, 1.0f };
inline XMFLOAT3 WHITE					= { 1.0f, 1.0f, 1.0f };
inline XMFLOAT3 BLACK					= { 0.0f, 0.0f, 0.0f };
inline XMFLOAT3 RED						= { 1.0f, 0.0f, 0.0f };
inline XMFLOAT3 BLUE					= { 0.0f, 0.0f, 1.0f };
inline XMFLOAT3 GREEN					= { 0.0f, 1.0f, 0.0f };
inline XMFLOAT3 ORANGE					= { 1.0f, 0.5f, 0.0f };

// Light
#define LIGHTING_UNLIT					0
#define LIGHTING_GOURAUD				1
#define LIGHTING_LAMBERT				2

// Text
#define TEXT_LEFT						0
#define TEXT_CENTER						1
#define TEXT_RIGHT						2

// Clear
#define CLEAR_NONE						0
#define CLEAR_TRANSPARENT				1
#define CLEAR_COLOR						2
#define CLEAR_SKY						3

// Depth
#define DEPTH_READ						1
#define DEPTH_WRITE						2

// Core
#include "png32.h"
#include "avx2.h"
#include "global.h"
#include "Input.h"
#include "Thread.h"

// Engine
#include "State.h"
#include "UI.h"
#include "Font.h"
#include "Particle.h"
#include "Geometry.h"
#include "Shader.h"
#include "Entity.h"
#include "Multithreading.h"
#include "Engine.h"
