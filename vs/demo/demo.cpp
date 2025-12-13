#include "stdafx.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int cmdShow)
{
	Game game;
	//game.Initialize(hInstance, 320, 200, 4.0f); // AMIGA NTSC
	game.Initialize(hInstance, 1280, 800);
	game.Run();
	game.Uninitialize();
	return 0;
}
