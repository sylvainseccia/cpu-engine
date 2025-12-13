#pragma once

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

// DirectX
////////////

// Enable GPU_PRESENT for improved stretching when window size != render size
// or if you want to use V-Sync
//#define GPU_PRESENT

#ifdef GPU_PRESENT
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
struct AABB;
struct CAMERA;
struct OBB;
struct PIXELSHADER;
class Engine;

// Types
using i16								= __int16;
using ui16								= unsigned __int16;
using i32								= __int32;
using ui32								= unsigned __int32;
using i64								= __int64;
using ui64								= unsigned __int64;
using PS_FUNC							= bool(*)(XMFLOAT3& out, const PIXELSHADER& in, const void* data);

// Macro
#define DELPTR(p)						{ if ( (p) ) { delete (p); (p) = nullptr; } }
#define RELPTR(p)						{ if ( (p) ) { (p)->Release(); (p) = nullptr; } }

// Light
#define UNLIT							0
#define GOURAUD							1
#define LAMBERT							2

// Core
#include "lodepng.h"
#include "Keyboard.h"
#include "Thread.h"

// Engine
#include "UI.h"
#include "Font.h"
#include "Geometry.h"
#include "Entity.h"
#include "Multithreading.h"
#include "Engine.h"
