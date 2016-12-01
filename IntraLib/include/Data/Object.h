#pragma once

#include "Containers/Array.h"
#include "Algorithms/Polymorphic.h"

namespace Intra { namespace Data {

class IConstObject
{
public:
	virtual double GetNumber(StringView key, double defaultValue=0) const = 0;
	virtual bool NumberExists(StringView key) const = 0;

	virtual StringView GetString(StringView key, StringView defaultValue=null) const = 0;
	virtual bool StringExists(StringView key) const = 0;

	virtual const IConstObject& GetObject(StringView key) const = 0;
	virtual bool ObjectExists(StringView key) const = 0;


	virtual ArrayRange<const double> GetNumberArray(StringView key) const = 0;
	virtual bool NumberArrayExists(StringView key) const = 0;

	virtual ArrayRange<const String> GetStringArray(StringView key) const = 0;
	virtual bool StringArrayExists(StringView key) const = 0;

	virtual Range::IndexableRangeHolder<const IConstObject&> GetObjectArray(StringView key) const = 0;
	virtual bool ObjectArrayExists(StringView key) const = 0;


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

	StringView GetString(StringView key, StringView defaultValue=null) const override final
	{
		size_t index = mStrings.FindIndex(key);
		if(index==mStrings.Count()) return defaultValue;
		return mStrings.Value(index);
	}

	String& StringValue(StringView key) {return mStrings[key];}
	bool StringExists(StringView key) const override final {return mStrings.KeyExists(key);}

	const Object& GetObject(StringView key) const override final {return mObjects.Get(key);}
	Object& ObjectValue(StringView key) {return mObjects[key];}
	bool ObjectExists(StringView key) const override final {return mObjects.KeyExists(key);}

	ArrayRange<const double> GetNumberArray(StringView key) const override final {return mNumberArrays.Get(key);}
	Array<double>& NumberArray(StringView key) {return mNumberArrays[key];}
	bool NumberArrayExists(StringView key) const override final {return mNumberArrays.KeyExists(key);}

	ArrayRange<const String> GetStringArray(StringView key) const override final {return mStringArrays.Get(key);}
	Array<String>& StringArray(StringView key) {return mStringArrays[key];}
	bool StringArrayExists(StringView key) const {return mStringArrays.KeyExists(key);}

	Range::IndexableRangeHolder<const IConstObject&> GetObjectArray(StringView key) const override final
	{
		return mObjectArrays.Get(key);
	}
	Array<Object>& ObjectArray(StringView key) {return mObjectArrays[key];}
	bool ObjectArrayExists(StringView key) const override final {return mObjectArrays.KeyExists(key);}


	Object() {}
	Object(const Object& rhs) = default;
	Object(Object&& rhs) = default;

	Object& operator=(null_t)
	{
		mNumbers = null;
		mStrings = null;
		mObjects = null;
		mNumberArrays = null;
		mStringArrays = null;
		mObjectArrays = null;
		return *this;
	}

	Object& operator=(const Object& rhs) = default;
	Object& operator=(Object&& rhs) = default;
};

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

	Range::IndexableRangeHolder<const IConstObject&> GetObjectArray(StringView key) const
	{
		return mObjectArrays.Get(key);
	}
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

}}
