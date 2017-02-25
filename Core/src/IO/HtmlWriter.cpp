#include "IO/HtmlWriter.h"

namespace Intra { namespace IO {

//Это нужно вставить в начало html файла, если используются спойлеры
const StringView HtmlWriter::CssSpoilerCode =
"<style type=\"text/css\">"
	".spoiler > input + label:after{content: \"+\";float: right; font-family: monospace; font-weight: bold; color: FFF;}"
	".spoiler > input {display:none;}"
	".spoiler > input + label, .spoiler > .spoiler_body{background:#080; color: FFF; padding:5px 15px; overflow:hidden; width:100%; box-sizing: border-box; display: block;}"
	".spoiler > input + label +.spoiler_body {display:none;}"
	".spoiler > input:checked + label +.spoiler_body{display: block; }"
	".spoiler >.spoiler_body{background: #FFF; color: 000; border: 3px solid #CCC; border-top: none; }"
"</style>";

const StringView HtmlWriter::SpoilerBeginCode =
"<div class='spoiler'>"
	"<input type='checkbox' id='spoilerid_<^>'>"
	"<label for='spoilerid_<^>'>\r\n<^>\r\n</label>"
	"<div class = 'spoiler_body'>";

const StringView HtmlWriter::SpoilerEndCode = "</div></div>";

const StringView HtmlWriter::CssLoggerCode = "<style type=\"text/css\">"
	".error {color: #ff0000; font-size: 125%;}\r\n"
	".warn {color: #ffaa00; font-size: 110%;}\r\n"
	".info {color: #707070; font-size: 100%;}\r\n"
	".normal {color: #000000; font-size: 100%;}\r\n"
	".perf {color: #aaaa00; font-size: 75%;}\r\n"
	"</style>";


void HtmlWriter::PushFont(Math::Vec3 color, float size, bool bold, bool italic, bool underline)
{
	auto curFont = GetCurrentFont();
	if(color==Math::Vec3(-1)) color = {0,0,0};
	if(size==-1) size = 3;
	mFontStack.AddLast({color, size, bold, italic, underline, false});
	if(curFont.Underline && !underline) RawPrint("</ins>");
	if(curFont.Italic && !italic) RawPrint("</i>");
	if(curFont.Bold && !bold) RawPrint("</b>");
	if(curFont.Color!=color || curFont.Size!=size)
	{
		const auto c = Math::USVec3(Math::Min(color*255.0f, 255.0f));
		String colstr = String::Format()((c.x<<16)|(c.y<<8)|c.z, 6, '0', 16);
		RawPrint("<font color="+colstr+" size="+ToString(size)+">");
	}
	if(!curFont.Bold && bold) RawPrint("<b>");
	if(!curFont.Italic && italic) RawPrint("<i>");
	if(!curFont.Underline && underline) RawPrint("<ins>");
}

void HtmlWriter::PopFont()
{
	auto oldFont = GetCurrentFont();
	mFontStack.RemoveLast();
	auto newFont = GetCurrentFont();
	if(oldFont.Bold && !newFont.Bold) RawPrint("</b>");
	if(oldFont.Italic && !newFont.Italic) RawPrint("</i>");
	if(oldFont.Underline && !newFont.Underline) RawPrint("</ins>");
	RawPrint("</font>");
	if(!oldFont.Bold && newFont.Bold) RawPrint("<b>");
	if(!oldFont.Italic && newFont.Italic) RawPrint("<i>");
	if(!oldFont.Underline && newFont.Underline) RawPrint("<ins>");
}

}}
