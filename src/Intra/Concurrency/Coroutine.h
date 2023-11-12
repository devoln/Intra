#pragma once

#include <Intra/Core.h>


#if !defined(_MSC_VER) && !defined(__clang__)
// GCC gives ICE without this header
#include <coroutine>
#endif

namespace Intra { INTRA_BEGIN

struct Coroutine;

// Attempt to implement coroutines without #include <coroutine> and without conflicting with this header.
// Tested with MSVC standard library and libstdc++.
// Currently supported:
// 1. MSVC 2019+
// 2. GCC 10.1+ works only with #include <coroutine> and -fcoroutines, otherwise ICE
// 3. Clang 11+ (previous versions give ICE)
// TODO: test with libc++
namespace std {
#if !defined(_MSC_VER)
inline namespace __n4861 { // libstdc++; libc++ may work differently
#endif
template<class> struct coroutine_handle;
template<class Ret, class...> struct coroutine_traits;
#if !defined(_MSC_VER)
namespace experimental {
template<typename... Ts> struct coroutine_traits: std::coroutine_traits<Ts...> {};
template<typename... Ts> struct coroutine_handle: std::coroutine_handle<Ts...> {};
}
}
#endif

template<class Ret> requires CConstructible<Ret, Coroutine>
struct coroutine_traits<Ret> {using promise_type = typename Ret::promise_type;};
}

struct Coroutine
{
    constexpr Coroutine() noexcept = default;
    constexpr Coroutine(decltype(nullptr)) noexcept {}
    template<typename T> constexpr Coroutine(const std::coroutine_handle<T>& c) noexcept: mPtr(reinterpret_cast<void* const&>(c)) {}

    [[nodiscard]] static Coroutine FromPromise(auto& promise) noexcept
	{
        const auto promisePtr = const_cast<void*>(static_cast<const volatile void*>(__builtin_addressof(promise)));
        const auto framePtr = __builtin_coro_promise(promisePtr, 0, true);
        Coroutine res;
        res.mPtr = framePtr;
        return res;
    }

    Coroutine& operator=(decltype(nullptr)) noexcept {mPtr = nullptr; return *this;}

    [[nodiscard]] constexpr void* Address() const noexcept {return mPtr;}

    [[nodiscard]] static constexpr Coroutine FromAddress(void* addr) noexcept
	{
        Coroutine res;
        res.mPtr = addr;
        return res;
    }

    constexpr explicit operator bool() const noexcept {return mPtr != nullptr;}

	[[nodiscard]] bool Done() const noexcept {return __builtin_coro_done(mPtr);}
    void operator()() const {__builtin_coro_resume(mPtr);}
    void Destroy() const noexcept {__builtin_coro_destroy(mPtr);}
	[[nodiscard]] void* PromisePtr() const noexcept {return __builtin_coro_promise(mPtr, 0, false);}

	explicit operator const std::coroutine_handle<void>&() const {return reinterpret_cast<const std::coroutine_handle<void>&>(*this);}

private:
    void* mPtr = nullptr;
};

namespace std {
template<class P> requires CConstructible<decltype(P().get_return_object()), Coroutine>
struct coroutine_handle<P> {
    [[nodiscard]] INTRA_FORCEINLINE static constexpr Coroutine from_address(void* addr) noexcept {return Coroutine::FromAddress(addr);}
};
}

template<bool B> struct Suspend
{
	[[nodiscard]] INTRA_FORCEINLINE constexpr bool await_ready() const noexcept {return !B;}
	INTRA_FORCEINLINE constexpr void await_suspend(Coroutine c) const noexcept {}
	INTRA_FORCEINLINE constexpr void await_resume() const noexcept {}
};

template<typename T> class Generator
{
public:
	struct promise_type
	{
		T Current = 0;

		INTRA_FORCEINLINE Generator get_return_object() {return Coroutine::FromPromise(*this);}
		INTRA_FORCEINLINE void return_void() {}
		INTRA_FORCEINLINE Suspend<true> initial_suspend() {return {};}
		INTRA_FORCEINLINE Suspend<true> final_suspend() noexcept {return {};}
		INTRA_FORCEINLINE Suspend<true> yield_value(int value) {Current = value; return {};}
		INTRA_FORCEINLINE void unhandled_exception() {}
	};

	INTRA_FORCEINLINE Generator(Coroutine handle): mHandle(handle) {}
	INTRA_FORCEINLINE ~Generator() {mHandle.Destroy();}
	INTRA_FORCEINLINE promise_type& Promise() {return *static_cast<promise_type*>(Handle.PromisePtr());}
	
private:
	Coroutine mHandle;
};

class Task
{
public:
	struct promise_type
	{
		INTRA_FORCEINLINE Task get_return_object() {return Coroutine::FromPromise(*this);}
		INTRA_FORCEINLINE void return_void() {}
		INTRA_FORCEINLINE Suspend<true> initial_suspend() {return {};}
		INTRA_FORCEINLINE Suspend<true> final_suspend() noexcept {return {};}
		INTRA_FORCEINLINE void unhandled_exception() {}
	};

	INTRA_FORCEINLINE Task(Coroutine handle): mHandle(handle) {}
	INTRA_FORCEINLINE ~Task() {mHandle.Destroy();}
	INTRA_FORCEINLINE promise_type& Promise() {return *static_cast<promise_type*>(mHandle.PromisePtr());}
private:
	Coroutine mHandle;
};

} INTRA_END
