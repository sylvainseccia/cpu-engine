#include "stdafx.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int cmdShow)
{
	// AMIGA
	//CPU_RUN(hInstance, 320, 200, true, true);		// AMIGA NTSC
	//CPU_RUN(hInstance, 320, 256, true, true);		// AMIGA PAL

	// RETRO
	CPU_RUN(hInstance, 512, 256, true, true);			// fullscreen

	// MODERN
	//CPU_RUN(hInstance, 1024, 576);

	// FULL HD (please use release)
	//CPU_RUN(hInstance, 1920, 1080);

	return 0;
}
