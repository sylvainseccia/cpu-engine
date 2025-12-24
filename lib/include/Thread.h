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

	DWORD GetParentID() const { return m_idThreadParent; }
	DWORD GetID() const { return m_idThread; }
	bool IsRunning() const { return m_idThread!=0; }

protected:
	static DWORD WINAPI ThreadProc(void* pParam);

protected:
	HANDLE m_hThread;
	DWORD m_idThread;
	DWORD m_idThreadParent;
	_METHOD m_callback;
	bool m_quitAsap;
};
