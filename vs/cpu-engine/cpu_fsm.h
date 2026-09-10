#pragma once

struct cpu_fsm_base : public cpu_object
{
public:
	friend cpu_engine;

public:
	int state;
	float preGlobalTotalTime;
	float postGlobalTotalTime;
	float totalTime;

protected:
	int pending;
	void* pPendingParam;

public:
	cpu_fsm_base();
	virtual ~cpu_fsm_base();

	void ToState(int to, void* pParam = nullptr);

protected:
	virtual void Update() = 0;
	virtual void UpdatePreGlobal() = 0;
	virtual void UpdatePostGlobal() = 0;
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
		using EnterFn = void(*)(void* self, T& cur, int from, void* pParam);
		using ExecuteFn = void(*)(void* self, T& cur);
		using ExitFn = void(*)(void* self, T& cur, int to);

		void* self = nullptr;
		EnterFn enter = nullptr;
		ExecuteFn execute = nullptr;
		ExitFn exit = nullptr;
	};

private:
	T* pReceiver;
	_cpu_handle preGlobalState;
	_cpu_handle postGlobalState;
	std::vector<_cpu_handle> states;

public:
	cpu_fsm(T* pRcv)
	{
		pReceiver = pRcv;
	}

	template <typename S>
	void SetPreGlobal();

	template <typename S>
	void SetPostGlobal();

	template <typename S>
	void Add();

protected:
	template<typename S>
	static void Enter(void* self, T& cur, int from, void* pParam)
	{
		static_cast<S*>(self)->OnEnter(cur, from, pParam);
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

	void Update() override;
	void UpdatePreGlobal() override;
	void UpdatePostGlobal() override;
};

template <typename T>
template <typename S>
void cpu_fsm<T>::SetPreGlobal()
{
	if ( preGlobalState.self )
		return;

	static S state;
	preGlobalState.self = &state;
	preGlobalState.enter = &Enter<S>;
	preGlobalState.execute = &Execute<S>;
	preGlobalState.exit = &Exit<S>;
}

template <typename T>
template <typename S>
void cpu_fsm<T>::SetPostGlobal()
{
	if ( postGlobalState.self )
		return;

	static S state;
	postGlobalState.self = &state;
	postGlobalState.enter = &Enter<S>;
	postGlobalState.execute = &Execute<S>;
	postGlobalState.exit = &Exit<S>;
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

	assert( CPU_ID(S)==(int)states.size()-1 );
}

template <typename T>
void cpu_fsm<T>::Update()
{
	float dt = cpuTime.delta;
	totalTime += dt;
	
	if ( pending!=state )
	{
		int from = state;
		int to = pending;
		void* pParam = pPendingParam;
	
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
				handle.enter(handle.self, *pReceiver, from, pParam);
		}
	}
	
	if ( state!=-1 )
	{
		_cpu_handle& handle = states[state];
		if ( handle.execute )
			handle.execute(handle.self, *pReceiver);
	}
}

template <typename T>
void cpu_fsm<T>::UpdatePreGlobal()
{
	float dt = cpuTime.delta;
	preGlobalTotalTime += dt;
	if ( preGlobalState.execute )
		preGlobalState.execute(preGlobalState.self, *pReceiver);
}

template <typename T>
void cpu_fsm<T>::UpdatePostGlobal()
{
	float dt = cpuTime.delta;
	postGlobalTotalTime += dt;
	if ( postGlobalState.execute )
		postGlobalState.execute(postGlobalState.self, *pReceiver);
}
