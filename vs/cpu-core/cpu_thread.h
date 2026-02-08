#pragma once

class cpu_thread
{
public:
	using _METHOD = std::function<void()>;

public:
	cpu_thread();
	virtual ~cpu_thread();

	bool Run();
	bool Run(const _METHOD& callback);
	void Wait();
	void QuitAsap();

	void SetCallback(const _METHOD& callback) { m_callback = callback; }
	virtual void OnCallback() {}

	ui32 GetParentID() const { return m_idThreadParent; }
	ui32 GetID() const { return m_idThread; }
	bool IsRunning() const { return m_idThread!=0; }

protected:
	static ui32 WINAPI ThreadProc(void* pParam);

protected:
	HANDLE m_hThread;
	ui32 m_idThread;
	ui32 m_idThreadParent;
	_METHOD m_callback;
	bool m_quitAsap;
};
