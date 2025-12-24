#pragma once

struct cpu_fsm_base : public cpu_object
{
	friend cpu_engine;

	int state;
	float globalTotalTime;
	float totalTime;

protected:
	int pending;

public:
	cpu_fsm_base();
	virtual ~cpu_fsm_base();

	void ToState(int to);

protected:
	virtual void Update(float dt) = 0;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct cpu_fsm : public cpu_fsm_base
{
private:
	struct _cpu_handle
	{
		using EnterFn = void(*)(void* self, T& cur, int from);
		using ExecuteFn = void(*)(void* self, T& cur);
		using ExitFn = void(*)(void* self, T& cur, int to);

		void* self = nullptr;
		EnterFn enter = nullptr;
		ExecuteFn execute = nullptr;
		ExitFn exit = nullptr;
	};

private:
	T* pReceiver;
	_cpu_handle globalState;
	std::vector<_cpu_handle> states;

public:
	cpu_fsm(T* pRcv)
	{
		pReceiver = pRcv;
	}

	template <typename S>
	void SetGlobal();

	template <typename S>
	void Add();

protected:
	template<typename S>
	static void Enter(void* self, T& cur, int from)
	{
		static_cast<S*>(self)->OnEnter(cur, from);
	}

	template<typename S>
	static void Execute(void* self, T& cur)
	{
		static_cast<S*>(self)->OnExecute(cur);
	}

	template<typename S>
	static void Exit(void* self, T& cur, int to)
	{
		static_cast<S*>(self)->OnExit(cur, to);
	}

	void Update(float dt) override;
};

template <typename T>
template <typename S>
void cpu_fsm<T>::SetGlobal()
{
	if ( globalState.self )
		return;

	static S state;
	globalState.self = &state;
	globalState.enter = &Enter<S>;
	globalState.execute = &Execute<S>;
	globalState.exit = &Exit<S>;
}

template <typename T>
template <typename S>
void cpu_fsm<T>::Add()
{
	static S state;
	_cpu_handle handle;
	handle.self = &state;
	handle.enter = &Enter<S>;
	handle.execute = &Execute<S>;
	handle.exit = &Exit<S>;

	int id = (int)states.size();
	if ( CPU_ID(S)==-1 )
		CPU_ID(S) = id;

	states.push_back(handle);
}

template <typename T>
void cpu_fsm<T>::Update(float dt)
{
	globalTotalTime += dt;
	totalTime += dt;
	
	if ( pending!=state )
	{
		int from = state;
		int to = pending;
	
		if ( from!=-1 )
		{
			_cpu_handle& handle = states[from];
			if ( handle.exit )
				handle.exit(handle.self, *pReceiver, to);
		}

		state = to;
	
		if ( to!=-1 )
 		{
			totalTime = 0.0f;
			_cpu_handle& handle = states[to];
			if ( handle.enter )
				handle.enter(handle.self, *pReceiver, from);
		}
	}
	
	if ( state!=-1 )
	{
		_cpu_handle& handle = states[state];
		if ( handle.execute )
			handle.execute(handle.self, *pReceiver);
	}
	
	if ( globalState.execute )
		globalState.execute(globalState.self, *pReceiver);
}
