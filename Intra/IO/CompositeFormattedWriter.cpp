#include "IO/CompositeFormattedWriter.h"
#include "Container/Sequential/Array.h"
#include "IO/FormattedWriter.h"
#include "IO/ReferenceFormattedWriter.h"

namespace Intra { namespace IO {

struct CompositeFormattedWriterImpl final: FormattedWriter::BasicImpl
{
	using FormattedWriter::BasicImpl::PushFont;
	using FormattedWriter::BasicImpl::PopFont;

	void PushStyle(OutputStream&, StringView style) override
	{for(auto& writer: Attached) writer.PushStyle(style);}
	
	void PopStyle(OutputStream&) override
	{for(auto& writer: Attached) writer.PopStyle();}

	void BeginCode(OutputStream&) override
	{for(auto& writer: Attached) writer.BeginCode();}

	void EndCode(OutputStream&) override
	{for(auto& writer: Attached) writer.EndCode();}

	void HorLine(OutputStream&) override
	{for(auto& writer: Attached) writer.HorLine();}

	void LineBreak(OutputStream&) override
	{for(auto& writer: Attached) writer.LineBreak();}

	void PrintPreformatted(OutputStream&, StringView s) override
	{for(auto& writer: Attached) writer.PrintPreformatted(s);}

	void PushFont(OutputStream&, const FontDesc& newFont, const FontDesc&) override
	{for(auto& writer: Attached) writer.PushFont(newFont);}

	void PopFont(OutputStream&, const FontDesc&, const FontDesc&) override
	{for(auto& writer: Attached) writer.PopFont();}
	
	void beginSpoiler(OutputStream&, StringView show) override
	{for(auto& writer: Attached) writer.BeginSpoiler(show);}
	
	void endSpoiler(OutputStream&) override
	{for(auto& writer: Attached) writer.EndSpoiler();}

	Array<FormattedWriter> Attached;
};

struct CompositeFormattedWriterOut
{
	Array<FormattedWriter>& Attached;

	CompositeFormattedWriterOut(const CompositeFormattedWriterOut& rhs): Attached(rhs.Attached) {}
	CompositeFormattedWriterOut(Array<FormattedWriter>& attached): Attached(attached) {}
	CompositeFormattedWriterOut() = delete;
	CompositeFormattedWriterOut& operator=(const CompositeFormattedWriterOut&) = delete;

	void Put(char c) {for(auto& stream: Attached) stream.Put(c);}

	size_t PutAllAdvance(CSpan<char>& src)
	{
		size_t maxElementsCopied = 0;
		for(auto& stream: Attached)
		{
			auto srcCopy = src;
			size_t elementsCopied = stream.PutAllAdvance(srcCopy);
			if(maxElementsCopied < elementsCopied) maxElementsCopied = elementsCopied;
		}
		src.PopFirstExactly(maxElementsCopied);
		return maxElementsCopied;
	}
};

CompositeFormattedWriter::CompositeFormattedWriter(FormattedWriter::Interface* impl):
	FormattedWriter(CompositeFormattedWriterOut{static_cast<CompositeFormattedWriterImpl*>(impl)->Attached}, impl) {}

CompositeFormattedWriter::CompositeFormattedWriter():
	CompositeFormattedWriter(new CompositeFormattedWriterImpl) {}

size_t CompositeFormattedWriter::Attach(FormattedWriter&& stream)
{
	auto& attached = static_cast<CompositeFormattedWriterImpl*>(mInterface.Ptr())->Attached;
	size_t index = attached.Count();
	attached.AddLast(Cpp::Move(stream));
	return index;
}

size_t CompositeFormattedWriter::Attach(FormattedWriter* stream)
{
	if(stream==null) return ~size_t(0);
	return Attach(ReferenceFormattedWriter(*stream));
}

void CompositeFormattedWriter::Detach(size_t index)
{
	auto& attached = static_cast<CompositeFormattedWriterImpl*>(mInterface.Ptr())->Attached;
	INTRA_DEBUG_ASSERT(index < attached.Count());
	INTRA_DEBUG_ASSERT(attached[index] != null);
	attached[index] = null;
	while(!attached.Empty() && attached.Last()==null)
		attached.RemoveLast();
}

}}
