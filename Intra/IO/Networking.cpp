

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Emscripten)



INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
INTRA_IGNORE_WARNING(dollar-in-identifier-extension)

#include <emscripten.h>

#include "Core/Core.h"
#include "IO/Networking.h"

INTRA_BEGIN
namespace IO {

Array<byte> DownloadFile(StringView path)
{
	size_t dataSize;
	auto dataInt = EM_ASM_INT({
		function stringToByteBufferData(str)
	{
		bbPtr = Module._malloc(str.length);
		for(var i=0, strLen=str.length; i<strLen; i++)
			HEAPU8[bbPtr+i] = str.charCodeAt(i) & 0xff;
		return bbPtr;
	}

	function getXmlHttp()
	{
		var xmlhttp;
		try { xmlhttp = new ActiveXObject("Msxml2.XMLHTTP"); }
		catch(e)
		{
			try { xmlhttp = new ActiveXObject("Microsoft.XMLHTTP"); }
			catch(E) { xmlhttp = false; }
		}
		if(!xmlhttp && typeof XMLHttpRequest!='undefined') xmlhttp = new XMLHttpRequest();
		return xmlhttp;
	}

		/*var oReq = new XMLHttpRequest();

		oReq.open("GET", Pointer_stringify($0, $1), true);
		oReq.responseType = "arraybuffer";
		var bbPtr;
		oReq.onload = function(e)
		{
		var arraybuffer = new Uint8Array(oReq.response);
		bbPtr = Module._malloc(arraybuffer.length);
		var bbData = new Uint8Array(bbPtr);
		bbData.set(arraybuffer);
		HEAPU32[$2] = arraybuffer.length;
		}
		oReq.send();*/



	var request = new XMLHttpRequest();
	request.open('GET', Pointer_stringify($0, $1), false);
	request.overrideMimeType('text\/plain; charset=x-user-defined');
	request.send();

	var data;
	if(request.status === 200)
	{
		HEAPU32[$2] = request.responseText.length;
		return stringToByteBufferData(request.response);
	}

	alert('Something bad happen!\n(' + request.status + ') ' + request.statusText);
	return 0;
	}, path.Data(), path.Length(), reinterpret_cast<size_t>(&dataSize)/sizeof(&dataSize)
	);

	return Array<byte>::CreateAsOwnerOf({reinterpret_cast<byte*>(dataInt), dataSize});
}

}}


#endif
