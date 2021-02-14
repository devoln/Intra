#pragma once

#include "IntraX/Container/Sequential/String.h"
#include "Intra/Range/StringView.h"
#include "IntraX/Utils/Unique.h"
#include "Formatter.h"
#include "IntraX/Container/Utility/SparseArray.h"
#include "Intra/Range/Search/Single.h"

#ifndef INTRA_NO_CONCURRENCY
#include "Intra/Concurrency/Atomic.h"
#endif

namespace Intra { INTRA_BEGIN
class FormattedWriter
{
public:
	struct FormattedStream
	{
		bool IsOwner() const {return mStream != nullptr;}

		~FormattedStream() {clear();}

		FormattedStream(Unique<IOutputStream> stream, Unique<IFormatter> formatter):
			mStream(stream.Release()), mFormatter(formatter.Release())
		{
			INTRA_ASSERT(stream != nullptr || formatter == nullptr);
		}

		FormattedStream(OutputStream stream, Unique<IFormatter> formatter):
			FormattedStream(Move(stream.Stream), Move(formatter)) {}

		FormattedStream(FormattedWriter* writerRef):
			mStream(nullptr), mWriter(writerRef) {mWriter->incRef();}

		bool operator==(decltype(nullptr)) const {return mStream == nullptr && mFormatter == nullptr;}
		bool operator!=(decltype(nullptr)) const {return !operator==(nullptr);}

		FormattedStream(decltype(nullptr)=nullptr):
			mStream(nullptr), mFormatter(nullptr) {}

		FormattedStream(FormattedStream&& rhs) noexcept:
			mStream(rhs.mStream), mFormatter(rhs.mFormatter)
		{
			rhs.mStream = nullptr;
			rhs.mFormatter = nullptr;
		}

		FormattedStream(const FormattedStream& rhs) = delete;

	private:
		void clear()
		{
			if(!IsOwner())
			{
				if(mWriter) mWriter->decRef();
				return;
			}
			if(mFormatter)
			{
				mFormatter->CleanUp(*mStream);
				delete mFormatter;
			}
			delete mStream;
		}

		FormattedStream& operator=(FormattedStream&& fs) noexcept
		{
			clear();
			mStream = fs.mStream;
			mFormatter = fs.mFormatter;
			fs.mStream = nullptr;
			fs.mFormatter = nullptr;
			return *this;
		}

		FormattedStream& operator=(const FormattedStream&) = delete;

		void reset()
		{
			clear();
			mStream = nullptr;
			mFormatter = nullptr;
		}

		friend class FormattedWriter;

		IOutputStream* mStream;
		union
		{
			IFormatter* mFormatter;
			FormattedWriter* mWriter;
		};
	};

private:
	Array<FormattedStream> mFormattedStreams;


#if(!defined(INTRA_NO_CONCURRENCY) && INTRA_LIBRARY_ATOMIC != INTRA_LIBRARY_ATOMIC_None)
	AtomicInt mRefCount;
	unsigned incRef() {return unsigned(mRefCount.GetIncrementRelaxed());}
	unsigned getRC() {return unsigned(mRefCount.GetRelaxed());}
	bool decRef() {return mRefCount.DecrementRelaxed() == 0;}
#else
	unsigned incRef() {return mRefCount++;}
	unsigned getRC() {return mRefCount;}
	bool decRef() {return --mRefCount == 0;}
	unsigned mRefCount;
#endif

public:
	FormattedWriter(decltype(nullptr)=nullptr) {}

	explicit FormattedWriter(OutputStream stream)
	{
		mFormattedStreams.EmplaceLast(Move(stream.Stream), new BasicFormatter);
	}

	explicit FormattedWriter(FormattedWriter* writerRef)
	{
		mFormattedStreams.EmplaceLast(writerRef);
	}

	FormattedWriter(OutputStream stream, IFormatter* formatter)
	{
		mFormattedStreams.EmplaceLast(Move(stream.Stream), Move(formatter));
	}

	FormattedWriter(FormattedWriter&& rhs):
		mFormattedStreams(Move(rhs.mFormattedStreams))
	{
		INTRA_ASSERT(rhs.getRC() == 0);
	}

	~FormattedWriter()
	{
		INTRA_ASSERT(getRC() == 0);
		operator=(nullptr);
	}

	FormattedWriter& operator=(FormattedWriter&& rhs)
	{
		if(this == &rhs) return *this;
		INTRA_ASSERT(getRC() == 0);
		INTRA_ASSERT(rhs.getRC() == 0);
		mFormattedStreams = Move(rhs.mFormattedStreams);
		return *this;
	}

	FormattedWriter& operator=(decltype(nullptr))
	{
		mFormattedStreams = nullptr;
		return *this;
	}

	index_t Attach(OutputStream stream)
	{
		const auto index = CountUntil(mFormattedStreams, nullptr);
		if(index == mFormattedStreams.Length())
			mFormattedStreams.EmplaceLast(Move(stream), new BasicFormatter);
		else mFormattedStreams[index] = {Move(stream.Stream), new BasicFormatter};
		return index;
	}

	index_t Attach(FormattedWriter* writerRef)
	{
		const auto index = CountUntil(mFormattedStreams, nullptr);
		if(index == mFormattedStreams.Length())
			mFormattedStreams.EmplaceLast(writerRef);
		else mFormattedStreams[index] = {writerRef};
		return index;
	}

