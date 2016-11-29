#pragma once

namespace Intra { namespace Range {

template<typename R, typename T=typename R::return_value_type> class PolymorphicInputRange;

template<typename T> struct IInputRange:
	RangeMixin<IInputRange<T>, T, TypeEnum::Input, false>
{
private:
	typedef RangeMixin<IInputRange<T>, T, TypeEnum::Input, false> base;
public:
	typedef Meta::RemoveConstRef<T> value_type;
	typedef T return_value_type;

	virtual ~IInputRange() {}

	virtual bool Empty() const = 0;
	virtual T First() const = 0;
	virtual void PopFirst() = 0;

	//Эти методы могут быть реализованы через методы выше, 
	//но чтобы избежать многократных виртуальных вызовов их лучше переопределить
	virtual void PopFirstN(size_t count) {base::PopFirstN(count);}
	virtual void PopFirstExactly(size_t count) {base::PopFirstExactly(count);}
};

template<typename R, typename T> class PolymorphicInputRange: public IInputRange<T>
{
	R mRange;
public:
	PolymorphicInputRange(R&& range): mRange(core::move(range)) {}

	bool Empty() const override final {return mRange.Empty();}
	T First() const override final {return mRange.First();}
	void PopFirst() override final {mRange.PopFirst();}

	void PopFirstN(size_t count) override final {mRange.PopFirstN(count);}
	void PopFirstExactly(size_t count) override final {mRange.PopFirstExactly(count);}
};



template<typename R, typename T=typename R::return_value_type> class PolymorphicFiniteInputRange;

template<typename T> struct IFiniteInputRange:
	FiniteInputRangeMixin<IFiniteInputRange<T>, T, IInputRange<T>>
{
private:
	typedef FiniteInputRangeMixin<IFiniteInputRange<T>, T, IInputRange<T>> base;
public:
	virtual void FillAdvance(const T& value) {base::FillAdvance(value);}

	template<typename R> static Memory::UniqueRef<IFiniteInputRange> Wrap(R&& range)
	{
		return new PolymorphicFiniteInputRange<R>(core::move(range));
	}
};

template<typename R, typename T> class PolymorphicFiniteInputRange:
	public IFiniteInputRange<T>
{
	R mRange;
public:
	PolymorphicFiniteInputRange(const PolymorphicFiniteInputRange& rhs) = delete;
	PolymorphicFiniteInputRange(PolymorphicFiniteInputRange&& rhs): mRange(core::move(rhs.mRange)) {}
	PolymorphicFiniteInputRange(R&& range): mRange(core::move(range)) {}
	bool Empty() const override final {return mRange.Empty();}
	T First() const override final {return mRange.First();}
	void PopFirst() override final {mRange.PopFirst();}
	void PopFirstN(size_t count) override final {mRange.PopFirstN(count);}
	void PopFirstExactly(size_t count) override final {mRange.PopFirstExactly(count);}
	void FillAdvance(const T& value) override final {mRange.FillAdvance(value);}
};



template<typename R, typename T=typename R::return_value_type> class PolymorphicFiniteForwardRange;

template<typename T> struct IFiniteForwardRange:
	FiniteForwardRangeMixin<IFiniteForwardRange<T>, T, IFiniteInputRange<T>>
{
private:
	typedef FiniteForwardRangeMixin<IFiniteForwardRange<T>, T, IFiniteInputRange<T>> base;
public:
	virtual Memory::UniqueRef<IFiniteForwardRange> Clone() = 0;
	virtual size_t Count() const = 0;

	template<typename R> static Memory::UniqueRef<IFiniteForwardRange> Wrap(R range)
	{
		return new PolymorphicFiniteForwardRange<R>(range);
	}
};

template<typename R, typename T> class PolymorphicFiniteForwardRange: public IFiniteForwardRange<T>
{
	R mRange;
public:
	PolymorphicFiniteForwardRange(R&& range): mRange(core::move(range)) {}
	PolymorphicFiniteForwardRange(const R& range): mRange(range) {}
	bool Empty() const override final {return mRange.Empty();}
	T First() const override final {return mRange.First();}
	void PopFirst() override final {mRange.PopFirst();}
	void PopFirstN(size_t count) override final {mRange.PopFirstN(count);}
	void PopFirstExactly(size_t count) override final {mRange.PopFirstExactly(count);}
	void FillAdvance(const T& value) override final {mRange.FillAdvance(value);}

	Memory::UniqueRef<IFiniteForwardRange<T>> Clone() override final
	{
		return new PolymorphicFiniteForwardRange(*this);
	}

	size_t Count() const override final {return mRange.Count();}
};



template<typename R, typename T=typename R::return_value_type> class PolymorphicBidirectionalRange;

template<typename T> struct IBidirectionalRange:
	BidirectionalRangeMixin<IBidirectionalRange<T>, T, IFiniteForwardRange<T>>
{
private:
	typedef BidirectionalRangeMixin<IBidirectionalRange<T>, T, IFiniteForwardRange<T>> base;
public:
	virtual T Last() const = 0;
	virtual void PopLast() = 0;
	virtual void PopLastN(size_t count) = 0;
	virtual void PopLastExactly(size_t count) = 0;

	template<typename R> static Memory::UniqueRef<IBidirectionalRange> Wrap(R range)
	{
		return new PolymorphicFiniteForwardRange<R, T>(range);
	}
};

template<typename R, typename T> class PolymorphicBidirectionalRange: public IBidirectionalRange<T>
{
	R mRange;
public:
	PolymorphicBidirectionalRange(R&& range): mRange(core::move(range)) {}
	PolymorphicBidirectionalRange(const R& range): mRange(range) {}
	
	bool Empty() const override final {return mRange.Empty();}

	T First() const override final {return mRange.First();}
	void PopFirst() override final {mRange.PopFirst();}
	void PopFirstN(size_t count) override final {mRange.PopFirstN(count);}
	void PopFirstExactly(size_t count) override final {mRange.PopFirstExactly(count);}

	T Last() const override final {return mRange.Last();}
	void PopLast() override final {mRange.PopLast();}
	void PopLastN(size_t count) override final {mRange.PopLastN(count);}
	void PopLastExactly(size_t count) override final {mRange.PopLastExactly(count);}

	void FillAdvance(const T& value) override final {mRange.FillAdvance(value);}
	Memory::UniqueRef<IBidirectionalRange<T>> Clone() override final {return new PolymorphicBidirectionalRange(*this);}
	size_t Count() const override final {return mRange.Count();}
};



template<typename R, typename T=typename R::return_value_type> class PolymorphicIndexableRange;

template<typename T> struct IIndexableRange: IBidirectionalRange<T>
{
public:
	virtual T OpIndex(size_t index) const = 0;

	T operator[](size_t index) const {return OpIndex(index);}

	template<typename R> static Memory::UniqueRef<IIndexableRange> Wrap(R range)
	{
		return new PolymorphicIndexableRange<R, T>(range);
	}
};

template<typename R, typename T> class PolymorphicIndexableRange: public IIndexableRange<T>
{
	R mRange;
public:
	PolymorphicIndexableRange(R&& range): mRange(core::move(range)) {}
	PolymorphicIndexableRange(const R& range): mRange(range) {}
	
	bool Empty() const override final {return mRange.Empty();}

	T First() const override final {return mRange.First();}
	void PopFirst() override final {mRange.PopFirst();}
	void PopFirstN(size_t count) override final {mRange.PopFirstN(count);}
	void PopFirstExactly(size_t count) override final {mRange.PopFirstExactly(count);}

	T Last() const override final {return mRange.Last();}
	void PopLast() override final {mRange.PopLast();}
	void PopLastN(size_t count) override final {mRange.PopLastN(count);}
	void PopLastExactly(size_t count) override final {mRange.PopLastExactly(count);}

	void FillAdvance(const T& value) override final {mRange.FillAdvance(value);}
	Memory::UniqueRef<IIndexableRange<T>> Clone() override final {return new PolymorphicIndexableRange(*this);}
	size_t Count() const override final {return mRange.Count();}

	T OpIndex(size_t index) const override final {return mRange[index];}
};


}}
