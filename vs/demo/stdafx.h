#pragma once

#ifdef _DEBUG
	#pragma comment(lib, "../x64/debug/cpu-engine.lib")
#else
	#pragma comment(lib, "../x64/release/cpu-engine.lib")
#endif

#include "../cpu-engine/cpu.h"
#include "Game.h"
