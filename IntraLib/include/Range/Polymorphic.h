#pragma once

#include "Memory/SmartRef.h"
#include "Range/Operations.h"
#include "Algo/Op.h"
#include "Algo/Mutation/Copy.h"
#include "Range/Concepts.h"
#include "Range/ArrayRange.h"
#include "Range/StringView.h"

namespace Intra { namespace Range {

INTRA_WARNING_PUSH
INTRA_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED
INTRA_WARNING_DISABLE_SIGN_CONVERSION

#ifdef _MSC_VER
#define TEMPLATE //VS не хочет компилировать конструкцию typename ...<...>::template ...<...>, но без template компилирует
#else
#define TEMPLATE template //clang требует слово template в typename ...<...>::template ...<...>
#endif

namespace D {

template<typename R, typename PolymorphicRange> using EnableCondition =
Meta::EnableIf<
	ValueTypeIsConvertible<AsRangeResult<R>, ValueTypeOf<PolymorphicRange>>::_ &&
	IsInputRange<AsRangeResult<R>>::_ &&
	(IsFiniteRange<AsRangeResult<R>>::_ || !IsFiniteRange<PolymorphicRange>::_) &&
	!Meta::TypeEqualsIgnoreCVRef<R, PolymorphicRange>::_
>;

}

template<typename T> struct InputRange
{
protected:
	typedef Meta::RemoveReference<T> value_type;
	struct Interface
	{
		virtual ~Interface() {}

		virtual bool Empty() const = 0;
		virtual T First() const = 0;
		virtual void PopFirst() = 0;
		virtual T GetNext() = 0;

		virtual void PopFirstN(size_t count) = 0;
		virtual void PopFirstExactly(size_t count) = 0;

		virtual ArrayRange<Meta::RemoveConst<value_type>> CopyAdvanceToAdvance(
			ArrayRange<Meta::RemoveConst<value_type>>& dst, size_t n = ~size_t(0)) = 0;
	};

	template<typename R, typename PARENT> struct ImplFiller: PARENT
	{
		R OriginalRange;

		template<typename A> ImplFiller(A&& range):
			OriginalRange(Meta::Forward<A>(range)) {}

		bool Empty() const override
		{return OriginalRange.Empty();}

		T First() const override
		{return OriginalRange.First();}

		void PopFirst() override
		{OriginalRange.PopFirst();}

		T GetNext() override
		{
			auto&& result = OriginalRange.First();
			OriginalRange.PopFirst();
			return result;
		}

		void PopFirstN(size_t count) override
		{Range::PopFirstN(OriginalRange, count);}
		void PopFirstExactly(size_t count) override
		{Range::PopFirstExactly(OriginalRange, count);}

		ArrayRange<Meta::RemoveConst<value_type>> CopyAdvanceToAdvance(
			ArrayRange<Meta::RemoveConst<value_type>>& dst, size_t n=~size_t(0)) override
		{
			auto beg = dst.Begin;
			Algo::CopyAdvanceToAdvance(OriginalRange, Op::Min(n, dst.Length()), dst);
			return {beg, dst.Begin};
		}
	};

	template<typename R, typename PARENT> using FullImplFiller = ImplFiller<R, PARENT>;

	template<typename R> struct WrapperImpl: FullImplFiller<R, Interface>
	{
		template<typename A> WrapperImpl(A&& range):
			FullImplFiller<R, Interface>(Meta::Forward<A>(range)) {}
	};

private:
	template<typename R> forceinline static Interface* wrap(R&& range)
	{
		typedef Meta::RemoveConstRef<AsRangeResult<R&&>> Range;
		return new WrapperImpl<Range>(AsRange(Meta::Forward<R>(range)));
	}

public:
	forceinline InputRange(null_t=null): mInterface(null) {}

	forceinline InputRange(InputRange&& rhs):
		mInterface(Meta::Move(rhs.mInterface)) {}

	forceinline InputRange& operator=(InputRange&& rhs)
	{
		mInterface = Meta::Move(rhs.mInterface);
		return *this;
	}

	InputRange(const InputRange& rhs) = delete;

	InputRange& operator=(const InputRange& rhs) = delete;

	template<typename R, typename = D::EnableCondition<R, InputRange>>
	forceinline InputRange(R range):
		mInterface(wrap(range)) {}

	template<typename R, typename = D::EnableCondition<R, InputRange>>
	forceinline InputRange& operator=(R&& range)
	{
		mInterface = wrap(Meta::Forward<R>(range));
		return *this;
	}

	template<size_t N> forceinline InputRange(value_type(&arr)[N]):
		mInterface(new WrapperImpl<AsRangeResult<value_type(&)[N]>>(AsRange(arr))) {}

	template<size_t N> forceinline InputRange& operator=(value_type(&arr)[N])
	{
		operator=(AsRange(arr));
		return *this;
	}

	forceinline InputRange(std::initializer_list<Meta::RemoveConst<value_type>> arr):
		InputRange(AsRange(arr)) {}

	forceinline InputRange& operator=(std::initializer_list<Meta::RemoveConst<value_type>> arr)
	{
		operator=(AsRange(arr));
		return *this;
	}


	forceinline bool Empty() const {return mInterface==null || mInterface->Empty();}
	forceinline T First() const {return mInterface->First();}
	forceinline void PopFirst() {mInterface->PopFirst();}
	forceinline T GetNext() {return mInterface->GetNext();}

	forceinline void PopFirstN(size_t count) {mInterface->PopFirstN(count);}
	forceinline void PopFirstExactly(size_t count) {mInterface->PopFirstExactly(count);}

	forceinline ArrayRange<Meta::RemoveConst<value_type>> CopyAdvanceToAdvance(ArrayRange<Meta::RemoveConst<value_type>>& dst, size_t n = ~size_t(0))
	{return mInterface->CopyAdvanceToAdvance(dst, Op::Min(n, dst.Length()));}

protected:
	Memory::UniqueRef<Interface> mInterface;
	InputRange(Interface* interfacePtr): mInterface(interfacePtr) {}
};


template<typename T> struct FiniteInputRange: InputRange<T>
{
	enum: bool {RangeIsFinite = true};

protected:
	typedef typename InputRange<T>::value_type value_type;
	typedef typename InputRange<T>::Interface Interface;
	template<typename R> using WrapperImpl = typename InputRange<T>::template WrapperImpl<R>;

private:
	template<typename R> forceinline static Interface* wrap(R&& range)
	{
		typedef Meta::RemoveConstRef<AsRangeResult<R>> Range;
		return new WrapperImpl<Range>(AsRange(Meta::Forward<R>(range)));
	}
public:

	forceinline FiniteInputRange(null_t=null) {}

	forceinline FiniteInputRange(FiniteInputRange&& rhs):
		InputRange<T>(Meta::Move(static_cast<InputRange<T>&&>(rhs))) {}

	FiniteInputRange(const FiniteInputRange& rhs) = delete;

	forceinline FiniteInputRange& operator=(FiniteInputRange&& rhs)
	{
		InputRange<T>::operator=(Meta::Move(rhs));
		return *this;
	}

	FiniteInputRange& operator=(const FiniteInputRange& rhs) = delete;

	template<typename R, typename = D::EnableCondition<R, FiniteInputRange>>
	forceinline FiniteInputRange(R&& range):
		InputRange<T>(wrap(Meta::Forward<R>(range))) {}

	template<typename R, typename = D::EnableCondition<R, FiniteInputRange>>
	forceinline FiniteInputRange& operator=(R&& range)
	{
		InputRange<T>::mInterface = wrap(Meta::Forward<R>(range));
		return *this;
	}

	template<size_t N> forceinline FiniteInputRange(value_type(&arr)[N]):
		FiniteInputRange(AsRange(arr)) {}

	template<size_t N> forceinline FiniteInputRange& operator=(value_type(&arr)[N])
	{
		operator=(AsRange(arr));
		return *this;
	}

	forceinline FiniteInputRange(std::initializer_list<Meta::RemoveConst<value_type>> arr):
		FiniteInputRange(AsRange(arr)) {}

	forceinline FiniteInputRange& operator=(std::initializer_list<Meta::RemoveConst<value_type>> arr)
	{
		operator=(AsRange(arr));
		return *this;
	}

protected:
	FiniteInputRange(Interface* interfacePtr): InputRange<T>(interfacePtr) {}
};

template<typename T> struct ForwardRange: InputRange<T>
{
protected:
	typedef typename InputRange<T>::value_type value_type;
	struct Interface: InputRange<T>::Interface
	{
		virtual Interface* Clone() const = 0;
	};

	template<typename R, typename PARENT> using FullImplFiller =
		typename InputRange<T>::TEMPLATE ImplFiller<R, PARENT>;

	template<typename R> struct WrapperImpl: FullImplFiller<R, Interface>
	{
		template<typename A> WrapperImpl(A&& range): FullImplFiller<R, Interface>(Meta::Forward<A>(range)) {}

		Interface* Clone() const override {return new WrapperImpl(FullImplFiller<R, Interface>::OriginalRange);}
	};

	Interface* clone() const {return static_cast<Interface*>(InputRange<T>::mInterface.ptr)->Clone();}

private:
	template<typename R> forceinline static Interface* wrap(R&& range)
	{
		typedef Meta::RemoveConstRef<AsRangeResult<R>> Range;
		return new WrapperImpl<Range>(AsRange(Meta::Forward<R>(range)));
	}

public:
	forceinline ForwardRange(null_t=null) {}

	forceinline ForwardRange(ForwardRange&& rhs):
		InputRange<T>(Meta::Move(static_cast<InputRange<T>&&>(rhs))) {}

	forceinline ForwardRange& operator=(ForwardRange&& rhs)
	{
		InputRange<T>::operator=(Meta::Move(static_cast<InputRange<T>&&>(rhs)));
		return *this;
	}

	forceinline ForwardRange(const ForwardRange& rhs):
		InputRange<T>(rhs.clone()) {}

	forceinline ForwardRange& operator=(const ForwardRange& rhs)
	{
		InputRange<T>::mInterface = rhs.clone();
		return *this;
	}

	template<typename R, typename = D::EnableCondition<R, ForwardRange>>
	forceinline ForwardRange(R&& range):
		InputRange<T>(wrap(Meta::Forward<R>(range))) {}

	template<typename R, typename = D::EnableCondition<R, ForwardRange>>
	forceinline ForwardRange& operator=(R&& range)
	{
		InputRange<T>::mInterface = wrap(Meta::Forward<R>(range));
		return *this;
	}

	template<size_t N> forceinline ForwardRange(value_type(&arr)[N]):
		ForwardRange(AsRange(arr)) {}

	template<size_t N> forceinline ForwardRange& operator=(value_type(&arr)[N])
	{
		operator=(AsRange(arr));
		return *this;
	}

	forceinline ForwardRange(std::initializer_list<Meta::RemoveConst<value_type>> arr):
		ForwardRange(AsRange(arr)) {}

	forceinline ForwardRange& operator=(std::initializer_list<Meta::RemoveConst<value_type>> arr)
	{
		operator=(AsRange(arr));
		return *this;
	}

protected:
	ForwardRange(Interface* interfacePtr): InputRange<T>(interfacePtr) {}
};

template<typename T> struct FiniteForwardRange: ForwardRange<T>
{
	enum: bool {RangeIsFinite = true};
protected:
	typedef typename InputRange<T>::value_type value_type;
	struct Interface: ForwardRange<T>::Interface
	{
		virtual size_t Count() const = 0;
	};

	template<typename R, typename PARENT> using FullImplFiller =
		typename InputRange<T>::TEMPLATE ImplFiller<R, PARENT>;

	template<typename R> struct WrapperImpl:
		FullImplFiller<R, Interface>
	{
		template<typename A> WrapperImpl(A&& range): FullImplFiller<R, Interface>(Meta::Forward<A>(range)) {}

		size_t Count() const override {return FullImplFiller<R, Interface>::OriginalRange.Count();}
		Interface* Clone() const override {return new WrapperImpl(FullImplFiller<R, Interface>::OriginalRange);}
	};

private:
	template<typename R> forceinline static Interface* wrap(R&& range)
	{
		typedef Meta::RemoveConstRef<AsRangeResult<R>> Range;
		return new WrapperImpl<Range>(AsRange(Meta::Forward<R>(range)));
	}

public:
	forceinline FiniteForwardRange(null_t=null) {}

	forceinline FiniteForwardRange(FiniteForwardRange&& rhs):
		ForwardRange<T>(Meta::Move(static_cast<ForwardRange<T>&&>(rhs))) {}

	forceinline FiniteForwardRange& operator=(FiniteForwardRange&& rhs)
	{
		ForwardRange<T>::operator=(Meta::Move(static_cast<ForwardRange<T>&&>(rhs)));
		return *this;
	}

	forceinline FiniteForwardRange(const FiniteForwardRange& rhs):
		ForwardRange<T>(rhs.clone()) {}

	forceinline FiniteForwardRange& operator=(const FiniteForwardRange& rhs)
	{
		InputRange<T>::mInterface = rhs.clone();
		return *this;
	}

	template<typename R, typename = D::EnableCondition<R, FiniteForwardRange>>
	forceinline FiniteForwardRange(R&& range):
		ForwardRange<T>(wrap(Meta::Forward<R>(range))) {}

	template<typename R, typename = D::EnableCondition<R, FiniteForwardRange>>
	forceinline FiniteForwardRange& operator=(R&& range)
	{
		InputRange<T>::mInterface = wrap(Meta::Forward<R>(range));
		return *this;
	}

	template<size_t N> forceinline FiniteForwardRange(value_type(&arr)[N]):
		FiniteForwardRange(AsRange(arr)) {}

	template<size_t N> forceinline FiniteForwardRange& operator=(value_type(&arr)[N])
	{
		operator=(AsRange(arr));
		return *this;
	}

	forceinline FiniteForwardRange(std::initializer_list<Meta::RemoveConst<value_type>> arr):
		FiniteForwardRange(AsRange(arr)) {}

	forceinline FiniteForwardRange& operator=(std::initializer_list<Meta::RemoveConst<value_type>> arr)
	{
		operator=(AsRange(arr));
		return *this;
	}

	forceinline size_t Length() const
	{return static_cast<Interface*>(InputRange<T>::mInterface.ptr)->Count();}

protected:
	FiniteForwardRange(Interface* interfacePtr): ForwardRange<T>(interfacePtr) {}
};

template<typename T> struct BidirectionalRange: FiniteForwardRange<T>
{
protected:
	typedef typename InputRange<T>::value_type value_type;
	struct Interface: FiniteForwardRange<T>::Interface
	{
		virtual T Last() const = 0;
		virtual void PopLast() = 0;
		virtual void PopLastN(size_t count) = 0;
		virtual void PopLastExactly(size_t count) = 0;
	};

	template<typename R, typename PARENT> struct ImplFiller: PARENT
	{
		size_t Count() const override {return PARENT::OriginalRange.Count();}

		T Last() const override {return PARENT::OriginalRange.Last();}
		void PopLast() override {PARENT::OriginalRange.PopLast();}
		void PopLastN(size_t count) override {PARENT::OriginalRange.PopLastN(count);}
		void PopLastExactly(size_t count) override {PARENT::OriginalRange.PopLastExactly(count);}
	};

	template<typename R, typename PARENT> using FullImplFiller =
		ImplFiller<R, typename InputRange<T>::TEMPLATE ImplFiller<R, PARENT>>;

	template<typename R> struct WrapperImpl: FullImplFiller<R, Interface>
	{
		typedef FullImplFiller<R, Interface> base;
		template<typename A> WrapperImpl(A&& range): base(Meta::Forward<A>(range)) {}
		Interface* Clone() const override {return new WrapperImpl(base::OriginalRange);}
	};

private:
	template<typename R> forceinline static Interface* wrap(R&& range)
	{
		typedef Meta::RemoveConstRef<AsRangeResult<R>> Range;
		return new WrapperImpl<Range>(AsRange(Meta::Forward<R>(range)));
	}

public:
	forceinline BidirectionalRange(null_t=null) {}

	forceinline BidirectionalRange(BidirectionalRange&& rhs):
		FiniteForwardRange<T>(Meta::Move(static_cast<FiniteForwardRange<T>&&>(rhs))) {}

	forceinline BidirectionalRange& operator=(BidirectionalRange&& rhs)
	{
		FiniteForwardRange<T>::operator=(Meta::Move(static_cast<FiniteForwardRange<T>&&>(rhs)));
		return *this;
	}

	forceinline BidirectionalRange& operator=(const BidirectionalRange& range)
	{
		InputRange<T>::mInterface = range.clone();
		return *this;
	}

	forceinline BidirectionalRange(const BidirectionalRange& rhs):
		FiniteForwardRange<T>(rhs.clone()) {}

	template<typename R, typename = D::EnableCondition<R, BidirectionalRange>>
	forceinline BidirectionalRange(R&& range):
		FiniteForwardRange<T>(wrap(Meta::Forward<R>(range))) {}

	template<typename R, typename = D::EnableCondition<R, BidirectionalRange>>
	forceinline BidirectionalRange& operator=(R&& range)
	{
		InputRange<T>::mInterface = wrap(Meta::Forward<R>(range));
		return *this;
	}

	template<size_t N> forceinline BidirectionalRange(value_type(&arr)[N]):
		BidirectionalRange(AsRange(arr)) {}

	template<size_t N> forceinline BidirectionalRange& operator=(value_type(&arr)[N])
	{
		operator=(AsRange(arr));
		return *this;
	}

	forceinline BidirectionalRange(std::initializer_list<Meta::RemoveConst<value_type>> arr):
		BidirectionalRange(AsRange(arr)) {}

	forceinline BidirectionalRange& operator=(std::initializer_list<Meta::RemoveConst<value_type>> arr)
	{
		operator=(AsRange(arr));
		return *this;
	}

	forceinline T Last() const
	{return static_cast<Interface*>(InputRange<T>::mInterface.ptr)->Last();}

	forceinline void PopLast()
	{static_cast<Interface*>(InputRange<T>::mInterface.ptr)->PopLast();}

	forceinline void PopLastN(size_t count)
	{static_cast<Interface*>(InputRange<T>::mInterface.ptr)->PopLastN(count);}

	forceinline void PopLastExactly(size_t count)
	{static_cast<Interface*>(InputRange<T>::mInterface.ptr)->PopLastExactly(count);}

protected:
	BidirectionalRange(Interface* interfacePtr): FiniteForwardRange<T>(interfacePtr) {}
};

template<typename T> struct RandomAccessRange: ForwardRange<T>
{
protected:
	typedef typename InputRange<T>::value_type value_type;
	struct Interface: ForwardRange<T>::Interface
	{
		virtual T OpIndex(size_t index) const = 0;
	};

	template<typename R, typename PARENT> using FullImplFiller =
		typename ForwardRange<T>::template FullImplFiller<R, PARENT>;

	template<typename R> struct WrapperImpl: FullImplFiller<R, Interface>
	{
		template<typename A> WrapperImpl(A&& range): FullImplFiller<R, Interface>(Meta::Forward<A>(range)) {}

		Interface* Clone() const override
		{return new WrapperImpl(FullImplFiller<R, Interface>::OriginalRange);}

		T OpIndex(size_t index) const override
		{return FullImplFiller<R, Interface>::OriginalRange[index];}
	};

private:
	template<typename R> forceinline static Interface* wrap(R&& range)
	{
		typedef Meta::RemoveConstRef<AsRangeResult<R>> Range;
		return new WrapperImpl<Range>(AsRange(Meta::Forward<R>(range)));
	}

public:
	forceinline RandomAccessRange(null_t=null) {}

	forceinline RandomAccessRange(RandomAccessRange&& rhs):
		ForwardRange<T>(Meta::Move(static_cast<ForwardRange<T>&&>(rhs))) {}

	forceinline RandomAccessRange& operator=(RandomAccessRange&& rhs)
	{
		InputRange<T>::mInterface = Meta::Move(rhs.mInterface);
		return *this;
	}

	forceinline RandomAccessRange(const RandomAccessRange& rhs):
		ForwardRange<T>(rhs.clone()) {}

	forceinline RandomAccessRange& operator=(const RandomAccessRange& range)
	{
		InputRange<T>::mInterface = range.clone();
		return *this;
	}

	template<typename R, typename = D::EnableCondition<R, RandomAccessRange>>
	forceinline RandomAccessRange(R&& range):
		ForwardRange<T>(wrap(Meta::Forward<R>(range))) {}

	template<typename R, typename = D::EnableCondition<R, RandomAccessRange>>
	forceinline RandomAccessRange& operator=(R&& range)
	{
		InputRange<T>::mInterface = wrap(Meta::Forward<R>(range));
		return *this;
	}

	template<size_t N> forceinline RandomAccessRange(value_type(&arr)[N]):
		RandomAccessRange(AsRange(arr)) {}

	template<size_t N> forceinline RandomAccessRange& operator=(value_type(&arr)[N])
	{
		operator=(AsRange(arr));
		return *this;
	}

	forceinline RandomAccessRange(std::initializer_list<Meta::RemoveConst<value_type>> arr):
		RandomAccessRange(AsRange(arr)) {}

	forceinline RandomAccessRange& operator=(std::initializer_list<Meta::RemoveConst<value_type>> arr)
	{
		operator=(AsRange(arr));
		return *this;
	}


	forceinline T operator[](size_t index) const
	{return static_cast<Interface*>(InputRange<T>::mInterface.ptr)->OpIndex(index);}

	RandomAccessRange Drop(size_t count=1)
	{
		auto result = *this;
		result.PopFirstN(count);
		return result;
	}

	RTake<RandomAccessRange> operator()(size_t start, size_t end) const
	{
		INTRA_ASSERT(end>=start);
		return Take(Drop(start), end-start);
	}

protected:
	RandomAccessRange(Interface* interfacePtr): ForwardRange<T>(interfacePtr) {}
};


template<typename T> struct FiniteRandomAccessRange: BidirectionalRange<T>
{
protected:
	typedef typename InputRange<T>::value_type value_type;
	struct Interface: BidirectionalRange<T>::Interface
	{
		virtual T OpIndex(size_t index) const = 0;
		virtual T OpSlice(size_t start, size_t end) const = 0;
	};

	template<typename R, typename PARENT> using FullImplFiller = 
		typename BidirectionalRange<T>::TEMPLATE FullImplFiller<R, PARENT>;

	template<typename R> struct WrapperImpl: FullImplFiller<R, Interface>
	{
		template<typename A> WrapperImpl(A&& range): FullImplFiller<R, Interface>(Meta::Forward<A>(range)) {}

		WrapperImpl* Clone() const override {return new WrapperImpl(FullImplFiller<R, Interface>::OriginalRange);}
		T OpIndex(size_t index) const override {return FullImplFiller<R, Interface>::OriginalRange[index];}
		
		Interface* OpSlice(size_t start, size_t end) const override
		{return new WrapperImpl(FullImplFiller<R, Interface>::OriginalRange(start, end));}

		size_t Count() const override {return FullImplFiller<R, Interface>::OriginalRange.Count();}
	};

private:
	template<typename R> forceinline static Interface* wrap(R&& range)
	{
		typedef Meta::RemoveConstRef<AsRangeResult<R>> Range;
		return new WrapperImpl<Range>(AsRange(Meta::Forward<R>(range)));
	}

public:
	forceinline FiniteRandomAccessRange(null_t=null) {}

	forceinline FiniteRandomAccessRange(FiniteRandomAccessRange&& rhs):
		BidirectionalRange<T>(Meta::Move(static_cast<BidirectionalRange<T>&&>(rhs))) {}

	forceinline FiniteRandomAccessRange& operator=(FiniteRandomAccessRange&& rhs)
	{
		BidirectionalRange<T>::operator=(Meta::Move(static_cast<BidirectionalRange<T>&&>(rhs)));
		return *this;
	}

	forceinline FiniteRandomAccessRange(const FiniteRandomAccessRange& rhs):
		BidirectionalRange<T>(rhs.clone()) {}

	forceinline FiniteRandomAccessRange& operator=(const FiniteRandomAccessRange& rhs)
	{
		InputRange<T>::mInterface = rhs.clone();
		return *this;
	}

	template<typename R, typename = D::EnableCondition<R, FiniteRandomAccessRange>>
	forceinline FiniteRandomAccessRange(R&& range):
		BidirectionalRange<T>(wrap(Meta::Forward<R>(range))) {}

	template<typename R, typename = D::EnableCondition<R, FiniteRandomAccessRange>>
	forceinline FiniteRandomAccessRange& operator=(R&& range)
	{
		InputRange<T>::mInterface = wrap(Meta::Forward<R>(range));
		return *this;
	}

	template<size_t N> forceinline FiniteRandomAccessRange(value_type(&arr)[N]):
		FiniteRandomAccessRange(AsRange(arr)) {}

	template<size_t N> forceinline FiniteRandomAccessRange& operator=(value_type(&arr)[N])
	{
		operator=(AsRange(arr));
		return *this;
	}

	forceinline FiniteRandomAccessRange(std::initializer_list<Meta::RemoveConst<value_type>> arr):
		FiniteRandomAccessRange(AsRange(arr)) {}

	forceinline FiniteRandomAccessRange& operator=(std::initializer_list<Meta::RemoveConst<value_type>> arr)
	{
		operator=(AsRange(arr));
		return *this;
	}

	forceinline T operator[](size_t index) const
	{return static_cast<Interface*>(InputRange<T>::mInterface.ptr)->OpIndex(index);}

	forceinline FiniteRandomAccessRange operator()(size_t start, size_t end) const
	{return FiniteRandomAccessRange(static_cast<Interface*>(InputRange<T>::mInterface.ptr)->OpSlice(start, end));}

protected:
	FiniteRandomAccessRange(Interface* interfacePtr): BidirectionalRange<T>(interfacePtr) {}
};

#undef TEMPLATE

INTRA_WARNING_POP

}


using Range::InputRange;
using Range::FiniteInputRange;
using Range::ForwardRange;
using Range::FiniteForwardRange;
using Range::BidirectionalRange;
using Range::RandomAccessRange;
using Range::FiniteRandomAccessRange;
}
