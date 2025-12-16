#include "stdafx.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int cmdShow)
{
	if ( InitializeEngine() )
	{
		App app;
		//app.Initialize(hInstance, 320, 200, 5.0f);		// AMIGA NTSC
		app.Initialize(hInstance, 1280, 800);
		app.Run();
	}
	UninitializeEngine();
	return 0;
}
