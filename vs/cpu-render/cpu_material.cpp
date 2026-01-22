#include "pch.h"

cpu_material::cpu_material()
{
#ifdef _DEBUG
	lighting = CPU_LIGHTING_GOURAUD;
#else
	lighting = CPU_LIGHTING_LAMBERT;
#endif

	ps = nullptr;
	color = CPU_WHITE;
	pTexture = nullptr;
	values = nullptr;
}
