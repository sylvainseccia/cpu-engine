#pragma once

namespace cpu
{

template <typename E, typename A>
void Run(int width, int height, bool fullscreen = false, bool amigaStyle = false)
{
#ifdef _DEBUG
	_CrtMemState memStateInit;
	_CrtMemCheckpoint(&memStateInit);
#endif

	cpu::Initialize();
	
	MSG msg;
	while ( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) );
	E* pEngine = new E;
	if ( pEngine->Create(width, height, fullscreen, amigaStyle) )
	{
		A* pApp = new A;
		pEngine->Run();
		delete pApp;
	}
	delete pEngine;
	
	cpu::Uninitialize();

#ifdef _DEBUG
	_CrtMemState memState, memStateDiff;
	_CrtMemCheckpoint(&memState);
	if ( _CrtMemDifference(&memStateDiff, &memStateInit, &memState) )
		MessageBoxA(nullptr, "Memory leaks", "ALERT", 0);
	_CrtDumpMemoryLeaks();
#endif
}

}
