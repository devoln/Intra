#pragma once

#include "BinarySerializer.h"
#include "BinaryDeserializer.h"
#include "TextSerializer.h"
#include "TextDeserializer.h"

namespace Intra { namespace Data {

//! Интерфейс для сериализации runtime класса или структуры (не-POD)
struct IDynamicSerializer
{
	virtual ~IDynamicSerializer() {}

	virtual void operator()(DummyBinarySerializer& serializer, const void* src) const = 0;

	virtual void operator()(BinarySerializer& serializer, const void* src) const = 0;

	//Реализация должна корректно преобразовать тип dst и предполагать, что объект уже сконструирован
	virtual void operator()(BinaryDeserializer& deserializer, void* dst) const = 0;
};


template<typename T> struct DynamicSerializer: IDynamicSerializer
{
	void operator()(DummyBinarySerializer& serializer, const void* src) const override final
	{
		serializer(*reinterpret_cast<const T*>(src));
	}

	void operator()(BinarySerializer& serializer, const void* src) const override final
	{
		serializer(*reinterpret_cast<const T*>(src));
	}

	void operator()(BinaryDeserializer& deserializer, void* dst) const override final
	{
		deserializer(*reinterpret_cast<T*>(dst));
	}
};

}}

