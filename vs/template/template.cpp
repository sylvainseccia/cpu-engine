#include "stdafx.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int cmdShow)
{
	CPU_RUN(hInstance, 1024, 576);
	return 0;
}
