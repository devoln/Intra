#pragma once

#include "Cpp/Warnings.h"
#include "Container/Sequential/String.h"
#include "Utils/StringView.h"
#include "Utils/Unique.h"
#include "Concurrency/Atomic.h"
#include "Formatter.h"
#include "Container/Utility/SparseArray.h"
#include "Range/Search/Single.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace IO {

class FormattedWriter
{
public:
	struct FormattedStream
	{
		bool IsOwner() const {return mStream != null;}

		~FormattedStream() {clear();}

		forceinline FormattedStream(Unique<IOutputStream<char>> stream, Unique<IFormatter> formatter):
			mStream(stream.Release()), mFormatter(formatter.Release())
		{
			INTRA_ASSERT(stream != null || formatter == null);
		}

		forceinline FormattedStream(OutputStream stream, Unique<IFormatter> formatter):
			FormattedStream(Cpp::Move(stream.Stream), Cpp::Move(formatter)) {}

		forceinline FormattedStream(FormattedWriter* writerRef):
			mStream(null), mWriter(writerRef) {mWriter->mRefCount.IncrementRelaxed();}

		forceinline bool operator==(null_t) const {return mStream == null && mFormatter == null;}
		forceinline bool operator!=(null_t) const {return !operator==(null);}

		forceinline FormattedStream(null_t=null):
			mStream(null), mFormatter(null) {}

		forceinline FormattedStream(FormattedStream&& rhs):
			mStream(rhs.mStream), mFormatter(rhs.mFormatter)
		{
			rhs.mStream = null;
			rhs.mFormatter = null;
		}

		forceinline FormattedStream(const FormattedStream& rhs) = delete;

	private:
		void clear()
		{
			if(!IsOwner())
			{
				if(mWriter) mWriter->mRefCount.DecrementRelaxed();
				return;
			}
			if(mFormatter)
			{
				mFormatter->CleanUp(*mStream);
				delete mFormatter;
			}
			delete mStream;
		}

		FormattedStream& operator=(FormattedStream&& fs)
		{
			clear();
			mStream = fs.mStream;
			mFormatter = fs.mFormatter;
			fs.mStream = null;
			fs.mFormatter = null;
			return *this;
		}

		FormattedStream& operator=(const FormattedStream&) = delete;

		void reset()
		{
			clear();
			mStream = null;
			mFormatter = null;
		}

		friend class FormattedWriter;

		IOutputStream<char>* mStream;
		union
		{
			IFormatter* mFormatter;
			FormattedWriter* mWriter;
		};
	};

private:
	Array<FormattedStream> mFormattedStreams;
	AtomicInt mRefCount;

public:
	FormattedWriter(null_t=null) {}

	explicit FormattedWriter(OutputStream stream)
	{
		mFormattedStreams.EmplaceLast(Cpp::Move(stream.Stream), new BasicFormatter);
	}

	explicit FormattedWriter(FormattedWriter* writerRef)
	{
		mFormattedStreams.EmplaceLast(writerRef);
	}

	FormattedWriter(OutputStream stream, IFormatter* formatter)
	{
		mFormattedStreams.EmplaceLast(Cpp::Move(stream.Stream), Cpp::Move(formatter));
	}

	FormattedWriter(FormattedWriter&& rhs):
		mFormattedStreams(Cpp::Move(rhs.mFormattedStreams))
	{
		INTRA_ASSERT(rhs.mRefCount.Get() == 0);
	}

	~FormattedWriter()
	{
		INTRA_ASSERT(mRefCount.Get() == 0);
		operator=(null);
	}

	FormattedWriter& operator=(FormattedWriter&& rhs)
	{
		if(this == &rhs) return *this;
		INTRA_ASSERT(mRefCount.Get() == 0);
		INTRA_ASSERT(rhs.mRefCount.Get() == 0);
		mFormattedStreams = Cpp::Move(rhs.mFormattedStreams);
		return *this;
	}

	FormattedWriter& operator=(null_t)
	{
		mFormattedStreams = null;
		return *this;
	}

	size_t Attach(OutputStream stream)
	{
		const size_t index = Range::CountUntil(mFormattedStreams, null);
		if(index == mFormattedStreams.Length())
			mFormattedStreams.EmplaceLast(Cpp::Move(stream), new BasicFormatter);
		else mFormattedStreams[index] = {Cpp::Move(stream.Stream), new BasicFormatter};
		return index;
	}

	size_t Attach(FormattedWriter* writerRef)
	{
		const size_t index = Range::CountUntil(mFormattedStreams, null);
		if(index == mFormattedStreams.Length())
			mFormattedStreams.EmplaceLast(writerRef);
		else mFormattedStreams[index] = {writerRef};
		return index;
	}

	size_t Attach(FormattedWriter&& writer)
	{
		const size_t index = mFormattedStreams.Length();
		for(auto& fs: writer.mFormattedStreams)
		{
			if(fs == null) continue;
			mFormattedStreams.AddLast(Cpp::Move(fs));
		}
		return index;
	}

	size_t Attach(OutputStream stream, IFormatter* formatter)
	{
		const size_t index = Range::CountUntil(mFormattedStreams, null);
		if(index == mFormattedStreams.Length())
			mFormattedStreams.EmplaceLast(Cpp::Move(stream.Stream), Cpp::Move(formatter));
		else mFormattedStreams[index] = {Cpp::Move(stream.Stream), Cpp::Move(formatter)};
		return index;
	}


	bool operator==(null_t) const
	{
		for(auto& fs: mFormattedStreams)
			if(fs != null) return false;
		return true;
	}

	forceinline bool operator!=(null_t) const
	{return !operator==(null);}

	FormattedWriter& PushFont(Math::Vec3 color, float size=3,
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

	FormattedWriter& LineBreak(size_t count=1)
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
			if(fs.IsOwner()) fs.mFormatter->PrintPreformatted(*fs.mStream, String::Concat(Cpp::Forward<Arg0>(arg0), Cpp::Forward<Args>(args)...));
			else fs.mWriter->PrintPreformatted(String::Concat(Cpp::Forward<Arg0>(arg0), Cpp::Forward<Args>(args)...));
		}
		return *this;
	}

	template<typename T> FormattedWriter& operator<<(T&& x) {return Print(x);}

	template<typename Arg0, typename... Args>
	FormattedWriter& PrintLine(Arg0&& arg0, Args&&... args)
	{
		Print(Cpp::Forward<Arg0>(arg0), Cpp::Forward<Args>(args)...);
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

}
using IO::FormattedWriter;

}

INTRA_WARNING_POP
