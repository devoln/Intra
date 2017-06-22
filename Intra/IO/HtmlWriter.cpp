#include "IO/HtmlWriter.h"
#include "Range/Stream/ToString.h"

namespace Intra { namespace IO {

//Это нужно вставить в начало html файла, если используются спойлеры
static const StringView HtmlWriter_CssSpoilerCode =
"<style type=\"text/css\">"
	".spoiler > input + label:after{content: \"+\";float: right; font-family: monospace; font-weight: bold; color: FFF;}"
	".spoiler > input {display:none;}"
	".spoiler > input + label, .spoiler > .spoiler_body{background:#080; color: FFF; padding:5px 15px; overflow:hidden; width:100%; box-sizing: border-box; display: block;}"
	".spoiler > input + label +.spoiler_body {display:none;}"
	".spoiler > input:checked + label +.spoiler_body{display: block; }"
	".spoiler >.spoiler_body{background: #FFF; color: 000; border: 3px solid #CCC; border-top: none; }"
"</style>";

static const StringView HtmlWriter_SpoilerBeginCode =
"<div class='spoiler'>"
	"<input type='checkbox' id='spoilerid_<^>'>"
	"<label for='spoilerid_<^>'>\r\n<^>\r\n</label>"
	"<div class = 'spoiler_body'>";

static const StringView HtmlWriter_SpoilerEndCode = "</div></div>";

static const StringView HtmlWriter_CssLoggerCode = "<style type=\"text/css\">"
	".error {color: #ff0000; font-size: 125%;}\r\n"
	".warn {color: #ffaa00; font-size: 110%;}\r\n"
	".info {color: #707070; font-size: 100%;}\r\n"
	".normal {color: #000000; font-size: 100%;}\r\n"
	".perf {color: #aaaa00; font-size: 75%;}\r\n"
	"</style>";

class HtmlFormatter final: public BasicFormatter
{
	uint rndCounter = 1;
public:
	using BasicFormatter::PushFont;
	using BasicFormatter::PopFont;

	void BeginCode(IOutputStream<char>& s) override {s << "\n<pre>\n";}
	void EndCode(IOutputStream<char>& s) override {s << "\n</pre>\n";}

	void PushStyle(IOutputStream<char>& s, StringView style) override {s << "<span class='" << style << "'>";}
	void PopStyle(IOutputStream<char>& s) override {s << "</span>";}

	void HorLine(IOutputStream<char>& s) override {s << "<hr>";}

	void LineBreak(IOutputStream<char>& s) override {s << "<br />\n";}

	void PrintPreformatted(IOutputStream<char>& s, StringView text) override
	{
		s << String::MultiReplace(text,
			{"&",      "<",     ">",   "\r\n",   "\n",     "\r"},
			{"&amp;", "&lt;", "&gt;", "<br />", "<br />", "<br />"}
		);
	}

	void PushFont(IOutputStream<char>& s, const FontDesc& newFont, const FontDesc& curFont) override
	{
		if(curFont.Underline && !newFont.Underline) s << "</ins>";
		if(curFont.Italic && !newFont.Italic) s << "</i>";
		if(curFont.Bold && !newFont.Bold) s << "</b>";
		if(curFont.Color != newFont.Color || curFont.Size != newFont.Size)
		{
			const auto c = Math::USVec3(Math::Min(newFont.Color*255.0f, 255.0f));
			const String colstr = String::Format()((c.x<<16)|(c.y<<8)|c.z, 6, '0', 16);
			s << "<font color=" << colstr << " size=" << newFont.Size << ">";
		}
		if(!curFont.Bold && newFont.Bold) s << "<b>";
		if(!curFont.Italic && newFont.Italic) s << "<i>";
		if(!curFont.Underline && newFont.Underline) s << "<ins>";
	}

	void PopFont(IOutputStream<char>& s, const FontDesc& curFont, const FontDesc& prevFont) override
	{
		if(curFont.Bold && !prevFont.Bold) s << "</b>";
		if(curFont.Italic && !prevFont.Italic) s << "</i>";
		if(curFont.Underline && !prevFont.Underline) s << "</ins>";
		s << "</font>";
		if(!curFont.Bold && prevFont.Bold) s << "<b>";
		if(!curFont.Italic && prevFont.Italic) s << "<i>";
		if(!curFont.Underline && prevFont.Underline) s << "<ins>";
	}

	void beginSpoiler(IOutputStream<char>& s, StringView label) override
	{
		const uint rndSeed = (uint(reinterpret_cast<size_t>(label.Data())) ^ ToHash(label)) + rndCounter;
		const ulong64 id = Random::FastUniform<ulong64>(rndSeed)();
		s << *String::Format(HtmlWriter_SpoilerBeginCode)(id)(id)(label);
	}

	void endSpoiler(IOutputStream<char>& s) override {s << HtmlWriter_SpoilerEndCode;}
};

FormattedWriter HtmlWriter(OutputStream stream, bool addDefinitions)
{
	if(addDefinitions) stream.Print(HtmlWriter_CssSpoilerCode);
	return FormattedWriter(Cpp::Move(stream), new HtmlFormatter);
}

}}
