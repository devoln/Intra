#include "RawEvent.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Midi {

RawEvent::RawEvent(uint delay, byte status, byte data0, byte data1, size_t metadataLength)
{
	mData.SetLengthUninitialized(7 + metadataLength);
	byte* data = reinterpret_cast<byte*>(mData.Data());
	*data++ = byte(delay & 255);
	*data++ = byte((delay >> 8) & 255);
	*data++ = byte((delay >> 16) & 255);
	*data++ = byte(delay >> 24);
	*data++ = status;
	*data++ = data0;
	*data++ = data1;
}

void RawEventStream::PopFirst()
{
	if(mStream.Empty())
	{
		mCurrentEvent = null;
		return;
	}

	uint delay = uint(Range::ParseVarUInt(mStream));
	byte status = 0;
	byte firstByte = Range::RawReadByte(mStream);
	byte data[2] = {0, 0};
	byte dataIndex = 0;
	if(firstByte & 0x80)
	{
		status = firstByte;
		if(firstByte < 0xF0) mStatus = firstByte;
	}
	else
	{
		status = mStatus;
		data[dataIndex++] = firstByte;
	}

	if(dataIndex == 0 && (status < 0xF0 || status == 0xF2 || status == 0xF3 || status == 0xFF))
		data[dataIndex++] = Range::RawReadByte(mStream);

	if(status == 0xFF && data[0] == 0x2F)
	{
		// end of track event
		mStream = null;
		mCurrentEvent = null;
		return;
	}

	if(status < 0xC0 || (status >= 0xE0 && status < 0xF0) || status == 0xF2)
		data[dataIndex++] = Range::RawReadByte(mStream);

	size_t metadataLength = 0;
	if(status == 0xF0 || status == 0xF7 || status == 0xFF)
		metadataLength = size_t(Range::ParseVarUInt(mStream));

	mCurrentEvent = RawEvent(delay, status, data[0], data[1], metadataLength);
	RawReadTo(mStream, mCurrentEvent.MetaData());
}

}}}

INTRA_WARNING_POP
