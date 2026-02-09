#pragma once

struct cpu_input
{
private:
	cpu_window* pWindow;
	std::vector<byte> mapActionsForVI[CPU_INPUT_ACTIONS];
	std::vector<byte> mapActionsForXI[CPU_INPUT_ACTIONS];

public:
	bool focus;
	cpu_vinput vi;	// virtual key input
	cpu_xinput xi;	// xinput

public:
	~cpu_input();
	static cpu_input& GetInstance();
	
	void Initialize();
	void Uninitialize();
	void SetWindow(cpu_window* pWnd);
	void Reset();

	template<typename... IDS>
	void MapActionForVI(int actionIndex, IDS... ids);
	template<typename... IDS>
	void MapActionForXI(int actionIndex, IDS... ids);
	bool IsAction(int index = 0);
	bool IsActionPressed(int index = 0);
	bool IsActionReleased(int index = 0);

	bool IsBackPressed();

	bool IsLeft();
	bool IsLeftPressed();
	bool IsLeftReleased();
	bool IsRight();
	bool IsRightPressed();
	bool IsRightReleased();
	bool IsUp();
	bool IsUpPressed();
	bool IsUpReleased();
	bool IsDown();
	bool IsDownPressed();
	bool IsDownReleased();

	void Update();

private:
	cpu_input();
};

template<typename... IDS>
void cpu_input::MapActionForVI(int actionIndex, IDS... ids)
{
	mapActionsForVI[actionIndex].clear();
	int a[] = {ids...};
	for ( int id : a )
		mapActionsForVI[actionIndex].push_back(id);
}

template<typename... IDS>
void cpu_input::MapActionForXI(int actionIndex, IDS... ids)
{
	mapActionsForXI[actionIndex].clear();
	int a[] = {ids...};
	for ( int id : a )
		mapActionsForXI[actionIndex].push_back(id);
}
