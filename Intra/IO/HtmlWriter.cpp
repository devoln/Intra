#include "IO/HtmlWriter.h"

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

class HtmlWriterImpl final: public FormattedWriter::BasicImpl
{
	uint rndCounter = 1;
public:
	using FormattedWriter::BasicImpl::PushFont;
	using FormattedWriter::BasicImpl::PopFont;

	void BeginCode(OutputStream& s) override {s.Print("\n<pre>\n");}
	void EndCode(OutputStream& s) override {s.Print("\n</pre>\n");}

	void PushStyle(OutputStream& s, StringView style) override {s.Print("<span class='", style, "'>");}
	void PopStyle(OutputStream& s) override {s.Print("</span>");}

	void HorLine(OutputStream& s) override {s.Print("<hr>");}

	void LineBreak(OutputStream& s) override {s.PrintLine("<br />");}

	void PrintPreformatted(OutputStream& s, StringView text) override
	{
		s.Print(String::MultiReplace(text,
			{"&",      "<",     ">",   "\r\n",   "\n",     "\r"},
			{"&amp;", "&lt;", "&gt;", "<br />", "<br />", "<br />"}
		));
	}

	void PushFont(OutputStream& s, const FontDesc& newFont, const FontDesc& curFont) override
	{
		if(curFont.Underline && !newFont.Underline) s.Print("</ins>");
		if(curFont.Italic && !newFont.Italic) s.Print("</i>");
		if(curFont.Bold && !newFont.Bold) s.Print("</b>");
		if(curFont.Color != newFont.Color || curFont.Size != newFont.Size)
		{
			const auto c = Math::USVec3(Math::Min(newFont.Color*255.0f, 255.0f));
			String colstr = String::Format()((c.x<<16)|(c.y<<8)|c.z, 6, '0', 16);
			s.Print("<font color=", colstr, " size=", newFont.Size, ">");
		}
		if(!curFont.Bold && newFont.Bold) s.Print("<b>");
		if(!curFont.Italic && newFont.Italic) s.Print("<i>");
		if(!curFont.Underline && newFont.Underline) s.Print("<ins>");
	}

	void PopFont(OutputStream& s, const FontDesc& curFont, const FontDesc& prevFont) override
	{
		if(curFont.Bold && !prevFont.Bold) s.Print("</b>");
		if(curFont.Italic && !prevFont.Italic) s.Print("</i>");
		if(curFont.Underline && !prevFont.Underline) s.Print("</ins>");
		s.Print("</font>");
		if(!curFont.Bold && prevFont.Bold) s.Print("<b>");
		if(!curFont.Italic && prevFont.Italic) s.Print("<i>");
		if(!curFont.Underline && prevFont.Underline) s.Print("<ins>");
	}

	void beginSpoiler(OutputStream& s, StringView label) override
	{
		const uint rndSeed = uint(reinterpret_cast<size_t>(label.Data()))^ToHash(label)+rndCounter;
		const ulong64 id = Random::FastUniform<ulong64>(rndSeed)();
		s.Print(*String::Format(HtmlWriter_SpoilerBeginCode)(id)(id)(label));
	}

	void endSpoiler(OutputStream& s) override {s.Print(HtmlWriter_SpoilerEndCode);}
};

FormattedWriter HtmlWriter(OutputStream stream, bool addDefinitions)
{
	if(addDefinitions) stream.Print(HtmlWriter_CssSpoilerCode);
	return FormattedWriter(Cpp::Move(stream), new HtmlWriterImpl);
}

}}
