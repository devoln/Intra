#pragma once

#include "IO/FormattedWriter.h"
#include "Range/Generators/ArrayRange.h"
#include "Container/Sequential/Array.h"

namespace Intra { namespace IO {

class CompositeFormattedWriter final: public AFormattedWriter
{
public:
	CompositeFormattedWriter(): mAttached(null) {}
	CompositeFormattedWriter(ArrayRange<AFormattedWriter* const> streams): mAttached(streams) {}

	void Attach(AFormattedWriter* stream) {mAttached.AddLast(stream);}
	void Detach(AFormattedWriter* stream) {mAttached.FindAndRemoveUnordered(stream);}

	bool operator==(null_t) const {return mAttached==null;}
	bool operator!=(null_t) const {return mAttached!=null;}


	void PushStyle(StringView style) override {for(auto stream: mAttached) stream->PushStyle(style);}
	void PopStyle() override {for(auto stream: mAttached) stream->PopStyle();}

	void BeginCode() override {for(auto stream: mAttached) stream->BeginCode();}
	void EndCode() override {for(auto stream: mAttached) stream->EndCode();}
	void HorLine() override {for(auto stream: mAttached) stream->HorLine();}
	void PrintRaw(StringView s) override {for(auto stream: mAttached) stream->PrintRaw(s);}
	void PrintPreformatted(StringView s) override {for(auto stream: mAttached) stream->PrintPreformatted(s);}

protected:
	void pushFont(const FontDesc& fontDesc) override final
	{for(auto stream: mAttached) stream->PushFont(fontDesc);}

	void popFont() override {for(auto stream: mAttached) stream->PopFont();}
	void beginSpoiler(StringView show) override {for(auto stream: mAttached) stream->BeginSpoiler(show);}
	void endSpoiler() override {for(auto stream: mAttached) stream->EndSpoiler();}

private:
	Array<AFormattedWriter*> mAttached;
};

}}
