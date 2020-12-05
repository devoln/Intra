#pragma once

#include "Intra/Range/Concepts.h"
#include "Intra/Functional.h"
#include "Intra/Container/Array.h"
#include "Intra/Container/Decorators.h"

INTRA_BEGIN
INTRA_IGNORE_WARN_COPY_IMPLICITLY_DELETED
INTRA_IGNORE_WARN_ASSIGN_IMPLICITLY_DELETED
template<typename F> class Generate
{
	[[no_unique_address]] F mFunc;
	TResultOf<F> mFirst;
public:
	constexpr bool IsAnyInstanceInfinite = true;

	template<CAsCallable F1> constexpr Generate(F1&& f): mFunc(ForwardAsFunc<F1>(f)), mFirst(mFunc()) {}
	[[nodiscard]] constexpr bool Empty() const {return false;}
	[[nodiscard]] constexpr const auto& First() const {return mFirst;}
	constexpr void PopFirst() {mFirst = mFunc();}
};

template<typename T> struct Repeat
{
	static constexpr bool IsAnyInstanceInfinite = true;
	T Value{};
	Repeat() = default;
	constexpr Repeat(T val): Value(INTRA_MOVE(val)) {}
	[[nodiscard]] constexpr bool Empty() const noexcept {return false;}
	[[nodiscard]] constexpr const T& First() const noexcept {return Value;}
	constexpr void PopFirst() noexcept {}
	constexpr index_t PopFirstCount(ClampedSize numElementsToPop) noexcept {return index_t(numElementsToPop);}
	[[nodiscard]] constexpr const T& operator[](Index) const noexcept {return Value;}
};

template<typename F, typename T, size_t N> class Recurrence
{
	[[no_unique_address]] TSelect<size_t, EmptyType, (N > 2)> mIndex = 0;
	[[no_unique_address]] F mFunc;
	Array<T, N> mState;
public:
	static constexpr bool IsAnyInstanceInfinite = true;

	template<typename... Ts, CAsCallable<Ts...> F1>
	constexpr Recurrence(F1&& function, Ts&&... initialSequence):
		mFunc(ForwardAsFunc<F1>(function)),
		mState{INTRA_FWD(initialSequence)...} {}

	[[nodiscard]] constexpr T First() const
	{
		if constexpr(N <= 2) return mState[0];
		else return mState[mIndex];
	}
	constexpr void PopFirst()
	{
		if constexpr(N == 1) mState[0] = mFunc(mState[0]);
		else if constexpr(N == 2)
		{
			mState[0] = mFunc(mState[0], mState[1]);
			Swap(mState[0], mState[1]);
		}
		else [&]<size_t... Is>(TIndexSeq<Is...>)
		{
			auto& nextVal = mState[mIndex++];
			if(mIndex == N) mIndex = 0;
			nextVal = mFunc(mState[mIndex], mState[(mIndex+(1+Is) - (mIndex >= N-(1+Is)? N: 0))...]);
		}();
	}
	[[nodiscard]] constexpr bool Empty() const {return false;}
};
template<typename F, typename... Ts>
Recurrence(F&&, Ts&&...) -> Recurrence<TFunctorOf<F&&>, TCommon<Ts...>, sizeof...(Ts)>;

template<typename F> class Sequence
{
	[[no_unique_address]] F mFunc;
public:
	constexpr bool IsAnyInstanceInfinite = true};

	Sequence() = default;
	template<CAsCallable<size_t> F1> constexpr RSequence(F1&& function, size_t offset = 0):
		mFunc(function), Offset(offset) {}

	[[nodiscard]] constexpr auto First() const {return mFunc(Offset);}
	INTRA_FORCEINLINE constexpr void PopFirst() noexcept {Offset++;}
	[[nodiscard]] constexpr bool Empty() const noexcept {return false;}
	[[nodiscard]] constexpr auto operator[](size_t index) const {return mFunc(Offset + index);}

	[[nodiscard]] constexpr index_t PopFirstCount(ClampedSize maxElementsToPop) noexcept
	{
		Offset += size_t(maxElementsToPop);
		return index_t(maxElementsToPop);
	}

	size_t Offset = 0;
};
template<typename F> Sequence(F&&) -> Sequence<TFunctorOf<F&&>>;

