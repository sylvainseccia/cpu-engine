#include "stdafx.h"

cpu_material::cpu_material()
{
#ifdef _DEBUG
	lighting = LIGHTING_GOURAUD;
#else
	lighting = LIGHTING_LAMBERT;
#endif

	ps = nullptr;
	color = WHITE;
	values = nullptr;
}
