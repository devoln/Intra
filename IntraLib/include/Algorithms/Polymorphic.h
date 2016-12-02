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
		virtual T GetNext() = 0;

		//Эти методы могут быть реализованы через методы выше, 
		//но чтобы избежать многократных виртуальных вызовов их лучше переопределить
		virtual void PopFirstN(size_t count) {while(count!=0 && !Empty()) PopFirst(), count--;}
		virtual void PopFirstExactly(size_t count) {while(count!=0) PopFirst(), count--;}
	};

	template<typename R, typename PARENT> struct ImplFiller: PARENT
	{
		R OriginalRange;

		template<typename A> ImplFiller(A&& range): OriginalRange(core::forward<A>(range)) {}

		bool Empty() const override {return OriginalRange.Empty();}
		T First() const override {return OriginalRange.First();}
		void PopFirst() override {OriginalRange.PopFirst();}
		T GetNext() override {auto&& result = OriginalRange.First(); OriginalRange.PopFirst(); return result;}

		void PopFirstN(size_t count) override {OriginalRange.PopFirstN(count);}
		void PopFirstExactly(size_t count) override {OriginalRange.PopFirstExactly(count);}
	};

	template<typename R> struct WrapperImpl: ImplFiller<R, Interface>
	{
		template<typename A> WrapperImpl(A&& range): ImplFiller<R, Interface>(core::forward<A>(range)) {}
	};

	template<typename R, typename = Meta::EnableIf<
		IsInputRangeOf<Meta::RemoveConstRef<R>, T>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, InputRange>::_
	>> forceinline InputRange(R&& range):
		mInterface(new WrapperImpl<Meta::RemoveReference<R>>(core::forward<R>(range))) {}

	template<typename R, typename = Meta::EnableIf<
		!IsInputRange<R>::_ && HasAsRange<R>::_
	>> forceinline InputRange(R range):
		mInterface(new WrapperImpl<AsRangeResult<R>>(AsRange(range))) {}

	template<typename R, typename = Meta::EnableIf<
		!IsInputRange<R>::_ && HasAsRange<R>::_
	>> forceinline InputRange& operator=(R range)
	{
		mInterface = new WrapperImpl<AsRangeResult<R>>(AsRange(range));
		return *this;
	}

	template<typename R, typename = Meta::EnableIf<
		IsInputRangeOf<Meta::RemoveConstRef<R>, T>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, InputRange>::_
	>> forceinline InputRange& operator=(R&& range)
	{
		mInterface = new WrapperImpl<R>(core::move(range));
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
	forceinline T GetNext() {return mInterface->GetNext();}

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
		IsFiniteInputRangeOf<Meta::RemoveConstRef<R>, T>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, FiniteInputRange>::_
	>> forceinline FiniteInputRange(R&& range):
		mInterface(new WrapperImpl<Meta::RemoveConstRef<R>>(core::forward<R>(range))) {}

	template<typename R, typename = Meta::EnableIf<
		!IsInputRange<R>::_ && HasAsRange<R>::_
	>> forceinline FiniteInputRange(R range):
		mInterface(new WrapperImpl<AsRangeResult<R>>(AsRange(range))) {}

	template<size_t N> forceinline FiniteInputRange(T(&arr)[N]):
		mInterface(new WrapperImpl<ArrayRange<T>>(ArrayRange<T>(arr))) {}

	forceinline FiniteInputRange(FiniteInputRange&& rhs):
		mInterface(core::move(rhs.mInterface)) {}

	FiniteInputRange(const FiniteInputRange& rhs) = delete;

	template<typename R, typename = Meta::EnableIf<
		!IsInputRange<R>::_ && HasAsRange<R>::_
	>> forceinline FiniteInputRange& operator=(R range)
	{
		mInterface = new WrapperImpl<AsRangeResult<R>>(AsRange(range));
		return *this;
	}

	template<typename R, typename = Meta::EnableIf<
		IsFiniteInputRangeOf<Meta::RemoveConstRef<R>, T>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, FiniteInputRange>::_
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

	template<typename R> struct WrapperImpl: InputRange<T>::template ImplFiller<R, Interface>
	{
		typedef typename InputRange<T>::template ImplFiller<R, Interface> base;
		template<typename A> WrapperImpl(A&& range): base(core::forward<A>(range)) {}

		WrapperImpl* Clone() const override {return new WrapperImpl(base::OriginalRange);}
	};

	template<typename R, typename = Meta::EnableIf<
		IsForwardRange<Meta::RemoveConstRef<R>>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, ForwardRange>::_
	>> forceinline ForwardRange(R&& range):
		mInterface(new WrapperImpl<Meta::RemoveConstRef<R>>(core::forward<R>(range))) {}

	template<typename R, typename = Meta::EnableIf<
		!IsInputRange<R>::_ && HasAsRange<R>::_
	>> forceinline ForwardRange(R range):
		mInterface(new WrapperImpl<AsRangeResult<R>>(AsRange(range))) {}

	template<size_t N> forceinline ForwardRange(T(&arr)[N]):
		mInterface(new WrapperImpl<ArrayRange<T>>(ArrayRange<T>(arr))) {}

	forceinline ForwardRange(ForwardRange&& rhs):
		mInterface(core::move(rhs.mInterface)) {}

	forceinline ForwardRange(const ForwardRange& rhs):
		mInterface(rhs.mInterface->Clone()) {}

	template<typename R, typename = Meta::EnableIf<
		!IsInputRange<R>::_ && HasAsRange<R>::_
	>> forceinline ForwardRange& operator=(R range)
	{
		mInterface = new WrapperImpl<AsRangeResult<R>>(AsRange(range));
		return *this;
	}

	template<typename R, typename = Meta::EnableIf<
		IsForwardRange<Meta::RemoveConstRef<R>>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, ForwardRange>::_
	>> forceinline ForwardRange& operator=(R&& range)
	{
		mInterface = new WrapperImpl<R>(core::move(range));
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

	template<typename R> struct WrapperImpl: InputRange<T>::template ImplFiller<R, Interface>
	{
		typedef typename InputRange<T>::template ImplFiller<R, Interface> base;
		template<typename A> WrapperImpl(A&& range): base(core::forward<A>(range)) {}

		WrapperImpl* Clone() const override {return new WrapperImpl(base::OriginalRange);}
		size_t Count() const override {return base::OriginalRange.Count();}
	};

	template<typename R, typename = Meta::EnableIf<
		IsFiniteForwardRangeOf<Meta::RemoveConstRef<R>, T>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, FiniteForwardRange>::_
	>> forceinline FiniteForwardRange(R&& range):
		mInterface(new WrapperImpl<Meta::RemoveConstRef<R>>(core::forward<R>(range))) {}

	template<typename R, typename = Meta::EnableIf<
		!IsInputRange<R>::_ && HasAsRange<R>::_
	>> forceinline FiniteForwardRange(R range):
		mInterface(new WrapperImpl<AsRangeResult<R>>(AsRange(range))) {}

	template<size_t N> forceinline FiniteForwardRange(T(&arr)[N]):
		mInterface(new WrapperImpl<ArrayRange<T>>(ArrayRange<T>(arr))) {}

	forceinline FiniteForwardRange(FiniteForwardRange&& rhs):
		mInterface(core::move(rhs.mInterface)) {}

	forceinline FiniteForwardRange(const FiniteForwardRange& rhs):
		mInterface(rhs.mInterface->Clone()) {}

	template<typename R, typename = Meta::EnableIf<
		!IsInputRange<R>::_ && HasAsRange<R>::_
	>> forceinline FiniteForwardRange& operator=(R range)
	{
		mInterface = new WrapperImpl<AsRangeResult<R>>(AsRange(range));
		return *this;
	}

	template<typename R, typename = Meta::EnableIf<
		IsFiniteForwardRangeOf<Meta::RemoveConstRef<R>, T>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, FiniteForwardRange>::_
	>> forceinline FiniteForwardRange& operator=(R&& range)
	{
		mInterface = new WrapperImpl<R>(core::move(range));
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

	template<typename R, typename PARENT> struct ImplFiller: PARENT
	{
		size_t Count() const override {return PARENT::OriginalRange.Count();}

		T Last() const override {return PARENT::OriginalRange.Last();}
		void PopLast() override {PARENT::OriginalRange.PopLast();}
		void PopLastN(size_t count) override {PARENT::OriginalRange.PopLastN(count);}
		void PopLastExactly(size_t count) override {PARENT::OriginalRange.PopLastExactly(count);}
	};

	template<typename R> struct WrapperImpl: ImplFiller<R, typename InputRange<T>::template ImplFiller<R, Interface>>
	{
		typedef ImplFiller<R, typename InputRange<T>::template ImplFiller<R, Interface>> base;
		template<typename A> WrapperImpl(A&& range): base(core::forward<A>(range)) {}
		WrapperImpl* Clone() const override {return new WrapperImpl(base::OriginalRange);}
	};

	template<typename R, typename = Meta::EnableIf<
		IsBidirectionalRange<Meta::RemoveConstRef<R>>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, BidirectionalRange>::_
	>> forceinline BidirectionalRange(R&& range):
		mInterface(new WrapperImpl<Meta::RemoveConstRef<R>>(core::forward<R>(range))) {}

	template<typename R, typename = Meta::EnableIf<
		!IsInputRange<R>::_ && HasAsRange<R>::_
	>> forceinline BidirectionalRange(R range):
		mInterface(new WrapperImpl<AsRangeResult<R>>(AsRange(range))) {}

	template<size_t N> forceinline BidirectionalRange(T(&arr)[N]):
		mInterface(new WrapperImpl<ArrayRange<T>>(ArrayRange<T>(arr))) {}

	forceinline BidirectionalRange(BidirectionalRange&& rhs):
		mInterface(core::move(rhs.mInterface)) {}

	forceinline BidirectionalRange(const BidirectionalRange& rhs):
		mInterface(rhs.mInterface->Clone()) {}

	template<typename R, typename = Meta::EnableIf<
		!IsInputRange<R>::_ && HasAsRange<R>::_
	>> forceinline BidirectionalRange& operator=(R range)
	{
		mInterface = new WrapperImpl<AsRangeResult<R>>(AsRange(range));
		return *this;
	}

	template<typename R, typename = Meta::EnableIf<
		IsBidirectionalRangeOf<Meta::RemoveConstRef<R>, T>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, BidirectionalRange>::_
	>> forceinline BidirectionalRange& operator=(R&& range)
	{
		mInterface = new WrapperImpl<R>(core::move(range));
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

		forceinline WrapperImpl(R&& range): OriginalRange(core::move(OriginalRange)) {}

		bool Empty() const override {return OriginalRange.Empty();}
		T First() const override {return OriginalRange.First();}
		void PopFirst() override {OriginalRange.PopFirst();}

		void PopFirstN(size_t count) override {OriginalRange.PopFirstN(count);}
		void PopFirstExactly(size_t count) override {OriginalRange.PopFirstExactly(count);}

		WrapperImpl* Clone() const override {return new WrapperImpl(OriginalRange);}

		T OpIndex(size_t index) const override {return OriginalRange[index];}
	};

	template<typename R, typename = Meta::EnableIf<
		IsRandomAccessRangeOf<Meta::RemoveConstRef<R>, T>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, RandomAccessRange>::_
	>> forceinline RandomAccessRange(R&& range):
		mInterface(new WrapperImpl<Meta::RemoveConstRef<R>>(core::forward<R>(range))) {}

	template<typename R, typename = Meta::EnableIf<
		!IsInputRange<R>::_ && HasAsRange<R>::_
	>> forceinline RandomAccessRange(R range):
		mInterface(new WrapperImpl<AsRangeResult<R>>(AsRange(range))) {}

	template<size_t N> forceinline RandomAccessRange(T(&arr)[N]):
		mInterface(new WrapperImpl<ArrayRange<T>>(ArrayRange<T>(arr))) {}

	forceinline RandomAccessRange(RandomAccessRange&& rhs):
		mInterface(core::move(rhs.mInterface)) {}

	forceinline RandomAccessRange(const RandomAccessRange& rhs):
		mInterface(rhs.mInterface->Clone()) {}

	template<typename R, typename = Meta::EnableIf<
		!IsInputRange<R>::_ && HasAsRange<R>::_
	>> forceinline RandomAccessRange& operator=(R range)
	{
		mInterface = new WrapperImpl<AsRangeResult<R>>(AsRange(range));
		return *this;
	}

	template<typename R, typename = Meta::EnableIf<
		IsRandomAccessRangeOf<Meta::RemoveConstRef<R>, T>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, RandomAccessRange>::_
	>> forceinline RandomAccessRange& operator=(R&& range)
	{
		mInterface = new WrapperImpl<R>(core::move(range));
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

		forceinline WrapperImpl(R&& range): OriginalRange(core::move(OriginalRange)) {}

		bool Empty() const override {return OriginalRange.Empty();}
		T First() const override {return OriginalRange.First();}
		void PopFirst() override {OriginalRange.PopFirst();}

		void PopFirstN(size_t count) override {OriginalRange.PopFirstN(count);}
		void PopFirstExactly(size_t count) override {OriginalRange.PopFirstExactly(count);}

		WrapperImpl* Clone() const override {return new WrapperImpl(OriginalRange);}

		size_t Count() const override {return OriginalRange.Count();}

		T OpIndex(size_t index) const override {return OriginalRange[index];}
	};

	template<typename R, typename = Meta::EnableIf<
		IsFiniteRandomAccessRangeOf<Meta::RemoveConstRef<R>, T>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, FiniteRandomAccessRange>::_
	>> forceinline FiniteRandomAccessRange(R&& range):
		mInterface(new WrapperImpl<Meta::RemoveConstRef<R>>(core::forward<R>(range))) {}

	template<typename R, typename = Meta::EnableIf<
		!IsInputRange<R>::_ && HasAsRange<R>::_
	>> forceinline FiniteRandomAccessRange(R range):
		mInterface(new WrapperImpl<AsRangeResult<R>>(range.AsRange())) {}

	template<size_t N> forceinline FiniteRandomAccessRange(T(&arr)[N]):
		mInterface(new WrapperImpl<ArrayRange<T>>(ArrayRange<T>(arr))) {}

	forceinline FiniteRandomAccessRange(FiniteRandomAccessRange&& rhs):
		mInterface(core::move(rhs.mInterface)) {}

	forceinline FiniteRandomAccessRange(const FiniteRandomAccessRange& rhs):
		mInterface(rhs.mInterface->Clone()) {}

	template<typename R, typename = Meta::EnableIf<
		!IsInputRange<R>::_ && HasAsRange<R>::_
	>> forceinline FiniteRandomAccessRange& operator=(R range)
	{
		mInterface = new WrapperImpl<AsRangeResult<R>>(range.AsRange());
		return *this;
	}

	template<typename R, typename = Meta::EnableIf<
		IsFiniteRandomAccessRangeOf<Meta::RemoveConstRef<R>, T>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, FiniteRandomAccessRange>::_
	>> forceinline FiniteRandomAccessRange& operator=(R&& range)
	{
		mInterface = new WrapperImpl<R>(core::move(range));
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

		forceinline WrapperImpl(R&& range): OriginalRange(core::move(OriginalRange)) {}

		bool Empty() const override {return OriginalRange.Empty();}
		T First() const override {return OriginalRange.First();}
		void PopFirst() override {OriginalRange.PopFirst();}

		void PopFirstN(size_t count) override {OriginalRange.PopFirstN(count);}
		void PopFirstExactly(size_t count) override {OriginalRange.PopFirstExactly(count);}

		WrapperImpl* Clone() const override {return new WrapperImpl(OriginalRange);}

		size_t Count() const override {return OriginalRange.Count();}

		T OpIndex(size_t index) const override {return OriginalRange[index];}

		T* Data() const override {return OriginalRange.Data();}
	};

	template<typename R, typename = Meta::EnableIf<
		IsArrayRangeOf<Meta::RemoveConstRef<R>, T>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, AnyArrayRange>::_
	>> forceinline AnyArrayRange(R&& range):
		mInterface(new WrapperImpl<Meta::RemoveConstRef<R>>(core::forward<R>(range))) {}

	template<typename R, typename = Meta::EnableIf<
		!IsInputRange<R>::_ && HasAsRange<R>::_
	>> forceinline AnyArrayRange(R range):
		mInterface(new WrapperImpl<AsRangeResult<R>>(AsRange(range))) {}

	template<size_t N> forceinline AnyArrayRange(T(&arr)[N]):
		mInterface(new WrapperImpl<ArrayRange<T>>(ArrayRange<T>(arr))) {}

	forceinline AnyArrayRange(AnyArrayRange&& rhs):
		mInterface(core::move(rhs.mInterface)) {}

	forceinline AnyArrayRange(const AnyArrayRange& rhs):
		mInterface(rhs.mInterface->Clone()) {}

	template<typename R, typename = Meta::EnableIf<
		!IsInputRange<R>::_ && HasAsRange<R>::_
	>> forceinline AnyArrayRange& operator=(R range)
	{
		mInterface = new WrapperImpl<AsRangeResult<R>>(AsRange(range));
		return *this;
	}

	template<typename R, typename = Meta::EnableIf<
		IsArrayRangeOf<Meta::RemoveConstRef<R>, T>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, AnyArrayRange>::_
	>> forceinline AnyArrayRange& operator=(R&& range)
	{
		mInterface = new WrapperImpl<R>(core::move(range));
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

	T* Data() const {return mInterface->Data();}

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
