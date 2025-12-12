#pragma once

// Librairies
///////////////

#pragma comment(lib, "winmm.lib")

// Windows
///////////

#include "targetver.h"
#include <windows.h>
#undef min
#undef max
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

#include <DirectXMath.h>

//#define GPU_PRESENT
#ifdef GPU_PRESENT
	#pragma comment(lib, "d2d1.lib")
	#include <d2d1.h>
#endif

// Engine
///////////

using ui32 = unsigned __int32;

struct AABB;
struct OBB;
class Engine;
class Game;

#define DELPTR(p)				{ if ( (p) ) { delete (p); (p) = nullptr; } }
#define RELPTR(p)				{ if ( (p) ) { (p)->Release(); (p) = nullptr; } }

#include "Thread.h"
#include "Engine.h"
