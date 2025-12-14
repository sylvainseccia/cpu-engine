#include "stdafx.h"

MATERIAL::MATERIAL()
{
	//lighting = UNLIT;
	lighting = GOURAUD;
	//lighting = LAMBERT;
	ps = nullptr;
	color = { 1.0f, 1.0f, 1.0f };
	data = nullptr;
}
