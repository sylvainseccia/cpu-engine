#pragma once

// Author
///////////

// Sylvain Seccia
// https://www.seccia.com

// Libraries
//////////////

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "xinput.lib")
#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "msacm32.lib")		// MP3
#pragma comment(lib, "wmvcore.lib")		// MP3

// Windows
///////////

#include <winsock2.h>
#include <windows.h>
#include <Ws2tcpip.h>

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

#include <xinput.h>
#include <xaudio2.h>
#include <msacm.h>						// MP3
#include <wmsdk.h>						// MP3

// DirectX
////////////

// Enable CPU_CONFIG_GPU for improved stretching when window size != render size OR if you want to use V-Sync
#define CPU_CONFIG_GPU
#ifdef CPU_CONFIG_GPU
	#pragma comment(lib, "d2d1.lib")
	#include <d2d1.h>
#endif

#include <DirectXMath.h>
using namespace DirectX;
inline XMVECTOR CPU_XMRIGHT				= g_XMIdentityR0;
inline XMVECTOR CPU_XMUP				= g_XMIdentityR1;
inline XMVECTOR CPU_XMDIR				= g_XMIdentityR2;

// Engine
///////////

#undef near
#undef far
#undef min
#undef max
#undef DrawText
#undef SetJob

// Enable CPU_CONFIG_MT if you want to use Multi-Threading
#ifdef _DEBUG
	//#define CPU_CONFIG_MT
#else
	#define CPU_CONFIG_MT
#endif

// Forward declarations
struct cpu_aabb;
struct cpu_input;
struct cpu_obb;
struct cpu_triangle;
struct cpu_ray;
class cpu_window;

// Types
using i16								= __int16;
using ui16								= unsigned __int16;
using i32								= __int32;
using ui32								= unsigned __int32;
using i64								= __int64;
using ui64								= unsigned __int64;

// Memory
#ifdef _DEBUG
	#define DEBUG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
	#define new DEBUG_NEW
#endif

// Singletons
#define cpuAudio						cpu_audio::GetInstance()
#define cpuInput						cpu_input::GetInstance()
#define cpuTime							cpu_time::GetInstance()

// Macro
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

// Constants
#define CPU_ZERO						1e-20f
#define CPU_EPSILON						1e-12f

// Controller
#define CPU_INPUT_ACTIONS				8
#define CPU_XINPUT_A					0
#define CPU_XINPUT_B					1
#define CPU_XINPUT_X					2
#define CPU_XINPUT_Y					3
#define CPU_XINPUT_LS					4
#define CPU_XINPUT_RS					5
#define CPU_XINPUT_COUNT				6

// Float3
inline XMFLOAT3 CPU_VEC3_RIGHT			= { 1.0f, 0.0f, 0.0f };
inline XMFLOAT3 CPU_VEC3_UP				= { 0.0f, 1.0f, 0.0f };
inline XMFLOAT3 CPU_VEC3_DIR			= { 0.0f, 0.0f, 1.0f };
inline XMFLOAT3 CPU_VEC3_ZERO			= { 0.0f, 0.0f, 0.0f };
inline XMFLOAT3 CPU_VEC3_ONE			= { 1.0f, 1.0f, 1.0f };
inline XMFLOAT3 CPU_WHITE				= { 1.0f, 1.0f, 1.0f };
inline XMFLOAT3 CPU_BLACK				= { 0.0f, 0.0f, 0.0f };
inline XMFLOAT3 CPU_RED					= { 1.0f, 0.0f, 0.0f };
inline XMFLOAT3 CPU_BLUE				= { 0.0f, 0.0f, 1.0f };
inline XMFLOAT3 CPU_GRAY				= { 0.5f, 0.5f, 0.5f };
inline XMFLOAT3 CPU_GREEN				= { 0.0f, 1.0f, 0.0f };
inline XMFLOAT3 CPU_MAGENTA				= { 1.0f, 0.0f, 1.0f };
inline XMFLOAT3 CPU_ORANGE				= { 1.0f, 0.5f, 0.0f };

// Core
#include "cpu_png32.h"
#include "cpu_img32.h"
#include "cpu_global.h"
#include "cpu_atomic.h"
#include "cpu_object.h"
#include "cpu_time.h"
#include "cpu_sound.h"
#include "cpu_player.h"
#include "cpu_audio.h"
#include "cpu_xinput_state.h"
#include "cpu_xinput.h"
#include "cpu_vinput.h"
#include "cpu_input.h"
#include "cpu_thread.h"
#include "cpu_function.h"
#include "cpu_vec3_cmp.h"
#include "cpu_vertex.h"
#include "cpu_triangle.h"
#include "cpu_rectangle.h"
#include "cpu_aabb.h"
#include "cpu_obb.h"
#include "cpu_sphere.h"
#include "cpu_ray.h"
#include "cpu_transform.h"
#include "cpu_mesh.h"
#include "cpu_window.h"