	index_t Attach(FormattedWriter&& writer)
	{
		const auto index = mFormattedStreams.Length();
		for(auto& fs: writer.mFormattedStreams)
		{
			if(fs == nullptr) continue;
			mFormattedStreams.AddLast(Move(fs));
		}
		return index;
	}

	index_t Attach(OutputStream stream, IFormatter* formatter)
	{
		const auto index = CountUntil(mFormattedStreams, nullptr);
		if(index == mFormattedStreams.Length())
			mFormattedStreams.EmplaceLast(Move(stream.Stream), Move(formatter));
		else mFormattedStreams[index] = {Move(stream.Stream), Move(formatter)};
		return index;
	}


	bool operator==(decltype(nullptr)) const
	{
		for(auto& fs: mFormattedStreams)
			if(fs != nullptr) return false;
		return true;
	}

	bool operator!=(decltype(nullptr)) const {return !operator==(nullptr);}

	FormattedWriter& PushFont(Vec3 color, float size=3,
		bool bold=false, bool italic=false, bool underline=false, bool strike=false)
	{return PushFont({color, size, bold, italic, underline, strike});}

	FormattedWriter& PushFont(const FontDesc& newFont)
	{
		for(auto& fs: mFormattedStreams)
		{
			if(fs.IsOwner()) fs.mFormatter->PushFont(*fs.mStream, newFont);
			else fs.mWriter->PushFont(newFont);
		}
		return *this;
	}

	FormattedWriter& PopFont()
	{
		for(auto& fs: mFormattedStreams)
		{
			if(fs.IsOwner()) fs.mFormatter->PopFont(*fs.mStream);
			else fs.mWriter->PopFont();
		}
		return *this;
	}

	FormattedWriter& BeginSpoiler(StringView label)
	{
		for(auto& fs: mFormattedStreams)
		{
			if(fs.IsOwner()) fs.mFormatter->BeginSpoiler(*fs.mStream, label);
			else fs.mWriter->BeginSpoiler(label);
		}
		return *this;
	}

	FormattedWriter& BeginSpoiler() {return BeginSpoiler("Show");}

	FormattedWriter& EndSpoiler()
	{
		for(auto& fs: mFormattedStreams)
		{
			if(fs.IsOwner()) fs.mFormatter->EndSpoiler(*fs.mStream);
			else fs.mWriter->EndSpoiler();
		}
		return *this;
	}

	FormattedWriter& BeginCode()
	{
		for(auto& fs: mFormattedStreams)
		{
			if(fs.IsOwner()) fs.mFormatter->BeginCode(*fs.mStream);
			else fs.mWriter->BeginCode();
		}
		return *this;
	}

	FormattedWriter& EndCode()
	{
		for(auto& fs: mFormattedStreams)
		{
			if(fs.IsOwner()) fs.mFormatter->EndCode(*fs.mStream);
			else fs.mWriter->EndCode();
		}
		return *this;
	}

	FormattedWriter& PushStyle(StringView style)
	{
		for(auto& fs: mFormattedStreams)
		{
			if(fs.IsOwner()) fs.mFormatter->PushStyle(*fs.mStream, style);
			else fs.mWriter->PushStyle(style);
		}
		return *this;
	}

	FormattedWriter& PopStyle()
	{
		for(auto& fs: mFormattedStreams)
		{
			if(fs.IsOwner()) fs.mFormatter->PopStyle(*fs.mStream);
			else fs.mWriter->PopStyle();
		}
		return *this;
	}

	FormattedWriter& HorLine()
	{
		for(auto& fs: mFormattedStreams)
		{
			if(fs.IsOwner()) fs.mFormatter->HorLine(*fs.mStream);
			else fs.mWriter->HorLine();
		}
		return *this;
	}

	FormattedWriter& LineBreak(index_t count = 1)
	{
		while(count --> 0)
		{
			for(auto& fs: mFormattedStreams)
			{
				if(fs.IsOwner()) fs.mFormatter->LineBreak(*fs.mStream);
				else fs.mWriter->LineBreak();
			}
		}
		return *this;
	}

	template<typename Arg0, typename... Args>
	FormattedWriter& Print(Arg0&& arg0, Args&&... args)
	{
		for(auto& fs: mFormattedStreams)
		{
			if(fs.IsOwner()) fs.mFormatter->PrintPreformatted(*fs.mStream, String::Concat(Forward<Arg0>(arg0), Forward<Args>(args)...));
			else fs.mWriter->PrintPreformatted(String::Concat(Forward<Arg0>(arg0), Forward<Args>(args)...));
		}
		return *this;
	}

	template<typename T> FormattedWriter& operator<<(T&& x) {return Print(x);}

	template<typename Arg0, typename... Args>
	FormattedWriter& PrintLine(Arg0&& arg0, Args&&... args)
	{
		Print(Forward<Arg0>(arg0), Forward<Args>(args)...);
		LineBreak();
		return *this;
	}

	FormattedWriter& PrintCode(StringView code)
	{
		BeginCode();
		PrintPreformatted(code);
		EndCode();
		return *this;
	}

	FormattedWriter& PrintPreformatted(StringView str)
	{
		for(auto& fs: mFormattedStreams)
		{
			if(fs.IsOwner()) fs.mFormatter->PrintPreformatted(*fs.mStream, str);
			else fs.mWriter->PrintPreformatted(str);
		}
		return *this;
	}

private:
	FormattedWriter(const FormattedWriter&) = delete;
	FormattedWriter& operator=(const FormattedWriter&) = delete;
};
} INTRA_END
