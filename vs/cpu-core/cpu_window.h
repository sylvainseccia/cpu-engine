#pragma once

class cpu_window
{
public:
	cpu_window();
	virtual ~cpu_window();

	static void Initialize();
	static void Uninitialize();

	auto* GetCallback() { return &m_callback; }

	bool Create(int width, int height, bool fullscreen = false);
	void Destroy();
	void Quit();

	HWND GetHWND() { return m_hWnd; }
	int GetWidth() { return m_width; }
	int GetHeight() { return m_height; }
	bool HasFocus() { return m_hWnd && GetForegroundWindow()==m_hWnd; }

	void Show();
	bool Update();

private:
	void Fix();

	static LRESULT WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT OnWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	cpu_function<void(UINT, WPARAM, LPARAM)> m_callback;
	inline static WNDCLASSA s_wc = {};
	HWND m_hWnd;
	int m_width;
	int m_height;
};
