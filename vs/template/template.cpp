#include "stdafx.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int cmdShow)
{
	if ( InitializeEngine() )
	{
		App app;
		app.Initialize(hInstance, 1024, 576);
		app.Run();
	}
	UninitializeEngine();
	return 0;
}