template<typename T> struct REmptyRange
{
	[[nodiscard]] constexpr bool Empty() const noexcept {return true;}
	[[nodiscard]] constexpr index_t Length() const noexcept {return 0;}
	[[nodiscard]] constexpr T First() const {INTRA_PRECONDITION(!Empty()); return T();}
	[[nodiscard]] constexpr T Last() const {INTRA_PRECONDITION(!Empty()); return T();}
	constexpr void PopFirst() const {INTRA_PRECONDITION(!Empty());}
	constexpr void PopLast() const {INTRA_PRECONDITION(!Empty());}
	constexpr T operator[](size_t) const {INTRA_PRECONDITION(!Empty()); return T();}
	[[nodiscard]] constexpr index_t PopFirstCount(ClampedSize) const noexcept {return 0;}
	constexpr T* Data() const noexcept {return nullptr;}
	constexpr REmptyRange Take(ClampedSize) {return *this;}
};
template<typename T> constexpr REmptyRange<T> EmptyRange;

constexpr struct {template<typename T> constexpr void Put(T&&) const {}} NullSink;

template<typename T, typename S = T> struct IotaInf
{
	static constexpr bool IsAnyInstanceInfinite = true;

	T Begin{0};
	S Step{1};

	explicit IotaInf() = default;
	explicit constexpr IotaInf(T begin, S step = 1): Begin(begin), Step(step) {}

	[[nodiscard]] constexpr T First() const {return Begin;}
	constexpr void PopFirst() {Begin = T(Begin+Step);}
	[[nodiscard]] constexpr bool Empty() const {return Step == 0;}
	[[nodiscard]] constexpr auto operator[](Index index) const {return Begin + Step*index_t(index);}

	[[nodiscard]] constexpr index_t PopFirstCount(ClampedSize elementsToPop) const
	{
		Begin = T(operator[](size_t(elementsToPop)));
		return index_t(elementsToPop);
	}
};
IotaInf() -> IotaInf<int>;
template<typename T> IotaInf(T) -> IotaInf<T, T>;
template<typename T, typename S> IotaInf(T, S) -> IotaInf<decltype(T() + S()), S>;

constexpr auto Iota = []<typename T>(T begin, T end, auto step = 1) {
	return IotaInf(begin, step)|Take((end - begin + step - 1) / step);
};

#if INTRA_CONSTEXPR_TEST
static_assert(CRandomAccessRange<IotaInf<int, int>>);
static_assert(CRandomAccessRange<decltype(Iota(1, 2, 3))>);
static_assert(!CBidirectionalRange<IotaInf<int, int>>);
static_assert(!CFiniteRange<IotaInf<int, int>>);
#endif

/** Range that counts all elements that are put into it.
  Useful for example for counting result string length before conversion to it to avoid reallocation.
*/
template<typename T = int, typename CounterT = index_t> struct RCounter
{
	static constexpr bool IsAnyInstanceInfinite = true;

	[[nodiscard]] constexpr bool Empty() const noexcept {return false;}
	[[nodiscard]] constexpr T First() const noexcept(noexcept(T())) {return T();}
	constexpr void Put(const T&) noexcept {Counter++;}
	constexpr void PopFirst() noexcept {Counter++;}
	[[nodiscard]] constexpr index_t PopFirstCount(ClampedSize elementsToPop) const
	{
		Counter += CounterT(elementsToPop);
		return index_t(elementsToPop);
	}

	CounterT Counter = 0;
};
template<typename T = int, typename CounterT = index_t> struct RCeilCounter: RCounter<T, CounterT> {};
template<typename T> concept CCounter = CInstanceOfTemplate<TRemoveConstRef<T>, RCounter>;
template<typename T> concept CCeilCounter = CInstanceOfTemplate<TRemoveConstRef<T>, RCeilCounter>;

INTRA_END
