#pragma once

struct cpu_fsm_base : public cpu_object
{
public:
	friend cpu_engine;

public:
	struct _RELEASE
	{
		using ReleaseFn = void(*)(void*);
		ReleaseFn func;
		void* pInst;
	};

public:
	float preGlobalTotalTime;
	float postGlobalTotalTime;
	float totalTime;
	cpu_fsm_base* pParent;

protected:
	void* pPendingParam;
	inline static std::list<_RELEASE>* pReleaseItems = nullptr;

public:
	cpu_fsm_base();
	virtual ~cpu_fsm_base();

	static void Initialize();
	static void Uninitialize();
	static void Free();

	template <typename S>
	static S** StaticGetStateInstance() { static S* pInstance = nullptr; return &pInstance; }

	template <typename S>
	S* GetStateInstance();

	bool IsRunning();

protected:
	virtual void Update() = 0;
	virtual void UpdatePreGlobal() = 0;
	virtual void UpdatePostGlobal() = 0;
};

template <typename S>
S* cpu_fsm_base::GetStateInstance()
{
	S** ppInst = StaticGetStateInstance<S>();
	if ( *ppInst==nullptr )
	{
		*ppInst = new S;
		_RELEASE obj;
		obj.func = +[](void* s)
			{
				delete static_cast<S*>(s);
				*StaticGetStateInstance<S>() = nullptr;
			};
		obj.pInst = (void*)*ppInst;
		pReleaseItems->push_back(obj);
	}
	return *ppInst;
}

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

		void* self;
		EnterFn enter;
		ExecuteFn execute;
		ExitFn exit;
		int id;
		std::string name;

		_cpu_handle() { Clear(); }
		void Clear()
		{
			self = nullptr;
			enter = nullptr;
			execute = nullptr;
			exit = nullptr;
			id = -1;
			name = "";
		}
	};

private:
	T* pReceiver;
	_cpu_handle preGlobalState;
	_cpu_handle postGlobalState;
	std::unordered_map<std::type_index, _cpu_handle> states;
	_cpu_handle* pState;
	_cpu_handle* pPendingState;

public:
	cpu_fsm(T* pRcv)
	{
		pReceiver = pRcv;
		pState = nullptr;
		pPendingState = nullptr;
	}

	void Clear();

	template <typename S>
	void SetPreGlobal();

	template <typename S>
	void SetPostGlobal();

	template <typename S>
	void Add(cstr name = nullptr);

	std::string GetCurrentName();

	template <typename S>
	bool IsCurrentState();

	template <typename S>
	int GetStateID();

	template <typename S>
	void ToState(void* pParam = nullptr);

protected:
	template <typename S>
	_cpu_handle* GetState();

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
void cpu_fsm<T>::Clear()
{
	pState = nullptr;
	pPendingState = nullptr;
	preGlobalState.clear();
	postGlobalState.clear();
	states.clear();
}

template <typename T>
template <typename S>
void cpu_fsm<T>::SetPreGlobal()
{
	if ( preGlobalState.self )
		return;

	preGlobalState.self = GetStateInstance<S>();
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

	postGlobalState.self = GetStateInstance<S>();
	postGlobalState.enter = &Enter<S>;
	postGlobalState.execute = &Execute<S>;
	postGlobalState.exit = &Exit<S>;
}

template <typename T>
template <typename S>
void cpu_fsm<T>::Add(cstr name)
{
	_cpu_handle handle;
	handle.self = GetStateInstance<S>();
	handle.enter = &Enter<S>;
	handle.execute = &Execute<S>;
	handle.exit = &Exit<S>;
	handle.id = (int)states.size();
	if ( name )
		handle.name = name;
	std::type_index id = typeid(S);
	states.insert(std::make_pair(id, handle));
}

template <typename T>
std::string cpu_fsm<T>::GetCurrentName()
{
	if ( pState==nullptr )
		return "";
	return pState->name;
}

template <typename T>
template <typename S>
bool cpu_fsm<T>::IsCurrentState()
{
	_cpu_handle* pStateHandle = GetState<S>();
	if ( pStateHandle==nullptr )
		return false;
	return pStateHandle==pState;
}

template <typename T>
template <typename S>
int cpu_fsm<T>::GetStateID()
{
	_cpu_handle* pStateHandle = GetState<S>();
	if ( pStateHandle==nullptr )
		return -1;
	return pStateHandle->id;
}

template <typename T>
template <typename S>
void cpu_fsm<T>::ToState(void* pParam)
{
	_cpu_handle* pNewState = GetState<S>();
	if ( pNewState==nullptr )
		return;
	pPendingState = pNewState;
	pPendingParam = pParam;
}

template <typename T>
template <typename S>
cpu_fsm<T>::_cpu_handle* cpu_fsm<T>::GetState()
{
	auto it = states.find(typeid(S));
	if ( it==states.end() )
		return nullptr;
	return &it->second;
}

template <typename T>
void cpu_fsm<T>::Update()
{
	if ( IsRunning()==false )
		return;

	float dt = cpuTime.delta;
	totalTime += dt;
	
	if ( pPendingState!=pState )
	{
		_cpu_handle* pFrom = pState;
		_cpu_handle* pTo = pPendingState;
		void* pParam = pPendingParam;
	
		if ( pFrom )
		{
			if ( pFrom->exit )
				pFrom->exit(pFrom->self, *pReceiver, pTo ? pTo->id : -1);
		}

		pState = pTo;
	
		if ( pTo )
 		{
			totalTime = 0.0f;
			if ( pTo->enter )
				pTo->enter(pTo->self, *pReceiver, pFrom ? pFrom->id : -1, pParam);
		}
	}
	
	if ( pState )
	{
		if ( pState->execute )
			pState->execute(pState->self, *pReceiver);
	}
}

template <typename T>
void cpu_fsm<T>::UpdatePreGlobal()
{
	if ( IsRunning()==false )
		return;

	float dt = cpuTime.delta;
	preGlobalTotalTime += dt;
	if ( preGlobalState.execute )
		preGlobalState.execute(preGlobalState.self, *pReceiver);
}

template <typename T>
void cpu_fsm<T>::UpdatePostGlobal()
{
	if ( IsRunning()==false )
		return;

	float dt = cpuTime.delta;
	postGlobalTotalTime += dt;
	if ( postGlobalState.execute )
		postGlobalState.execute(postGlobalState.self, *pReceiver);
}
