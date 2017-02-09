#include "Platform/PlatformInfo.h"

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Emscripten)

#include <emscripten.h>

#include "IO/Networking.h"

namespace Intra { namespace IO {

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




	//Для кроссдоменных запросов
	//https://habrahabr.ru/post/130673/
	//Использовать так: xdr.xget(url, callback);
#if INTRA_DISABLED
	/* Requires _opera-xdr-engine.js to handle script-based requests in Opera*/
	var xdr ={
		/* request ID, JSONP counter*/
	reqId: 0,
		   req : {},
		/* Prevent caching*/
		prepareUrl : function(url)
		{
			url = /\?/.test(url) ? url + "&" + seed : url + "?" + seed;
			return url;
		},
		/*
		*
		*/
		xget : function(url, onDone)
		{
			url = this.prepareUrl(url);
			if(window.opera && window.opera.defineMagicVariable)
			{
				this.scriptTransport(url, onDone);
			}
			else if(chrome && chrome.extension)
			{
				this.xhrTransport(url, onDone);
			}
			else if(GM_xmlhttpRequest)
			{
				this.GMTransport(url,onDone);
			}
			else
			{
				var currentReqId = this.reqId++;
				this.req[currentReqId].handleJSONP = onDone;
				this.JSONPTransport(url, "xdr.req["+currentReqId+"].handleJSONP");
			}
		},

		/**
		* Make GET request via <script> transport.
		*
		*/
		scriptTransport: function(url, onDone)
		{
			var t = document.createElement("script");
			t.src = url;
			t._callback = onDone;
			document.body.appendChild(t);
		},

		// transport should be proxyed via background.html of the chrome
		// extension
		xhrTransport : function(url, onDone)
		{
			chrome.extension.sendRequest({'action' : 'xget', 'url':url}, onDone);
		},
		/**
		* Make GET request via GM_xmlhttpRequest.
		*
		*/
		GMTransport : function(url, onDone)
		{
			setTimeout(function()
			{
				GM_xmlhttpRequest({
				method: "GET",
						url : url,
					onload : function(x)
				{
					var o = x.responseText;
					if(onDone)
					{
						onDone(o);
					}
				}
				});
			},0);

		},
		JSONPTransport : function(url, callbackName)
		{
			if(callbackName && typeof callbackName == "string")
			{
				url += "&callback="+callbackName;
			}
			var t = document.createElement("script");
			t.src = url;
			document.body.appendChild(t);
		}
	}
#endif

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
