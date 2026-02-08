#pragma once

template <typename T>
struct cpu_atomic
{
public:
	cpu_atomic() {}

	void operator=(const T& v) { Store(v); }
	void Store(T v) { return val.store(v, std::memory_order_relaxed); }
	T Add(T v) { return val.fetch_add(v, std::memory_order_relaxed); }

private:
	std::atomic<T> val;
};
