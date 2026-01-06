#pragma once

#ifdef _DEBUG
	#pragma comment(lib, "../x64/Debug/cpu-core.lib")
	#pragma comment(lib, "../x64/Debug/cpu-render.lib")
	#pragma comment(lib, "../x64/debug/cpu-engine.lib")
#else
	#pragma comment(lib, "../x64/Release/cpu-core.lib")
	#pragma comment(lib, "../x64/Release/cpu-render.lib")
	#pragma comment(lib, "../x64/Release/cpu-engine.lib")
#endif

#include <SDKDDKVer.h>
#include "../cpu-engine/cpu-engine.h"

#include "App.h"
