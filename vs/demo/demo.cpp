#include "stdafx.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int cmdShow)
{
	// AMIGA NTSC FULLSCREEN
	//CPU_RUN(hInstance, 320, 200, true, true);

	// AMIGA PAL FULLSCREEN
	//CPU_RUN(hInstance, 320, 256, true, true);

	// RETRO FULLSCREEN
	CPU_RUN(hInstance, 512, 256, true, true);

	// MODERN
	//CPU_RUN(hInstance, 1024, 576);

	// FULL HD (please use release)
	//CPU_RUN(hInstance, 1920, 1080);

	return 0;
}
