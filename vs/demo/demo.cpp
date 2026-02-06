#include "pch.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int cmdShow)
{
	// AMIGA NTSC FULLSCREEN
	//CPU_RUN(320, 200, true, true);

	// AMIGA PAL FULLSCREEN
	//CPU_RUN(320, 256, true, true);

	// RETRO FULLSCREEN
	CPU_RUN(512, 256, true, true);

	// MODERN
	//CPU_RUN(1024, 576);

	// FULL HD (please use release)
	//CPU_RUN(1920, 1080, true);
	return 0;
}
