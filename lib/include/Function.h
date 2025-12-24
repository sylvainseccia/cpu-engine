#pragma once

template <typename Signature>
struct cpu_function;

template <typename R, typename... Args>
struct cpu_function<R(Args...)>
{
public:
	cpu_function() = default;

	template <typename T>
	cpu_function(T* instance, R (T::*method)(Args...)) { Set(instance, method); }

	template <typename T>
	cpu_function(const T* instance, R (T::*method)(Args...) const) { Set(instance, method); }

	cpu_function(R (*func)(Args...)) { Set(func); }

	R Call(Args... args) const
	{
		return stub(pObj, storage, std::forward<Args>(args)...);
	}

	explicit operator bool() const noexcept { return stub != nullptr; }

	void Clear() noexcept
	{
		pObj  = nullptr;
		stub = nullptr;
	}

	template <typename T>
	void Set(T* instance, R (T::*method)(Args...))
	{
		static_assert(std::is_pointer_v<T*>);
		using M = R (T::*)(Args...);
		StoreMember<M>(method);
		pObj  = instance;
		stub = &MemberStub<T, M>;
	}

	template <typename T>
	void Set(const T* instance, R (T::*method)(Args...) const)
	{
		using M = R (T::*)(Args...) const;
		StoreMember<M>(method);
		pObj  = const_cast<T*>(instance);
		stub = &ConstMemberStub<T, M>;
	}

	void Set(R (*func)(Args...))
	{
		using F = R (*)(Args...);
		StoreMember<F>(func);
		pObj  = nullptr;
		stub = &FreeFuncStub<F>;
	}

private:
	static constexpr std::size_t STORAGE_SIZE = 32;
	alignas(std::max_align_t) unsigned char storage[STORAGE_SIZE]{};

	void* pObj = nullptr;

	using Stub = R (*)(void* obj, const unsigned char* storage, Args&&...);
	Stub stub = nullptr;

	template <typename M>
	void StoreMember(const M& m)
	{
		static_assert(std::is_trivially_copyable_v<M>, "Member/function pointer must be trivially copyable");
		static_assert(sizeof(M) <= STORAGE_SIZE, "Increase StorageSize to fit this member pointer type on your ABI/compiler");
		std::memcpy(storage, &m, sizeof(M));
	}

	template <typename M>
	static M LoadMember(const unsigned char* storage)
	{
		M m{};
		std::memcpy(&m, storage, sizeof(M));
		return m;
	}

	template <typename T, typename M>
	static R MemberStub(void* obj, const unsigned char* storage, Args&&... args)
	{
		auto* self = static_cast<T*>(obj);
		const M m = LoadMember<M>(storage);
		return (self->*m)(std::forward<Args>(args)...);
	}

	template <typename T, typename M>
	static R ConstMemberStub(void* obj, const unsigned char* storage, Args&&... args)
	{
		auto* self = static_cast<const T*>(obj);
		const M m = LoadMember<M>(storage);
		return (self->*m)(std::forward<Args>(args)...);
	}

	template <typename F>
	static R FreeFuncStub(void*, const unsigned char* storage, Args&&... args)
	{
		const F f = LoadMember<F>(storage);
		return f(std::forward<Args>(args)...);
	}
};
