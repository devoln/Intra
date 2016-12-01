#pragma once

#include "Utils/Wrapper.h"

namespace Intra { namespace Range {

template<typename T> struct InputRange:
	RangeMixin<InputRange<T>, T, TypeEnum::Input, false>
{
	struct Interface
	{
		typedef Meta::RemoveConstRef<T> value_type;
		typedef T return_value_type;

		virtual ~Interface() {}

		virtual bool Empty() const = 0;
		virtual T First() const = 0;
		virtual void PopFirst() = 0;

		//Эти методы могут быть реализованы через методы выше, 
		//но чтобы избежать многократных виртуальных вызовов их лучше переопределить
		virtual void PopFirstN(size_t count) {while(count!=0 && !Empty()) PopFirst(), count--;}
		virtual void PopFirstExactly(size_t count) {while(count!=0) PopFirst(), count--;}
	};

	template<typename R> struct WrapperImpl: Interface
	{
		R OriginalRange;

		template<typename = Meta::EnableIf<
			Meta::IsCopyConstructible<R>::_
		>> WrapperImpl(const R& range): OriginalRange(range) {}

		WrapperImpl(R&& range): OriginalRange(core::move(range)) {}

		bool Empty() const override final {return OriginalRange.Empty();}
		T First() const override final {return OriginalRange.First();}
		void PopFirst() override final {OriginalRange.PopFirst();}

		void PopFirstN(size_t count) override final {OriginalRange.PopFirstN(count);}
		void PopFirstExactly(size_t count) override final {OriginalRange.PopFirstExactly(count);}

		WrapperImpl& operator=(const WrapperImpl&) = delete;
	};

	template<typename R, typename = Meta::EnableIf<
		IsInputRangeOf<R, T>::_
	>> forceinline InputRange(R&& range):
		mInterface(new WrapperImpl<Meta::RemoveReference<R>>(core::forward<R>(range))) {}

	template<typename R> forceinline InputRange(Meta::EnableIf<
		!IsInputRange<R>::_ && HasAsRange<R>::_,
	R> range): mInterface(new WrapperImpl<AsRangeResult<R>>(AsRange(range))) {}

	template<typename R> forceinline InputRange& operator=(Meta::RemoveConstRef<R>&& range)
	{
		mInterface = new WrapperImpl<R>(core::move(AsRange(range)));
		return *this;
	}

	template<typename R> forceinline InputRange& operator=(R range)
	{
		mInterface = new WrapperImpl<R>(AsRange(range));
		return *this;
	}

	forceinline InputRange(InputRange&& rhs):
		mInterface(core::move(rhs.mInterface)) {}

	InputRange(const InputRange& rhs) = delete;

	forceinline InputRange& operator=(InputRange&& rhs)
	{
		mInterface = core::move(rhs.mInterface);
		return *this;
	}

	InputRange& operator=(const InputRange& rhs) = delete;

	forceinline bool Empty() const {return mInterface==null || mInterface->Empty();}
	forceinline T First() const {return mInterface->First();}
	forceinline void PopFirst() {mInterface->PopFirst();}

	forceinline void PopFirstN(size_t count) {mInterface->PopFirstN(count);}
	forceinline void PopFirstExactly(size_t count) {mInterface->PopFirstExactly(count);}

private:
	Memory::UniqueRef<Interface> mInterface;
};


template<typename T> struct FiniteInputRange:
	RangeMixin<FiniteInputRange<T>, T, TypeEnum::Input, true>
{
	typedef typename InputRange<T>::Interface Interface;
	template<typename R> using WrapperImpl = typename InputRange<T>::template WrapperImpl<R>;

	template<typename R, typename = Meta::EnableIf<
		IsFiniteInputRange<Meta::RemoveConstRef<R>>::_ && !Meta::TypeEqualsIgnoreCVRef<R, FiniteInputRange>::_
	>> forceinline FiniteInputRange(R&& range):
		mInterface(new WrapperImpl<Meta::RemoveConstRef<R>>(core::forward<R>(range))) {}

	template<typename R, typename = Meta::EnableIf<
		!IsFiniteInputRange<R>::_ && HasAsRange<R>::_ && !Meta::TypeEqualsIgnoreCVRef<R, FiniteInputRange>::_
	>> forceinline FiniteInputRange(R range):
		mInterface(new WrapperImpl<AsRangeResult<R>>(AsRange(range))) {}

	template<size_t N> forceinline FiniteInputRange(T(&arr)[N]):
		mInterface(new WrapperImpl<ArrayRange<T>>(ArrayRange<T>(arr))) {}

	forceinline FiniteInputRange(FiniteInputRange&& rhs):
		mInterface(core::move(rhs.mInterface)) {}

	FiniteInputRange(const FiniteInputRange& rhs) = delete;

	template<typename R, typename = Meta::EnableIf<
		!IsFiniteInputRange<R>::_ && HasAsRange<R>::_ && !Meta::TypeEqualsIgnoreCVRef<R, FiniteInputRange>::_
	>> forceinline FiniteInputRange& operator=(R range)
	{
		mInterface = new WrapperImpl<AsRangeResult<R>>(AsRange(range));
		return *this;
	}

	template<typename R, typename = Meta::EnableIf<
		IsFiniteInputRange<Meta::RemoveConstRef<R>>::_ && !Meta::TypeEqualsIgnoreCVRef<R, FiniteInputRange>::_
	>> forceinline FiniteInputRange& operator=(R&& range)
	{
		mInterface = new WrapperImpl<R>(core::move(range));
		return *this;
	}

	forceinline FiniteInputRange& operator=(FiniteInputRange&& rhs)
	{
		mInterface = core::move(rhs.mInterface);
		return *this;
	}

	FiniteInputRange& operator=(const FiniteInputRange& rhs) = delete;

	forceinline bool Empty() const {return mInterface==null || mInterface->Empty();}
	forceinline T First() const {return mInterface->First();}
	forceinline void PopFirst() {mInterface->PopFirst();}

	forceinline void PopFirstN(size_t count) {mInterface->PopFirstN(count);}
	forceinline void PopFirstExactly(size_t count) {mInterface->PopFirstExactly(count);}

private:
	Memory::UniqueRef<Interface> mInterface;
};

template<typename T> struct ForwardRange:
	RangeMixin<ForwardRange<T>, T, TypeEnum::Forward, false>
{
	struct Interface: InputRange<T>::Interface
	{
		virtual Interface* Clone() const = 0;
	};

	template<typename R> struct WrapperImpl: Interface
	{
		R OriginalRange;

		WrapperImpl(const R& range): OriginalRange(range) {}
		WrapperImpl(R&& range): OriginalRange(core::move(range)) {}

		bool Empty() const override final {return OriginalRange.Empty();}
		T First() const override final {return OriginalRange.First();}
		void PopFirst() override final {OriginalRange.PopFirst();}

		void PopFirstN(size_t count) override final {OriginalRange.PopFirstN(count);}
		void PopFirstExactly(size_t count) override final {OriginalRange.PopFirstExactly(count);}

		WrapperImpl* Clone() const override final {return new WrapperImpl{OriginalRange};}
	};

	template<typename R> forceinline ForwardRange(const R& range):
		mInterface(new WrapperImpl<R>{range}) {}

	template<typename R> forceinline ForwardRange(Meta::RemoveConstRef<R>&& range):
		mInterface(new WrapperImpl<Meta::RemoveConstRef<R>>{core::move(range)}) {}

	forceinline ForwardRange(const ForwardRange& range):
		mInterface(range.mInterface->Clone()) {}

	forceinline ForwardRange(ForwardRange&& range):
		mInterface(core::move(range.mInterface)) {}

	template<typename R> forceinline ForwardRange& operator=(R&& range)
	{
		mInterface = new WrapperImpl<R>{core::move(range)};
		return *this;
	}

	template<typename R> forceinline ForwardRange& operator=(const R& range)
	{
		mInterface = new WrapperImpl<R>{range};
		return *this;
	}

	forceinline ForwardRange& operator=(const ForwardRange& range)
	{
		mInterface = range.mInterface->Clone();
		return *this;
	}

	forceinline ForwardRange& operator=(ForwardRange&& range)
	{
		mInterface = core::move(range.mInterface);
		return *this;
	}

	forceinline bool Empty() const {return mInterface==null || mInterface->Empty();}
	forceinline T First() const {return mInterface->First();}
	forceinline void PopFirst() {mInterface->PopFirst();}

	forceinline void PopFirstN(size_t count) {mInterface->PopFirstN(count);}
	forceinline void PopFirstExactly(size_t count) {mInterface->PopFirstExactly(count);}

private:
	Memory::UniqueRef<Interface> mInterface;
};

template<typename T> struct FiniteForwardRange:
	RangeMixin<FiniteForwardRange<T>, T, TypeEnum::Forward, true>
{
	struct Interface: ForwardRange<T>::Interface
	{
		virtual Interface* Clone() const = 0;
		virtual size_t Count() const = 0;
	};

	template<typename R> struct WrapperImpl: Interface
	{
		R OriginalRange;

		WrapperImpl(const R& range): OriginalRange(range) {}
		WrapperImpl(R&& range): OriginalRange(core::move(range)) {}

		bool Empty() const override final {return OriginalRange.Empty();}
		T First() const override final {return OriginalRange.First();}
		void PopFirst() override final {OriginalRange.PopFirst();}

		void PopFirstN(size_t count) override final {OriginalRange.PopFirstN(count);}
		void PopFirstExactly(size_t count) override final {OriginalRange.PopFirstExactly(count);}

		WrapperImpl* Clone() const override final {return new WrapperImpl{OriginalRange};}

		size_t Count() const override final {return OriginalRange.Count();}
	};

	template<typename R> forceinline FiniteForwardRange(const R& range):
		mInterface(new WrapperImpl<R>(range)) {}

	template<typename R> forceinline FiniteForwardRange(Meta::RemoveConstRef<R>&& range):
		mInterface(new WrapperImpl<Meta::RemoveConstRef<R>>(core::move(range))) {}

	forceinline FiniteForwardRange(const FiniteForwardRange& range):
		mInterface(range.mInterface->Clone()) {}

	forceinline FiniteForwardRange(FiniteForwardRange&& range):
		mInterface(core::move(range.mInterface)) {}

	template<typename R> forceinline FiniteForwardRange& operator=(R&& range)
	{
		mInterface = new WrapperImpl<R>(core::move(range));
		return *this;
	}

	template<typename R> forceinline FiniteForwardRange& operator=(const R& range)
	{
		mInterface = new WrapperImpl<R>(range);
		return *this;
	}

	forceinline FiniteForwardRange& operator=(const FiniteForwardRange& range)
	{
		mInterface = range.mInterface->Clone();
		return *this;
	}

	forceinline FiniteForwardRange& operator=(FiniteForwardRange&& range)
	{
		mInterface = core::move(range.mInterface);
		return *this;
	}

	forceinline bool Empty() const {return mInterface==null || mInterface->Empty();}
	forceinline T First() const {return mInterface->First();}
	forceinline void PopFirst() {mInterface->PopFirst();}

	forceinline void PopFirstN(size_t count) {mInterface->PopFirstN(count);}
	forceinline void PopFirstExactly(size_t count) {mInterface->PopFirstExactly(count);}

	forceinline size_t Count() const {return mInterface->Count();}

private:
	Memory::UniqueRef<Interface> mInterface;
};

template<typename T> struct BidirectionalRange:
	RangeMixin<BidirectionalRange<T>, T, TypeEnum::Bidirectional, true>
{
	struct Interface: FiniteForwardRange<T>::Interface
	{
		virtual Interface* Clone() const = 0;
		virtual T Last() const = 0;
		virtual void PopLast() = 0;
		virtual void PopLastN(size_t count) = 0;
		virtual void PopLastExactly(size_t count) = 0;
	};

	template<typename R> struct WrapperImpl: Interface
	{
		R OriginalRange;

		WrapperImpl(R&& range): OriginalRange(core::move(OriginalRange)) {}

		bool Empty() const override final {return OriginalRange.Empty();}
		T First() const override final {return OriginalRange.First();}
		void PopFirst() override final {OriginalRange.PopFirst();}

		void PopFirstN(size_t count) override final {OriginalRange.PopFirstN(count);}
		void PopFirstExactly(size_t count) override final {OriginalRange.PopFirstExactly(count);}

		WrapperImpl* Clone() const override final {return new WrapperImpl(OriginalRange);}

		size_t Count() const override final {return OriginalRange.Count();}

		T Last() const {return OriginalRange.Last();}
		void PopLast() {OriginalRange.PopLast();}
		void PopLastN(size_t count) {OriginalRange.PopLastN(count);}
		void PopLastExactly(size_t count) {OriginalRange.PopLastExactly(count);}
	};

	template<typename R> forceinline BidirectionalRange(const R& range):
		mInterface(new WrapperImpl<R>(range)) {}

	template<typename R> forceinline BidirectionalRange(R&& range):
		mInterface(new WrapperImpl<R>(core::move(range))) {}

	forceinline BidirectionalRange(const BidirectionalRange& range):
		mInterface(range.mInterface->Clone()) {}

	forceinline BidirectionalRange(BidirectionalRange&& range):
		mInterface(core::move(range.mInterface)) {}

	template<typename R> forceinline BidirectionalRange& operator=(R&& range)
	{
		mInterface = new WrapperImpl<R>(core::move(range));
		return *this;
	}

	template<typename R> forceinline BidirectionalRange& operator=(const R& range)
	{
		mInterface = new WrapperImpl<R>(range);
		return *this;
	}

	forceinline BidirectionalRange& operator=(const BidirectionalRange& range)
	{
		mInterface = range.mInterface->Clone();
		return *this;
	}

	forceinline BidirectionalRange& operator=(BidirectionalRange&& range)
	{
		mInterface = core::move(range.mInterface);
		return *this;
	}

	forceinline bool Empty() const {return mInterface==null || mInterface->Empty();}
	forceinline T First() const {return mInterface->First();}
	forceinline void PopFirst() {mInterface->PopFirst();}

	forceinline void PopFirstN(size_t count) {mInterface->PopFirstN(count);}
	forceinline void PopFirstExactly(size_t count) {mInterface->PopFirstExactly(count);}

	forceinline size_t Count() const {return mInterface->Count();}

	forceinline T Last() const {return mInterface->Last();}
	forceinline void PopLast() {mInterface->PopLast();}
	forceinline void PopLastN(size_t count) {mInterface->PopLastN(count);}
	forceinline void PopLastExactly(size_t count) {mInterface->PopLastExactly(count);}

private:
	Memory::UniqueRef<Interface> mInterface;
};

template<typename T> struct RandomAccessRange:
	RangeMixin<RandomAccessRange<T>, T, TypeEnum::RandomAccess, false>
{
private:
	typedef RangeMixin<RandomAccessRange<T>, T, TypeEnum::RandomAccess, false> base;

public:
	struct Interface: ForwardRange<T>::Interface
	{
		virtual Interface* Clone() const = 0;
		virtual T OpIndex(size_t index) const = 0;
	};

	template<typename R> struct WrapperImpl: Interface
	{
		R OriginalRange;

		WrapperImpl(R&& range): OriginalRange(core::move(OriginalRange)) {}

		bool Empty() const override final {return OriginalRange.Empty();}
		T First() const override final {return OriginalRange.First();}
		void PopFirst() override final {OriginalRange.PopFirst();}

		void PopFirstN(size_t count) override final {OriginalRange.PopFirstN(count);}
		void PopFirstExactly(size_t count) override final {OriginalRange.PopFirstExactly(count);}

		WrapperImpl* Clone() const override final {return new WrapperImpl(OriginalRange);}

		T OpIndex(size_t index) const override final {return OriginalRange[index];}
	};

	template<typename R> forceinline RandomAccessRange(const R& range):
		mInterface(new WrapperImpl<R>(range)) {}

	template<typename R> forceinline RandomAccessRange(R&& range):
		mInterface(new WrapperImpl<R>(core::move(range))) {}

	forceinline RandomAccessRange(const RandomAccessRange& range):
		mInterface(range.mInterface->Clone()) {}

	forceinline RandomAccessRange(RandomAccessRange&& range):
		mInterface(core::move(range.mInterface)) {}

	template<typename R> forceinline RandomAccessRange& operator=(R&& range)
	{
		mInterface = new WrapperImpl<R>(core::move(range));
		return *this;
	}

	template<typename R> forceinline RandomAccessRange& operator=(const R& range)
	{
		mInterface = new WrapperImpl<R>(range);
		return *this;
	}

	forceinline RandomAccessRange& operator=(const RandomAccessRange& range)
	{
		mInterface = range.mInterface->Clone();
		return *this;
	}

	forceinline RandomAccessRange& operator=(RandomAccessRange&& range)
	{
		mInterface = core::move(range.mInterface);
		return *this;
	}

	forceinline bool Empty() const {return mInterface==null || mInterface->Empty();}
	forceinline T First() const {return mInterface->First();}
	forceinline void PopFirst() {mInterface->PopFirst();}

	forceinline void PopFirstN(size_t count) {mInterface->PopFirstN(count);}
	forceinline void PopFirstExactly(size_t count) {mInterface->PopFirstExactly(count);}

	forceinline T operator[](size_t index) const {return mInterface->OpIndex(index);}

	TakeResult<RandomAccessRange> opSlice(size_t start, size_t end) const
	{
		INTRA_ASSERT(end>=start);
		return base::Drop(start).Take(end-start);
	}

private:
	Memory::UniqueRef<Interface> mInterface;
};


template<typename T> struct FiniteRandomAccessRange:
	RangeMixin<FiniteRandomAccessRange<T>, T, TypeEnum::RandomAccess, true>
{
	struct Interface: BidirectionalRange<T>::Interface
	{
		virtual Interface* Clone() const = 0;
		virtual T OpIndex(size_t index) const = 0;
	};

	template<typename R> struct WrapperImpl: Interface
	{
		R OriginalRange;

		WrapperImpl(R&& range): OriginalRange(core::move(OriginalRange)) {}

		bool Empty() const override final {return OriginalRange.Empty();}
		T First() const override final {return OriginalRange.First();}
		void PopFirst() override final {OriginalRange.PopFirst();}

		void PopFirstN(size_t count) override final {OriginalRange.PopFirstN(count);}
		void PopFirstExactly(size_t count) override final {OriginalRange.PopFirstExactly(count);}

		WrapperImpl* Clone() const override final {return new WrapperImpl(OriginalRange);}

		size_t Count() const override final {return OriginalRange.Count();}

		T OpIndex(size_t index) const override final {return OriginalRange[index];}
	};

	template<typename R> forceinline FiniteRandomAccessRange(const R& range):
		mInterface(new WrapperImpl<R>(range)) {}

	template<typename R> forceinline FiniteRandomAccessRange(R&& range):
		mInterface(new WrapperImpl<R>(core::move(range))) {}

	forceinline FiniteRandomAccessRange(const FiniteRandomAccessRange& range):
		mInterface(range.mInterface->Clone()) {}

	forceinline FiniteRandomAccessRange(FiniteRandomAccessRange&& range):
		mInterface(core::move(range.mInterface)) {}

	template<typename R> forceinline FiniteRandomAccessRange& operator=(R&& range)
	{
		mInterface = new WrapperImpl<R>(core::move(range));
		return *this;
	}

	template<typename R> forceinline FiniteRandomAccessRange& operator=(const R& range)
	{
		mInterface = new WrapperImpl<R>(range);
		return *this;
	}

	forceinline FiniteRandomAccessRange& operator=(const FiniteRandomAccessRange& range)
	{
		mInterface = range.mInterface->Clone();
		return *this;
	}

	forceinline FiniteRandomAccessRange& operator=(FiniteRandomAccessRange&& range)
	{
		mInterface = core::move(range.mInterface);
		return *this;
	}

	forceinline bool Empty() const {return mInterface==null || mInterface->Empty();}
	forceinline T First() const {return mInterface->First();}
	forceinline void PopFirst() {mInterface->PopFirst();}

	forceinline void PopFirstN(size_t count) {mInterface->PopFirstN(count);}
	forceinline void PopFirstExactly(size_t count) {mInterface->PopFirstExactly(count);}

	forceinline size_t Count() const {return mInterface->Count();}

	forceinline T operator[](size_t index) const {return mInterface->OpIndex(index);}

private:
	Memory::UniqueRef<Interface> mInterface;
};

template<typename T> struct AnyArrayRange:
	RangeMixin<AnyArrayRange<T>, T, TypeEnum::Array, true>
{
	struct Interface: FiniteRandomAccessRange<T>::Interface
	{
		virtual Interface* Clone() const = 0;
		virtual T* Data() const = 0;
	};

	template<typename R> struct WrapperImpl: Interface
	{
		R OriginalRange;

		WrapperImpl(R&& range): OriginalRange(core::move(OriginalRange)) {}

		bool Empty() const override final {return OriginalRange.Empty();}
		T First() const override final {return OriginalRange.First();}
		void PopFirst() override final {OriginalRange.PopFirst();}

		void PopFirstN(size_t count) override final {OriginalRange.PopFirstN(count);}
		void PopFirstExactly(size_t count) override final {OriginalRange.PopFirstExactly(count);}

		WrapperImpl* Clone() const override final {return new WrapperImpl(OriginalRange);}

		size_t Count() const override final {return OriginalRange.Count();}

		T OpIndex(size_t index) const override final {return OriginalRange[index];}

		T* Data() const override final {return OriginalRange.Data();}
	};

	template<typename R> forceinline AnyArrayRange(const R& range):
		mInterface(new WrapperImpl<R>(range)) {}

	template<typename R> forceinline AnyArrayRange(R&& range):
		mInterface(new WrapperImpl<R>(core::move(range))) {}

	forceinline AnyArrayRange(const AnyArrayRange& range):
		mInterface(range.mInterface->Clone()) {}

	forceinline AnyArrayRange(AnyArrayRange&& range):
		mInterface(core::move(range.mInterface)) {}

	template<typename R> forceinline AnyArrayRange& operator=(R&& range)
	{
		mInterface = new WrapperImpl<R>(core::move(range));
		return *this;
	}

	template<typename R> forceinline AnyArrayRange& operator=(const R& range)
	{
		mInterface = new WrapperImpl<R>(range);
		return *this;
	}

	forceinline AnyArrayRange& operator=(const AnyArrayRange& range)
	{
		mInterface = range.mInterface->Clone();
		return *this;
	}

	forceinline AnyArrayRange& operator=(AnyArrayRange&& range)
	{
		mInterface = core::move(range.mInterface);
		return *this;
	}

	forceinline bool Empty() const {return mInterface==null || mInterface->Empty();}
	forceinline T First() const {return mInterface->First();}
	forceinline void PopFirst() {mInterface->PopFirst();}

	forceinline void PopFirstN(size_t count) {mInterface->PopFirstN(count);}
	forceinline void PopFirstExactly(size_t count) {mInterface->PopFirstExactly(count);}

	forceinline size_t Count() const {return mInterface->Count();}

	forceinline T operator[](size_t index) const {return mInterface->OpIndex(index);}

	T* Data() const override final {return mInterface->Data();}

private:
	Memory::UniqueRef<Interface> mInterface;
};

}

using Range::InputRange;
using Range::FiniteInputRange;
using Range::ForwardRange;
using Range::FiniteForwardRange;
using Range::BidirectionalRange;
using Range::RandomAccessRange;
using Range::FiniteRandomAccessRange;
using Range::AnyArrayRange;
}
