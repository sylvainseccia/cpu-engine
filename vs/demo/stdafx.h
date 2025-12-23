#pragma once

#ifdef _DEBUG
	#pragma comment(lib, "../x64/debug/cpu-engine.lib")
#else
	#pragma comment(lib, "../x64/release/cpu-engine.lib")
#endif

#include <SDKDDKVer.h>
#include "../cpu-engine/cpu.h"

class Ship;

#include "App.h"
