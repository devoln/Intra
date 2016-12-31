#pragma once

#include "Memory/SmartRef.h"
#include "Range/Operations.h"

namespace Intra { namespace Range {

INTRA_WARNING_PUSH_DISABLE_COPY_MOVE_IMPLICITLY_DELETED

template<typename T> struct InputRange
{
	struct Interface
	{
		virtual ~Interface() {}

		virtual bool Empty() const = 0;
		virtual T First() const = 0;
		virtual void PopFirst() = 0;
		virtual T GetNext() = 0;

		virtual void PopFirstN(size_t count) = 0;
		virtual void PopFirstExactly(size_t count) = 0;
	};

	template<typename R, typename PARENT> struct ImplFiller: PARENT
	{
		R OriginalRange;

		template<typename A> ImplFiller(A&& range):
			OriginalRange(Meta::Forward<A>(range)) {}

		bool Empty() const override {return OriginalRange.Empty();}
		T First() const override {return OriginalRange.First();}
		void PopFirst() override {OriginalRange.PopFirst();}
		T GetNext() override {auto&& result = OriginalRange.First(); OriginalRange.PopFirst(); return result;}

		void PopFirstN(size_t count) override {Range::PopFirstN(OriginalRange, count);}
		void PopFirstExactly(size_t count) override {Range::PopFirstExactly(OriginalRange, count);}
	};

	template<typename R> struct WrapperImpl: ImplFiller<R, Interface>
	{
		template<typename A> WrapperImpl(A&& range):
			ImplFiller<R, Interface>(Meta::Forward<A>(range)) {}
	};

	template<typename R, typename = Meta::EnableIf<
		(IsInputRange<R>::_ || HasAsRange<R>::_) &&
		ValueTypeIsConvertible<R, T>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, InputRange>::_
	>> forceinline InputRange(R&& range):
		mInterface(new WrapperImpl<Meta::RemoveReference<R>>(AsRange(Meta::Forward<R>(range)))) {}

	template<size_t N> forceinline InputRange(T(&arr)[N]):
		mInterface(new WrapperImpl<AsRangeResult<T(&)[N]>>(AsRange(arr))) {}

	template<typename R, typename = Meta::EnableIf<
		IsInputRange<R>::_ && ValueTypeIsConvertible<R, T>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, InputRange>::_
	>> forceinline InputRange& operator=(R&& range)
	{
		mInterface = new WrapperImpl<Meta::RemoveReference<R>>(Meta::Forward<R>(range));
		return *this;
	}

	template<size_t N> forceinline InputRange& operator=(T(&arr)[N])
	{
		operator=(AsRange(arr));
		return *this;
	}

	forceinline InputRange(InputRange&& rhs):
		mInterface(Meta::Move(rhs.mInterface)) {}

	InputRange(const InputRange& rhs) = delete;

	forceinline InputRange& operator=(InputRange&& rhs)
	{
		mInterface = Meta::Move(rhs.mInterface);
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


template<typename T> struct FiniteInputRange
{
	enum: bool {RangeIsFinite = true};

	typedef typename InputRange<T>::Interface Interface;
	template<typename R> using WrapperImpl = typename InputRange<T>::template WrapperImpl<R>;

	template<typename R, typename = Meta::EnableIf<
		(IsFiniteInputRange<R>::_ || HasAsRange<R>::_) &&
		ValueTypeIsConvertible<R, T>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, FiniteInputRange>::_
	>> forceinline FiniteInputRange(R&& range):
		mInterface(new WrapperImpl<Meta::RemoveConstRef<AsRangeResult<R>>>(
			AsRange(Meta::Forward<R>(range)))) {}

	/*template<typename R, typename = Meta::EnableIf<
		!IsFiniteInputRange<R>::_ && HasAsRange<R>::_
	>> forceinline FiniteInputRange(R&& range):
		mInterface(new WrapperImpl<AsRangeResult<R>>(AsRange(range))) {}*/

	template<size_t N> forceinline FiniteInputRange(T(&arr)[N]):
		mInterface(new WrapperImpl<AsRangeResult<T(&)[N]>>(AsRange(arr))) {}

	forceinline FiniteInputRange(FiniteInputRange&& rhs):
		mInterface(Meta::Move(rhs.mInterface)) {}

	FiniteInputRange(const FiniteInputRange& rhs) = delete;

	/*template<typename R, typename = Meta::EnableIf<
		!IsInputRange<R>::_ && HasAsRange<R>::_
	>> forceinline FiniteInputRange& operator=(R&& range)
	{
		mInterface = new WrapperImpl<AsRangeResult<R>>(AsRange(Meta::Forward<R>(range)));
		return *this;
	}*/

	template<typename R, typename = Meta::EnableIf<
		(IsFiniteInputRange<R>::_ || HasAsRange<R>::_ || Meta::IsArrayType<Meta::RemoveReference<R>>::_) &&
		ValueTypeIsConvertible<R, T>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, FiniteInputRange>::_
	>> forceinline FiniteInputRange& operator=(R&& range)
	{
		mInterface = new WrapperImpl<Meta::RemoveConstRef<AsRangeResult<R>>>(AsRange(Meta::Forward<R>(range)));
		return *this;
	}

	forceinline FiniteInputRange& operator=(FiniteInputRange&& rhs)
	{
		mInterface = Meta::Move(rhs.mInterface);
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

template<typename T> struct ForwardRange
{
	struct Interface: InputRange<T>::Interface
	{
		virtual Interface* Clone() const = 0;
	};

	template<typename R> struct WrapperImpl: InputRange<T>::template ImplFiller<R, Interface>
	{
		typedef typename InputRange<T>::template ImplFiller<R, Interface> base;
		template<typename A> WrapperImpl(A&& range): base(Meta::Forward<A>(range)) {}

		WrapperImpl* Clone() const override {return new WrapperImpl(base::OriginalRange);}
	};

	template<typename R, typename = Meta::EnableIf<
		IsForwardRange<R>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, ForwardRange>::_
	>> forceinline ForwardRange(R&& range):
		mInterface(new WrapperImpl<Meta::RemoveConstRef<R>>(Meta::Forward<R>(range))) {}

	template<typename R, typename = Meta::EnableIf<
		!IsInputRange<R>::_ && HasAsRange<R>::_
	>> forceinline ForwardRange(R range):
		mInterface(new WrapperImpl<AsRangeResult<R>>(AsRange(range))) {}

	template<size_t N> forceinline ForwardRange(T(&arr)[N]):
		mInterface(new WrapperImpl<ArrayRange<T>>(ArrayRange<T>(arr))) {}

	forceinline ForwardRange(ForwardRange&& rhs):
		mInterface(Meta::Move(rhs.mInterface)) {}

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
		IsForwardRange<R>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, ForwardRange>::_
	>> forceinline ForwardRange& operator=(R&& range)
	{
		mInterface = new WrapperImpl<R>(Meta::Move(range));
		return *this;
	}

	forceinline ForwardRange& operator=(const ForwardRange& range)
	{
		mInterface = range.mInterface->Clone();
		return *this;
	}

	forceinline ForwardRange& operator=(ForwardRange&& range)
	{
		mInterface = Meta::Move(range.mInterface);
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

template<typename T> struct FiniteForwardRange
{
	struct Interface: ForwardRange<T>::Interface
	{
		virtual Interface* Clone() const = 0;
		virtual size_t Count() const = 0;
	};

	template<typename R> struct WrapperImpl: InputRange<T>::template ImplFiller<R, Interface>
	{
		typedef typename InputRange<T>::template ImplFiller<R, Interface> base;
		template<typename A> WrapperImpl(A&& range): base(Meta::Forward<A>(range)) {}

		WrapperImpl* Clone() const override {return new WrapperImpl(base::OriginalRange);}
		size_t Count() const override {return base::OriginalRange.Count();}
	};

	template<typename R, typename = Meta::EnableIf<
		IsFiniteForwardRange<R>::_ &&
		ValueTypeIsConvertible<R, T>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, FiniteForwardRange>::_
	>> forceinline FiniteForwardRange(R&& range):
		mInterface(new WrapperImpl<Meta::RemoveConstRef<R>>(Meta::Forward<R>(range))) {}

	template<typename R, typename = Meta::EnableIf<
		!IsInputRange<R>::_ && HasAsRange<R>::_
	>> forceinline FiniteForwardRange(R range):
		mInterface(new WrapperImpl<AsRangeResult<R>>(AsRange(range))) {}

	template<size_t N> forceinline FiniteForwardRange(T(&arr)[N]):
		mInterface(new WrapperImpl<ArrayRange<T>>(ArrayRange<T>(arr))) {}

	forceinline FiniteForwardRange(FiniteForwardRange&& rhs):
		mInterface(Meta::Move(rhs.mInterface)) {}

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
		IsFiniteForwardRange<R>::_ && ValueTypeIsConvertible<R, T>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, FiniteForwardRange>::_
	>> forceinline FiniteForwardRange& operator=(R&& range)
	{
		mInterface = new WrapperImpl<R>(Meta::Move(range));
		return *this;
	}

	forceinline FiniteForwardRange& operator=(const FiniteForwardRange& range)
	{
		mInterface = range.mInterface->Clone();
		return *this;
	}

	forceinline FiniteForwardRange& operator=(FiniteForwardRange&& range)
	{
		mInterface = Meta::Move(range.mInterface);
		return *this;
	}

	forceinline bool Empty() const {return mInterface==null || mInterface->Empty();}
	forceinline T First() const {return mInterface->First();}
	forceinline void PopFirst() {mInterface->PopFirst();}

	forceinline void PopFirstN(size_t count) {mInterface->PopFirstN(count);}
	forceinline void PopFirstExactly(size_t count) {mInterface->PopFirstExactly(count);}

	forceinline size_t Length() const {return mInterface->Count();}

private:
	Memory::UniqueRef<Interface> mInterface;
};

template<typename T> struct BidirectionalRange
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
		template<typename A> WrapperImpl(A&& range): base(Meta::Forward<A>(range)) {}
		WrapperImpl* Clone() const override {return new WrapperImpl(base::OriginalRange);}
	};

	template<typename R, typename = Meta::EnableIf<
		IsBidirectionalRange<Meta::RemoveConstRef<R>>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, BidirectionalRange>::_
	>> forceinline BidirectionalRange(R&& range):
		mInterface(new WrapperImpl<Meta::RemoveConstRef<R>>(Meta::Forward<R>(range))) {}

	template<typename R, typename = Meta::EnableIf<
		!IsInputRange<R>::_ && HasAsRange<R>::_
	>> forceinline BidirectionalRange(R range):
		mInterface(new WrapperImpl<AsRangeResult<R>>(AsRange(range))) {}

	template<size_t N> forceinline BidirectionalRange(T(&arr)[N]):
		mInterface(new WrapperImpl<ArrayRange<T>>(ArrayRange<T>(arr))) {}

	forceinline BidirectionalRange(BidirectionalRange&& rhs):
		mInterface(Meta::Move(rhs.mInterface)) {}

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
		IsBidirectionalRange<R>::_ &&
		ValueTypeIsConvertible<R, T>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, BidirectionalRange>::_
	>> forceinline BidirectionalRange& operator=(R&& range)
	{
		mInterface = new WrapperImpl<R>(Meta::Move(range));
		return *this;
	}

	forceinline BidirectionalRange& operator=(const BidirectionalRange& range)
	{
		mInterface = range.mInterface->Clone();
		return *this;
	}

	forceinline BidirectionalRange& operator=(BidirectionalRange&& range)
	{
		mInterface = Meta::Move(range.mInterface);
		return *this;
	}

	forceinline bool Empty() const {return mInterface==null || mInterface->Empty();}
	forceinline T First() const {return mInterface->First();}
	forceinline void PopFirst() {mInterface->PopFirst();}

	forceinline void PopFirstN(size_t count) {mInterface->PopFirstN(count);}
	forceinline void PopFirstExactly(size_t count) {mInterface->PopFirstExactly(count);}

	forceinline size_t Length() const {return mInterface->Count();}

	forceinline T Last() const {return mInterface->Last();}
	forceinline void PopLast() {mInterface->PopLast();}
	forceinline void PopLastN(size_t count) {mInterface->PopLastN(count);}
	forceinline void PopLastExactly(size_t count) {mInterface->PopLastExactly(count);}

private:
	Memory::UniqueRef<Interface> mInterface;
};

template<typename T> struct RandomAccessRange
{
	struct Interface: ForwardRange<T>::Interface
	{
		virtual Interface* Clone() const = 0;
		virtual T OpIndex(size_t index) const = 0;
	};

	template<typename R> struct WrapperImpl: InputRange<T>::template ImplFiller<R, Interface>
	{
		typedef typename InputRange<T>::template ImplFiller<R, Interface> base;
		template<typename A> WrapperImpl(A&& range): base(Meta::Forward<A>(range)) {}

		WrapperImpl* Clone() const override {return new WrapperImpl(base::OriginalRange);}
		T OpIndex(size_t index) const override {return base::OriginalRange[index];}
	};

	template<typename R, typename = Meta::EnableIf<
		IsRandomAccessRange<R>::_ &&
		ValueTypeIsConvertible<R, T>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, RandomAccessRange>::_
	>> forceinline RandomAccessRange(R&& range):
		mInterface(new WrapperImpl<Meta::RemoveConstRef<R>>(Meta::Forward<R>(range))) {}

	template<typename R, typename = Meta::EnableIf<
		!IsInputRange<R>::_ && HasAsRange<R>::_
	>> forceinline RandomAccessRange(R range):
		mInterface(new WrapperImpl<AsRangeResult<R>>(AsRange(range))) {}

	template<size_t N> forceinline RandomAccessRange(Meta::RemoveReference<T>(&arr)[N]):
		mInterface(new WrapperImpl<ArrayRange<Meta::RemoveReference<T>>>(AsRange(arr))) {}

	forceinline RandomAccessRange(RandomAccessRange&& rhs):
		mInterface(Meta::Move(rhs.mInterface)) {}

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
		IsRandomAccessRange<R>::_ && ValueTypeIsConvertible<R, T>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, RandomAccessRange>::_
	>> forceinline RandomAccessRange& operator=(R&& range)
	{
		mInterface = new WrapperImpl<R>(Meta::Move(range));
		return *this;
	}

	forceinline RandomAccessRange& operator=(const RandomAccessRange& range)
	{
		mInterface = range.mInterface->Clone();
		return *this;
	}

	forceinline RandomAccessRange& operator=(RandomAccessRange&& range)
	{
		mInterface = Meta::Move(range.mInterface);
		return *this;
	}

	forceinline bool Empty() const {return mInterface==null || mInterface->Empty();}
	forceinline T First() const {return mInterface->First();}
	forceinline void PopFirst() {mInterface->PopFirst();}

	forceinline void PopFirstN(size_t count) {mInterface->PopFirstN(count);}
	forceinline void PopFirstExactly(size_t count) {mInterface->PopFirstExactly(count);}

	forceinline T operator[](size_t index) const {return mInterface->OpIndex(index);}

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

private:
	Memory::UniqueRef<Interface> mInterface;
};


template<typename T> struct FiniteRandomAccessRange
{
	struct Interface: BidirectionalRange<T>::Interface
	{
		virtual Interface* Clone() const = 0;
		virtual T OpIndex(size_t index) const = 0;
	};

	template<typename R> struct WrapperImpl:
		BidirectionalRange<R>::template ImplFiller<R, typename InputRange<T>::template ImplFiller<R, Interface>>
	{
		typedef typename BidirectionalRange<R>::template ImplFiller<R, typename InputRange<T>::template ImplFiller<R, Interface>> base;
		template<typename A> WrapperImpl(A&& range): base(Meta::Forward<A>(range)) {}

		WrapperImpl* Clone() const override {return new WrapperImpl(base::OriginalRange);}
		T OpIndex(size_t index) const override {return base::OriginalRange[index];}
		size_t Count() const override {return base::OriginalRange.Count();}
	};

	template<typename R, typename = Meta::EnableIf<
		IsFiniteRandomAccessRange<R>::_ && ValueTypeIsConvertible<R, T>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, FiniteRandomAccessRange>::_
	>> forceinline FiniteRandomAccessRange(R&& range):
		mInterface(new WrapperImpl<Meta::RemoveConstRef<R>>(Meta::Forward<R>(range))) {}

	template<typename R, typename = Meta::EnableIf<
		!IsInputRange<R>::_ && HasAsRange<R>::_
	>> forceinline FiniteRandomAccessRange(R range):
		mInterface(new WrapperImpl<AsRangeResult<R>>(range.AsRange())) {}

	template<size_t N> forceinline FiniteRandomAccessRange(Meta::RemoveReference<T>(&arr)[N]):
		mInterface(new WrapperImpl<ArrayRange<Meta::RemoveReference<T>>>(AsRange(arr))) {}

	forceinline FiniteRandomAccessRange(FiniteRandomAccessRange&& rhs):
		mInterface(Meta::Move(rhs.mInterface)) {}

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
		IsFiniteRandomAccessRange<R>::_ && ValueTypeIsConvertible<R, T>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, FiniteRandomAccessRange>::_
	>> forceinline FiniteRandomAccessRange& operator=(R&& range)
	{
		mInterface = new WrapperImpl<R>(Meta::Move(range));
		return *this;
	}

	forceinline FiniteRandomAccessRange& operator=(const FiniteRandomAccessRange& range)
	{
		mInterface = range.mInterface->Clone();
		return *this;
	}

	forceinline FiniteRandomAccessRange& operator=(FiniteRandomAccessRange&& range)
	{
		mInterface = Meta::Move(range.mInterface);
		return *this;
	}

	forceinline bool Empty() const {return mInterface==null || mInterface->Empty();}
	forceinline T First() const {return mInterface->First();}
	forceinline void PopFirst() {mInterface->PopFirst();}

	forceinline void PopFirstN(size_t count) {mInterface->PopFirstN(count);}
	forceinline void PopFirstExactly(size_t count) {mInterface->PopFirstExactly(count);}

	forceinline T Last() const {return mInterface->Last();}
	forceinline void PopLast() {mInterface->PopLast();}
	forceinline void PopLastN(size_t count) {mInterface->PopLastN(count);}
	forceinline void PopLastExactly(size_t count) {mInterface->PopLastExactly(count);}

	forceinline size_t Length() const {return mInterface->Count();}

	forceinline T operator[](size_t index) const {return mInterface->OpIndex(index);}

private:
	Memory::UniqueRef<Interface> mInterface;
};

template<typename T> struct AnyArrayRange
{
	struct Interface: FiniteRandomAccessRange<T>::Interface
	{
		virtual Interface* Clone() const = 0;
		virtual T* Data() const = 0;
	};

	template<typename R> struct WrapperImpl:
		BidirectionalRange<R>::template ImplFiller<R,
	typename InputRange<T>::template ImplFiller<R, Interface>>
	{
		typedef typename BidirectionalRange<R>::template ImplFiller<R,
		        typename InputRange<T>::template ImplFiller<R, Interface>> base;
		template<typename A> WrapperImpl(A&& range): base(Meta::Forward<A>(range)) {}

		WrapperImpl* Clone() const override {return new WrapperImpl(base::OriginalRange);}
		T OpIndex(size_t index) const override {return base::OriginalRange[index];}
		size_t Count() const override {return base::OriginalRange.Count();}
		Meta::RemoveReference<T>* Data() const override {return base::OriginalRange.Data();}
	};

	template<typename R, typename = Meta::EnableIf<
		IsArrayRange<R>::_ && ValueTypeIsConvertible<R, T>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, AnyArrayRange>::_
	>> forceinline AnyArrayRange(R&& range):
		mInterface(new WrapperImpl<Meta::RemoveConstRef<R>>(Meta::Forward<R>(range))) {}

	template<typename R, typename = Meta::EnableIf<
		!IsInputRange<R>::_ && HasAsRange<R>::_
	>> forceinline AnyArrayRange(R range):
		mInterface(new WrapperImpl<AsRangeResult<R>>(AsRange(range))) {}

	template<size_t N> forceinline AnyArrayRange(T(&arr)[N]):
		mInterface(new WrapperImpl<ArrayRange<T>>(ArrayRange<T>(arr))) {}

	forceinline AnyArrayRange(AnyArrayRange&& rhs):
		mInterface(Meta::Move(rhs.mInterface)) {}

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
		IsArrayRange<R>::_ && ValueTypeIsConvertible<R, T>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, AnyArrayRange>::_
	>> forceinline AnyArrayRange& operator=(R&& range)
	{
		mInterface = new WrapperImpl<R>(Meta::Forward<R>(range));
		return *this;
	}

	forceinline AnyArrayRange& operator=(const AnyArrayRange& range)
	{
		mInterface = range.mInterface->Clone();
		return *this;
	}

	forceinline AnyArrayRange& operator=(AnyArrayRange&& range)
	{
		mInterface = Meta::Move(range.mInterface);
		return *this;
	}

	forceinline bool Empty() const {return mInterface==null || mInterface->Empty();}
	forceinline T First() const {return mInterface->First();}
	forceinline void PopFirst() {mInterface->PopFirst();}

	forceinline void PopFirstN(size_t count) {mInterface->PopFirstN(count);}
	forceinline void PopFirstExactly(size_t count) {mInterface->PopFirstExactly(count);}

	forceinline T Last() const {return mInterface->Last();}
	forceinline void PopLast() {mInterface->PopLast();}
	forceinline void PopLastN(size_t count) {mInterface->PopLastN(count);}
	forceinline void PopLastExactly(size_t count) {mInterface->PopLastExactly(count);}

	forceinline size_t Length() const {return mInterface->Count();}

	forceinline T operator[](size_t index) const
	{return mInterface->OpIndex(index);}
	
	forceinline ArrayRange<T> operator()(size_t start, size_t end) const
	{return ArrayRange<T>(Data(), Length())(start, end);}

	T* Data() const {return mInterface->Data();}

private:
	Memory::UniqueRef<Interface> mInterface;
};

INTRA_WARNING_POP

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
