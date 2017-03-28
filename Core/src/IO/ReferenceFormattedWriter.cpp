#include "IO/ReferenceFormattedWriter.h"

namespace Intra { namespace IO {

class ReferenceFormattedWriterImpl final: public FormattedWriter::BasicImpl
{
public:
	using FormattedWriter::BasicImpl::PushFont;
	using FormattedWriter::BasicImpl::PopFont;

	void PushStyle(OutputStream&, StringView style) override
	{Writer->PushStyle(style);}
	
	void PopStyle(OutputStream&) override
	{Writer->PopStyle();}

	void BeginCode(OutputStream&) override
	{Writer->BeginCode();}

	void EndCode(OutputStream&) override
	{Writer->EndCode();}

	void HorLine(OutputStream&) override
	{Writer->HorLine();}

	void LineBreak(OutputStream&) override
	{Writer->LineBreak();}

	void PrintPreformatted(OutputStream&, StringView s) override
	{Writer->PrintPreformatted(s);}

	void PushFont(OutputStream&, const FontDesc& newFont, const FontDesc&) override
	{Writer->PushFont(newFont);}

	void PopFont(OutputStream&, const FontDesc&, const FontDesc&) override
	{Writer->PopFont();}
	
	void beginSpoiler(OutputStream&, StringView show) override
	{Writer->BeginSpoiler(show);}
	
	void endSpoiler(OutputStream&) override
	{Writer->EndSpoiler();}

	ReferenceFormattedWriterImpl(FormattedWriter& writer): Writer(&writer) {Writer->mRefCount++;}
	~ReferenceFormattedWriterImpl() {Writer->mRefCount--;}
	FormattedWriter* Writer;
};

struct ReferenceFormattedWriterOut
{
	OutputStream* Stream;

	forceinline void Put(char c) {Stream->Put(c);}
	forceinline size_t CopyAdvanceFromAdvance(ArrayRange<const char>& src) {return Stream->CopyAdvanceFromAdvance(src);}
};

FormattedWriter ReferenceFormattedWriter(FormattedWriter& writer)
{
	return FormattedWriter(ReferenceFormattedWriterOut{&writer}, new ReferenceFormattedWriterImpl(writer));
}

}}

