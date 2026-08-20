#pragma once

#include <cstddef>

namespace Intra { namespace Container {

/// Minimal replacement for IntraX's DynamicBlob, storing polymorphic objects
/// derived from T (T must have a virtual destructor).
///
/// Slots are stable: Delete() leaves a null tombstone and keeps the slot, so
/// external uint16 indices (e.g. MidiSynth's note->sampler map) stay valid.
/// Note: freed slots are not reused yet; Add() always appends. This matches
/// the fixed-slot voice design and is fine for bounded MIDI note counts.
template<typename T, size_t Alignment = alignof(T), typename OffsetType = unsigned>
class DynamicBlob
{
	T** mItems = nullptr;
	size_t mCount = 0;      // number of allocated slots
	size_t mCapacity = 0;
	size_t mLiveCount = 0;  // number of non-null slots

	void Grow()
	{
		const size_t newCapacity = mCapacity == 0 ? 8 : mCapacity * 2;
		T** newItems = static_cast<T**>(::operator new(newCapacity * sizeof(T*)));
		for(size_t i = 0; i < mCount; i++) newItems[i] = mItems[i];
		::operator delete(mItems);
		mItems = newItems;
		mCapacity = newCapacity;
	}

public:
	DynamicBlob(size_t = 0) {}
	~DynamicBlob() {Clear(); ::operator delete(mItems);}

	template<typename T1, typename... Args> T1& Add(Args&&... args)
	{
		if(mCount == mCapacity) Grow();
		T1* const p = new T1(static_cast<Args&&>(args)...);
		mItems[mCount++] = p;
		mLiveCount++;
		return *p;
	}

	T* operator[](size_t i) const {return mItems[i];}
	T& Get(size_t i) const {return *mItems[i];}
	size_t Length() const {return mCount;}
	bool Empty() const {return mLiveCount == 0;}

	void Delete(size_t i)
	{
		if(mItems[i])
		{
			delete mItems[i];
			mItems[i] = nullptr;
			mLiveCount--;
		}
	}

	void Clear()
	{
		for(size_t i = 0; i < mCount; i++)
		{
			if(!mItems[i]) continue;
			delete mItems[i];
			mItems[i] = nullptr;
		}
		mCount = 0;
		mLiveCount = 0;
	}

	/// Отдать владение элементом наружу, не удаляя его.
	T* Release(size_t i)
	{
		T* const p = mItems[i];
		if(p) {mItems[i] = nullptr; mLiveCount--;}
		return p;
	}

	struct Range
	{
		DynamicBlob* Blob;
		size_t Index;

		void SkipNulls() {while(Index < Blob->mCount && !Blob->mItems[Index]) Index++;}

		bool Empty() {SkipNulls(); return Index >= Blob->mCount;}
		T& Next() {SkipNulls(); return *Blob->mItems[Index++];}
	};

	Range AsRange() {return {this, 0};}
};

}}
