#include "Data/Object.h"
#include "Range/Compositors/ZipKV.h"
#include "Range/Decorators/Map.h"

namespace Intra { namespace Data {

Object::StringMap<double> Object::GetNumbers() const
{
	return Range::Map(mNumbers, [](const KeyValuePair<const String&, const double&>& p) {
		return MapElement<double>{p.Key, p.Value};
	});
}

StringView Object::GetString(StringView key, StringView defaultValue) const
{
	size_t index = mStrings.FindIndex(key);
	if(index==mStrings.Count()) return defaultValue;
	return mStrings.Value(index);
}

Object::StringMap<StringView> Object::GetStrings() const
{
	return Range::Map(mStrings, [](const KeyValuePair<const String&, const String&>& p) {
		return MapElement<StringView>{p.Key, p.Value};
	});
}

Object::StringMap<const IConstObject&> Object::GetObjects() const
{
	return Range::Map(mObjects, [](const KeyValuePair<const String&, const Object&>& p) {
		return MapElement<const IConstObject&>{p.Key, p.Value};
	});
}


Object::StringMap<Object::Collection<double>> Object::GetNumberArrays() const
{
	return Range::Map(mNumberArrays, [](const KeyValuePair<const String&, const Array<double>&>& p) {
		return MapElement<Collection<double>>{p.Key, p.Value.AsRange()};
	});
}

Object::StringMap<Object::Collection<StringView>> Object::GetStringArrays() const
{
	return Range::Map(mStringArrays, [](const KeyValuePair<const String&, const Array<String>&>& p) {
		return MapElement<Collection<StringView>>{p.Key, p.Value.AsRange()};
	});
}

Object::StringMap<Object::Collection<const IConstObject&>> Object::GetObjectArrays() const
{
	return Range::Map(mObjectArrays, [](const KeyValuePair<const String&, const Array<Object>&>& p) {
		return MapElement<Collection<const IConstObject&>>{p.Key, p.Value.AsRange()};
	});
}

Object& Object::operator=(null_t)
{
	mNumbers = null;
	mStrings = null;
	mObjects = null;
	mNumberArrays = null;
	mStringArrays = null;
	mObjectArrays = null;
	return *this;
}

Object& Object::operator=(Object&& rhs)
{
	mNumbers = Meta::Move(rhs.mNumbers);
	mStrings = Meta::Move(rhs.mStrings);
	mObjects = Meta::Move(rhs.mObjects);
	mNumberArrays = Meta::Move(rhs.mNumberArrays);
	mStringArrays = Meta::Move(rhs.mStringArrays);
	mObjectArrays = Meta::Move(rhs.mObjectArrays);
	return *this;
}

}}

