#pragma once

#include "IO/FormattedWriter.h"
#include "Range/Generators/ArrayRange.h"
#include "Container/Sequential/Array.h"

namespace Intra { namespace IO {

class CompositeFormattedWriter: public IFormattedWriter
{
public:
	CompositeFormattedWriter(): attached(null) {}
	CompositeFormattedWriter(ArrayRange<IFormattedWriter* const> streams): attached(streams) {}

	void Attach(IFormattedWriter* stream) {attached.AddLast(stream);}
	void Detach(IFormattedWriter* stream) {attached.FindAndRemoveUnordered(stream);}

	bool operator==(null_t) const {return attached==null;}
	bool operator!=(null_t) const {return attached!=null;}
	operator bool() const {return *this!=null;}

	void PushFont(Math::Vec3 color={0,0,0}, float size=3, bool bold=false, bool italic=false, bool underline=false) override
	{for(auto stream: attached) stream->PushFont(color, size, bold, italic, underline);}

	void PopFont() override {for(auto stream: attached) stream->PopFont();}

	virtual FontDesc GetCurrentFont() const override
	{
		if(attached.Empty()) return {{1,1,1}, 3, false, false, false, false};
		return attached.First()->GetCurrentFont();
	}

	void PushStyle(StringView style) override {for(auto stream: attached) stream->PushStyle(style);}
	void PopStyle() override {for(auto stream: attached) stream->PopStyle();}

	void BeginSpoiler(StringView show) override {for(auto stream: attached) stream->BeginSpoiler(show);}
	void EndSpoiler() override {for(auto stream: attached) stream->EndSpoiler();}
	void EndAllSpoilers() override {for(auto stream: attached) stream->EndAllSpoilers();}
	void BeginCode() override {for(auto stream: attached) stream->BeginCode();}
	void EndCode() override {for(auto stream: attached) stream->EndCode();}

	CompositeFormattedWriter& operator<<(endl_t) override {for(auto stream: attached) *stream << endl; return *this;}
	void Print(StringView s) override {for(auto stream: attached) stream->Print(s);}

	void HorLine() override {for(auto stream: attached) stream->HorLine();}

private:
	virtual void WriteData(const void* data, size_t bytes) override
	{
		for(auto stream: attached)
			stream->WriteData(data, bytes);
	}
	Array<IFormattedWriter*> attached;
};

}}
