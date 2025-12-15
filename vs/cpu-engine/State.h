#pragma once

struct cpu_state_base
{
	virtual void OnEnter(void* user, int from) = 0;
	virtual void OnExecute(void* user) = 0;
	virtual void OnExit(void* user, int to) = 0;
};

template <typename T>
struct cpu_state : public cpu_state_base
{
	using EnterFn = void(*)(T& instance, int from);
	using ExecuteFn = void(*)(T& instance);
	using ExitFn = void(*)(T& instance, int to);

	EnterFn onEnter = nullptr;
	ExecuteFn onExecute = nullptr;
	ExitFn onExit = nullptr;

	void OnEnter(void* user, int from) override { onEnter(*(T*)user, from); }
	void OnExecute(void* user) override { onExecute(*(T*)user); }
	void OnExit(void* user, int to) override { onExit(*(T*)user, to); }
};

struct cpu_fsm_base
{
	bool dead;
	int index;
	int sortedIndex;
	int state;
	float globalTime;
	float time;

protected:
	void* pReceiver;
	cpu_state_base* pGlobal;
	std::vector<cpu_state_base*> states;
	int pending;

public:
	cpu_fsm_base()
	{
		dead = false;
		index = -1;
		sortedIndex = -1;

		pReceiver = nullptr;
		state = -1;
		pending = -1;
		globalTime = 0.0f;
		time = 0.0f;
		pGlobal = nullptr;
	}

	virtual ~cpu_fsm_base()
	{
		for ( auto it=states.begin() ; it!=states.end() ; ++it )
			delete *it;
		delete pGlobal;
	}

	void ToState(int to)
	{
		pending = to;
	}

	void Update(float dt)
	{
		globalTime += dt;
		time += dt;

		if ( pending!=state )
		{
			int from = state;
			int to = pending;

			if ( state!=-1 )
				states[state]->OnExit(pReceiver, to);

			state = to;

			if ( state!=-1 )
			{
				time = 0.0f;
				states[state]->OnEnter(pReceiver, from);
			}
		}

		if ( state!=-1 )
			states[state]->OnExecute(pReceiver);

		if ( pGlobal )
			pGlobal->OnExecute(pReceiver);
	}
};

template <typename T>
struct cpu_fsm : public cpu_fsm_base
{
public:
	cpu_fsm(T* pRcv)
	{
		pReceiver = pRcv;
	}

	template <typename S>
	void SetGlobal()
	{
		if ( pGlobal )
			return;
		static_assert(std::is_base_of_v<cpu_state<T>, S>, "S is not derived from cpu_state<T>");
		S* pState = new S;
		pState->id = -1;
		pState->onExecute = &S::OnExecute;
		pGlobal = pState;
	}

	template <typename S>
	void Add()
	{
		static_assert(std::is_base_of_v<cpu_state<T>, S>, "S is not derived from cpu_state<T>");
		S* pState = new S;
		pState->id = (int)states.size();
		pState->onEnter = &S::OnEnter;
		pState->onExecute = &S::OnExecute;
		pState->onExit = &S::OnExit;
		states.push_back(pState);
	}
};
