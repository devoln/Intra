#pragma once

#include "Platform/CppFeatures.h"
#include "Range/Generators/StringView.h"
#include "Container/Sequential/Array.h"
#include "Container/Sequential/String.h"
#include "Container/Associative/LinearMap.h"
#include "Range/Polymorphic/FiniteRandomAccessRange.h"
#include "Data/Serialization/TextSerializer.h"

namespace Intra { namespace Data {

class IConstObject
{
public:
	template<typename V> using Collection = FiniteRandomAccessRange<V>;
	template<typename V> using MapElement = Meta::KeyValuePair<StringView, V>;
	template<typename V> using StringMap = Collection<MapElement<V>>;

	virtual ~IConstObject() {}

	virtual double GetNumber(StringView key, double defaultValue=0) const = 0;
	virtual bool NumberExists(StringView key) const = 0;
	virtual StringMap<double> GetNumbers() const = 0;

	virtual StringView GetString(StringView key, StringView defaultValue=null) const = 0;
	virtual bool StringExists(StringView key) const = 0;
	virtual StringMap<StringView> GetStrings() const = 0;

	virtual const IConstObject& GetObject(StringView key) const = 0;
	virtual bool ObjectExists(StringView key) const = 0;
	virtual StringMap<const IConstObject&> GetObjects() const = 0;


	virtual ArrayRange<const double> GetNumberArray(StringView key) const = 0;
	virtual bool NumberArrayExists(StringView key) const = 0;
	virtual StringMap<Collection<double>> GetNumberArrays() const = 0;

	virtual ArrayRange<const String> GetStringArray(StringView key) const = 0;
	virtual bool StringArrayExists(StringView key) const = 0;
	virtual StringMap<Collection<StringView>> GetStringArrays() const = 0;

	virtual Collection<const IConstObject&> GetObjectArray(StringView key) const = 0;
	virtual bool ObjectArrayExists(StringView key) const = 0;
	virtual StringMap<Collection<const IConstObject&>> GetObjectArrays() const = 0;


	template<typename T> Meta::EnableIf<
		Meta::IsFloatType<T>::_ || Meta::IsIntegralType<T>::_,
	T> Get(StringView key, T defaultValue=T()) const
	{return T(GetDouble(key, defaultValue));}
};

class Object: public IConstObject
{
private:
	LinearMap<String, double> mNumbers;
	LinearMap<String, String> mStrings;
	LinearMap<String, Object> mObjects;
	LinearMap<String, Array<double>> mNumberArrays;
	LinearMap<String, Array<String>> mStringArrays;
	LinearMap<String, Array<Object>> mObjectArrays;

public:
	double GetNumber(StringView key, double defaultValue=0) const override final {return mNumbers.Get(key, defaultValue);}
	double& NumberValue(StringView key) {return mNumbers[key];}
	bool NumberExists(StringView key) const override final {return mNumbers.KeyExists(key);}
	StringMap<double> GetNumbers() const override final;


	StringView GetString(StringView key, StringView defaultValue=null) const override final;
	String& StringValue(StringView key) {return mStrings[key];}
	bool StringExists(StringView key) const override final {return mStrings.KeyExists(key);}
	StringMap<StringView> GetStrings() const override final;


	const Object& GetObject(StringView key) const override final {return mObjects.Get(key);}
	Object& ObjectValue(StringView key) {return mObjects[key];}
	bool ObjectExists(StringView key) const override final {return mObjects.KeyExists(key);}
	StringMap<const IConstObject&> GetObjects() const override final;


	ArrayRange<const double> GetNumberArray(StringView key) const override final {return mNumberArrays.Get(key);}
	Array<double>& NumberArray(StringView key) {return mNumberArrays[key];}
	bool NumberArrayExists(StringView key) const override final {return mNumberArrays.KeyExists(key);}
	StringMap<Collection<double>> GetNumberArrays() const override final;


	ArrayRange<const String> GetStringArray(StringView key) const override final {return mStringArrays.Get(key);}
	Array<String>& StringArray(StringView key) {return mStringArrays[key];}
	bool StringArrayExists(StringView key) const override final {return mStringArrays.KeyExists(key);}
	StringMap<Collection<StringView>> GetStringArrays() const override final;


	Collection<const IConstObject&> GetObjectArray(StringView key) const override final
	{return mObjectArrays.Get(key);}

	Array<Object>& ObjectArray(StringView key) {return mObjectArrays[key];}
	bool ObjectArrayExists(StringView key) const override final {return mObjectArrays.KeyExists(key);}
	StringMap<Collection<const IConstObject&>> GetObjectArrays() const override final;


	Object() {}
	Object(const Object& rhs) = default;

	Object& operator=(null_t);
	Object& operator=(const Object& rhs) = default;

	//MSVC 2013 не поддерживает генерацию конструктора перемещения
	Object(Object&& rhs) {operator=(Meta::Move(rhs));}
	Object& operator=(Object&& rhs);
};

/*template<typename O>
GenericTextSerializer<O>& operator<<(GenericTextSerializer<O>& serializer, const IConstObject& src)
{
	//serializer.
	return serializer;
}*/


#if INTRA_DISABLED
class ObjectCRef: public IConstObject
{
private:
	LinearMap<String, double> mNumbers;
	LinearMap<String, String> mStrings;
	LinearMap<String, Object> mObjects;
	LinearMap<String, Array<double>> mNumberArrays;
	LinearMap<String, Array<String>> mStringArrays;
	LinearMap<String, Array<Object>> mObjectArrays;

public:
	double GetNumber(StringView key, double defaultValue=0) const override final {return mNumbers.Get(key, defaultValue);}
	bool NumberExists(StringView key) const override final {return mNumbers.KeyExists(key);}

	StringView GetString(StringView key, StringView defaultValue=null) const override final {return mStrings.Get(key, defaultValue);}
	bool StringExists(StringView key) const override final {return mStrings.KeyExists(key);}

	const IConstObject& GetObject(StringView key) const override final {return mObjects.Get(key);}
	bool ObjectExists(StringView key) const override final {return mObjects.KeyExists(key);}

	ArrayRange<const double> GetNumberArray(StringView key) const override final {return mNumberArrays.Get(key);}
	bool NumberArrayExists(StringView key) const override final {return mNumberArrays.KeyExists(key);}

	ArrayRange<const String> GetStringArray(StringView key) const override final {return mStringArrays.Get(key);}
	bool StringArrayExists(StringView key) const override final {return mStringArrays.KeyExists(key);}

	FiniteRandomAccessRange<const IConstObject&> GetObjectArray(StringView key) const override final
	{return mObjectArrays.Get(key);}

	bool ObjectArrayExists(StringView key) const override final {return mObjectArrays.KeyExists(key);}


	ObjectCRef() {}
	ObjectCRef(const ObjectCRef& rhs) = default;
	ObjectCRef(ObjectCRef&& rhs) = default;

	ObjectCRef& operator=(null_t)
	{
		mNumbers = null;
		mStrings = null;
		mObjects = null;
		mNumberArrays = null;
		mStringArrays = null;
		mObjectArrays = null;
		return *this;
	}

	ObjectCRef& operator=(const ObjectCRef& rhs) = default;
	ObjectCRef& operator=(ObjectCRef&& rhs) = default;
};

#endif

}}
