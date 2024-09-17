#pragma once

#include <Intra/Core.h>
#include <Intra/Numeric/Integral.h>
#include <Intra/Range.h>

namespace Intra { INTRA_BEGIN
INTRA_DEFINE_FUNCTOR(AddLast)(auto&& v)
{
	return [v = INTRA_FWD(v)]<CList C>(C& container) requires z_D::CHasMethodAddLast<C> || z_D::CHasMethod_push_back<C> {
		if constexpr(z_D::CHasMethodAddLast<C>) container.AddLast(v);
		else container.push_back(v);
	};
};

INTRA_DEFINE_FUNCTOR(AddLastRef)(auto& v)
{
	return [&]<CList C>(C& container) requires z_D::CHasMethodAddLast<C> || z_D::CHasMethod_push_back<C> {
		if constexpr(z_D::CHasMethodAddLast<C>) container.AddLast(v);
		else container.push_back(v);
	};
};

INTRA_DEFINE_FUNCTOR(AddLastOnceRef)(auto&& v)
{
	return [&]<class C>(C& container) requires z_D::CHasMethodAddLast<C> || z_D::CHasMethod_push_back<C> {
		if constexpr(z_D::CHasMethodAddLast<C>) container.AddLast(INTRA_FWD(v));
		else container.push_back(INTRA_FWD(v));
	};
};

INTRA_DEFINE_FUNCTOR(AddFirst)(auto&& v)
{
	return [v = INTRA_FWD(v)]<class C>(C& container) requires z_D::CHasMethodAddFirst<C> || z_D::CHasMethod_push_front<C> {
		if constexpr(z_D::CHasMethodAddFirst<C>) container.AddFirst(v);
		else container.push_front(v);
	};
};

INTRA_DEFINE_FUNCTOR(AddFirstRef)(auto& v)
{
	return [&]<class C>(C& container) requires z_D::CHasMethodAddFirst<C> || z_D::CHasMethod_push_front<C> {
		if constexpr(z_D::CHasMethodAddFirst<C>) container.AddFirst(v);
		else container.push_front(v);
	};
};

INTRA_DEFINE_FUNCTOR(AddFirstOnceRef)(auto&& v)
{
	return [&]<class C>(C& container) requires z_D::CHasMethodAddFirst<C> || z_D::CHasMethod_push_front<C> {
		if constexpr(z_D::CHasMethodAddFirst<C>) container.AddFirst(INTRA_FWD(v));
		else container.push_front(INTRA_FWD(v));
	};
};

INTRA_DEFINE_FUNCTOR(SetLength)(Size newLength)
{
	return [newLength]<class C>(C& container) requires z_D::CHasMethodSetLength<C> || z_D::CHasMethod_resize<C> {
		if constexpr(z_D::CHasMethodSetLength<C>) container.SetLength(newLength);
		else container.resize(size_t(newLength));
	};
};

INTRA_DEFINE_FUNCTOR(SetLengthRaw)(TUnsafe, Size newLength)
{
	return [newLength]<class C>(C& container) requires z_D::CHasMethodSetLengthRaw<C> || z_D::CHasMethodSetLength<C> || z_D::CHasMethod_resize<C> {
		if constexpr(z_D::CHasMethodSetLengthRaw<C>) container.SetLengthRaw(Unsafe, newLength);
		else SetLength(container, newLength);
	};
};

INTRA_DEFINE_FUNCTOR(Reserve)(Size capacity)
{
	return [capacity]<class C>(C& container) requires z_D::CHasMethodReserve<C> || z_D::CHasMethod_reserve<C> {
		if constexpr(z_D::CHasMethodReserve<C>) container.Reserve(size_t(capacity));
		else container.reserve(size_t(capacity));
	};
};

template<typename L, typename R> requires
	(CGrowingList<L> && CConsumableList<R>) ||
	(CGrowingList<R> && CConsumableList<L>)
[[nodiscard]] constexpr bool operator==(L&& lhs, R&& rhs)
{
	return INTRA_FWD(lhs)|Equals(INTRA_FWD(rhs));
}

template<typename L, typename R> requires
	(CGrowingList<L> && CConsumableList<R>) ||
	(CGrowingList<R> && CConsumableList<L>)
[[nodiscard]] constexpr bool operator!=(L&& lhs, R&& rhs)
{
	return !operator==(INTRA_FWD(lhs), INTRA_FWD(rhs));
}



INTRA_IGNORE_WARN_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED
INTRA_IGNORE_WARN_DEFAULT_CTOR_IMPLICITLY_DELETED
template<typename C> struct LastAppender
{
	C& Dst;

	template<CSameUnqualRef<TListValue<C>> T> constexpr auto Put(T&& v) -> decltype(Dst.push_back(INTRA_FWD(v))) {Dst.push_back(INTRA_FWD(v));}
	template<CSameUnqualRef<TListValue<C>> T> constexpr auto Put(T&& v) -> decltype(Dst.AddLast(INTRA_FWD(v))) {Dst.AddLast(INTRA_FWD(v));}

	template<typename T> constexpr auto Put(T&& v) -> decltype(Dst.emplace_back(INTRA_FWD(v))) {Dst.emplace_back(INTRA_FWD(v));}
};

template<typename C> struct FirstAppender
{
	C& Dst;

	constexpr void Put(TRangeValue<C>&& v) {Dst.push_front(Move(v));}
	constexpr void Put(const TRangeValue<C>& v) {Dst.push_front(v);}

	template<typename T> requires z_D::CHasMethod_emplace_front<T&&>
	constexpr void Put(T&& v) {Dst.emplace_front(INTRA_FWD(v));}
};


template<typename SC, typename R> requires
	(z_D::CHasMethodSetLengthRaw<SC> || z_D::CHasMethodSetLength<SC> || z_D::CHasMethod_resize<SC> ||
		z_D::CHasMethodAddLast<SC> || z_D::CHasMethod_push_back<SC>) &&
	CConsumableListOf<R, TRangeValue<SC>>
SC& operator+=(SC& lhs, R&& rhs)
{
	if constexpr(!z_D::CHasMethodSetLengthRaw<SC> && !z_D::CHasMethod_resize<SC>)
	{
		LastAppender{lhs} << rhs;
	}
	else if constexpr(CSameArrays<SC, R> && CTriviallyCopyable<TArrayListValue<SC>>)
	{
		const auto oldLen = Length(lhs);
		SetLengthRaw(Unsafe, lhs, oldLen + Length(rhs));
		MemoryCopy(Unsafe, Data(lhs) + oldLen, Data(rhs), Length(rhs));
	}
	else
	{
		auto r = RangeOf(INTRA_FWD(rhs));
		const auto oldLen = Length(lhs);
		SetLengthRaw(Unsafe, lhs, oldLen + Count(r));
		r|CopyTo(lhs|Drop(oldLen));
	}
	return lhs;
}

} INTRA_END
