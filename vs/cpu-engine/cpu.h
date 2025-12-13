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

// Engine
///////////

#undef near
#undef far
#undef min
#undef max

using ui32								= unsigned __int32;

inline XMVECTOR XMRIGHT					= g_XMIdentityR0;
inline XMVECTOR XMUP					= g_XMIdentityR1;
inline XMVECTOR XMDIR					= g_XMIdentityR2;

#define DELPTR(p)						{ if ( (p) ) { delete (p); (p) = nullptr; } }
#define RELPTR(p)						{ if ( (p) ) { (p)->Release(); (p) = nullptr; } }

// Forward declaration
struct AABB;
struct CAMERA;
struct OBB;
class Engine;

// Core
#include "Keyboard.h"
#include "Thread.h"

// Engine
#include "Geometry.h"
#include "Entity.h"
#include "Multithreading.h"
#include "Engine.h"
