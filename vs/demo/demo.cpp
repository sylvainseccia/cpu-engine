#include "stdafx.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int cmdShow)
{
	Game game;
	game.Initialize(hInstance, 1280, 720);
	//game.Initialize(hInstance, 640, 480);
	game.Run();
	game.Uninitialize();
	return 0;
}
