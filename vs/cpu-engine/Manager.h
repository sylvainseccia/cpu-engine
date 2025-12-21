#pragma once

struct cpu_object
{
	int index;
	int sortedIndex;
	bool dead;

	cpu_object() = default;
	virtual ~cpu_object() = default;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
struct cpu_manager
{
	std::vector<T*> list;
	std::vector<T*> sortedList;
	int count = 0;
	std::vector<T*> bornList;
	int bornCount = 0;
	std::vector<T*> deadList;
	int deadCount = 0;

	cpu_manager() { static_assert(std::is_base_of_v<cpu_object, T>, "T must derive from cpu_object");}
	~cpu_manager() { Clear(); }

	void Clear();
	T* operator[](int index) { return list[index]; }
	T* Create();
	void Add(T* p);
	void Release(T* p);
	void Purge();
};

template<typename T>
void cpu_manager<T>::Clear()
{
	assert( bornCount==0 );
	assert( deadCount==0 );
	for ( int i=0 ; i<count ; i++ )
		delete list[i];
	list.clear();
	sortedList.clear();
	count = 0;
	bornList.clear();
	bornCount = 0;
	deadList.clear();
	deadCount = 0;
}

template<typename T>
T* cpu_manager<T>::Create()
{
	T* p = new T;
	if ( bornCount<bornList.size() )
		bornList[bornCount] = p;
	else
		bornList.push_back(p);
	bornCount++;
	return p;
}

template<typename T>
void cpu_manager<T>::Add(T* p)
{
	if ( bornCount<bornList.size() )
		bornList[bornCount] = p;
	else
		bornList.push_back(p);
	bornCount++;
}

template<typename T>
void cpu_manager<T>::Release(T* p)
{
	if ( p==nullptr || p->dead )
		return;
	p->dead = true;
	if ( deadCount<deadList.size() )
		deadList[deadCount] = p;
	else
		deadList.push_back(p);
	deadCount++;
}

template<typename T>
void cpu_manager<T>::Purge()
{
	// Born
	for ( int i=0 ; i<bornCount ; i++ )
	{
		T* p = bornList[i];
		if ( p->dead )
			continue;
		p->index = count;
		p->sortedIndex = p->index;
		if ( p->index<list.size() )
			list[p->index] = p;
		else
			list.push_back(p);
		if ( p->sortedIndex<sortedList.size() )
			sortedList[p->sortedIndex] = p;
		else
			sortedList.push_back(p);
		count++;
	}
	bornCount = 0;

	// Dead
	for ( int i=0 ; i<deadCount ; i++ )
	{
		T* p = deadList[i];
		if ( p->index==-1 )
		{
			delete deadList[i];
			deadList[i] = nullptr;
			continue;
		}
		if ( p->index<count-1 )
		{
			list[p->index] = list[count-1];
			list[p->index]->index = p->index;
		}
		if ( p->sortedIndex<count-1 )
		{
			sortedList[p->sortedIndex] = sortedList[count-1];
			sortedList[p->sortedIndex]->sortedIndex = p->sortedIndex;
		}
		delete deadList[i];
		deadList[i] = nullptr;
		count--;
	}
	deadCount = 0;
}
