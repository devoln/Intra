#pragma once

namespace Intra { namespace Range {

template<typename R> class PolymorphicInputRange;

template<typename T> struct IInputRange: RangeMixin<IInputRange<T>, Meta::RemoveConstRef<T>, TypeEnum::Input, false>
{
private:
	typedef RangeMixin<IInputRange<T>, Meta::RemoveConstRef<T>, TypeEnum::Input, false> base;
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

template<typename R> class PolymorphicInputRange:
	public IInputRange<typename R::value_type>
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



template<typename R> class PolymorphicFiniteInputRange;

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

template<typename R> class PolymorphicFiniteInputRange:
	public IFiniteInputRange<typename R::value_type>
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



template<typename R> class PolymorphicFiniteForwardRange;

template<typename T> struct IFiniteForwardRange:
	FiniteForwardRangeMixin<IFiniteForwardRange<T>, T, IFiniteInputRange<T>>
{
private:
	typedef FiniteForwardRangeMixin<IFiniteForwardRange<T>, T, IFiniteInputRange<T>> base;
public:
	virtual Memory::UniqueRef<IFiniteForwardRange> Clone() = 0;

	template<typename R> static Memory::UniqueRef<IFiniteForwardRange> Wrap(R&& range)
	{
		return new PolymorphicFiniteForwardRange<R>(core::move(range));
	}

	template<typename R> static Memory::UniqueRef<IFiniteForwardRange> Wrap(R range)
	{
		return new PolymorphicFiniteForwardRange<R>(range);
	}
};

template<typename R> class PolymorphicFiniteForwardRange:
	public IFiniteForwardRange<typename R::value_type>
{
	R mRange;
public:
	PolymorphicFiniteForwardRange(R&& range): mRange(core::move(range)) {}
	PolymorphicFiniteForwardRange(const R& range): mRange(range) {}
	bool Empty() const override final {return R::Empty();}
	T First() const override final {return R::First();}
	void PopFirst() override final {R::PopFirst();}
	void PopFirstN(size_t count) override final {R::PopFirstN(count);}
	void PopFirstExactly(size_t count) override final {R::PopFirstExactly(count);}
	void FillAdvance(const T& value) override final {R::FillAdvance(value);}
	Memory::UniqueRef<IFiniteForwardRange> Clone() {return new PolymorphicFiniteForwardRange(*this);}
};

}}
