#include "Sound/Midi.h"
#include "Sound/InstrumentLibrary.h"
#include "IO/Stream.h"
#include "IO/File.h"

namespace Intra {

using namespace IO;

class MidiReader
{
public:
	MemoryInputStream s;

	IMusicalInstrument* instruments[128];
	IMusicalInstrument* drumsInstrument=null;

	struct MidiHeader
	{
		MidiHeader(): type(0), tracks(0), timeFormat(0) {}
		shortBE type, tracks, timeFormat;
	};
	MidiHeader header;

	byte status;
	float startTickDuration, globalTickDuration, speed;
	byte track;

	MidiReader(const void* fileData, size_t size):
		s(fileData, size),
		drumsInstrument(null), header(),
		status(0), startTickDuration(0), globalTickDuration(0), speed(1), track(0)
	{
		if(s.ReadNChars(4)!="MThd") { s=null; return; }
		core::memset(instruments, 0, sizeof(instruments));
		uint chunkSize = s.Read<uintBE>();
		if(chunkSize!=6) { s=null; return; }
		header = s.Read<MidiHeader>();
		if(header.type>2) { s=null; return; }
		short timeFormat = header.timeFormat;
		if(timeFormat<0)
		{
			const float framesPerSecond = (timeFormat >> 8)==29? 29.97f: (timeFormat >> 8);
			startTickDuration = globalTickDuration = 1.0f/framesPerSecond/(timeFormat & 0xFF);
		}
		else startTickDuration = globalTickDuration = 60.0f/120/timeFormat;
	}

	uint ReadVarInt(uint* readBytes=null)
	{
		uint result=0;
		byte l;
		if(readBytes!=null) *readBytes=0;
		do
		{
			if(s.EndOfStream()) return result;
			l = s.Read<byte>();
			if(readBytes!=null) ++*readBytes;
			result = (result << 7) | (l & 0x7F);
		} while(l & 0x80);
		return result;
	}

	typedef Array<byte> MidiExtendedEventData;

	struct MidiEvent
	{
		byte status;
		uint delay;
		byte data[2];
		byte track;
		Array<byte> metadata;
	};

	struct TempoChange { uint tick; float tickDuration; };
	Array<TempoChange> tempoChanges;

	static size_t GetEventLength(byte status)
	{
		INTRA_ASSERT(status & 0x80);
		if(status<0xC0) return 2;
		if(status<0xE0) return 1;
		if(status<0xF0) return 2;
		return status==0xF2? 2u: status==0xF3? 1u: 0u;
	}

	MidiEvent ReadEvent(uint* oReadBytes)
	{
		MidiEvent event;
		event.track = track;
		uint delayBytes; event.delay = ReadVarInt(&delayBytes);
		byte firstByte = s.Read<byte>();
		if(firstByte & 0x80) status = firstByte;
		event.status = status;
		size_t bytesToRead = GetEventLength(status);
		size_t bytesRead = 0;
		if((firstByte & 0x80)==0) event.data[bytesRead++]=firstByte;
		s.ReadData(event.data+bytesRead, bytesToRead-bytesRead);
		if(oReadBytes!=null) *oReadBytes = uint(((firstByte & 0x80)!=0)+delayBytes+bytesToRead);

		if(status==0xF0 || status==0xF7 || status==0xFF)
		{
			if(status==0xFF) event.data[0] = s.Read<byte>();
			uint sizeRead;
			uint msgLength = ReadVarInt(&sizeRead);
			event.metadata = s.ReadArray<byte>(msgLength);
			if(status==0xFF) event.metadata.AddLast(0);
			if(oReadBytes!=null) *oReadBytes += sizeRead+msgLength + (status==0xFF);
		}

		return event;
	}

	MusicTrack ReadTrack()
	{
		if(startTickDuration==0) return null; //Файл имеет неверный формат или не открыт
		String chunkType = s.ReadNChars(4);
		uint size = s.Read<uintBE>();
		if(chunkType!="MTrk") {s.Skip(size); return null;}
		uint bytesRemaining=size;
		uint timeInTicks = 0;
		bool trackIsDrum = false;
		Array<MidiEvent> events;
		while(bytesRemaining>0 && !s.EndOfStream())
		{
			uint eventSize;
			MidiEvent event = ReadEvent(&eventSize);
			timeInTicks += event.delay;
			events.AddLast(event);

			if(event.status==0xFF && event.data[0]==0x51)
				if(header.timeFormat>0)
			{
				auto value = (event.metadata.Data()[0] << 16)|(event.metadata.Data()[1] << 8)|(event.metadata.Data()[2]);
				tempoChanges.AddLast(TempoChange{timeInTicks, value/1000000.0f/header.timeFormat*speed});
			}
			if(event.status==0x99)
				trackIsDrum = true;
			bytesRemaining -= eventSize;
		}

		track++;

		MusicTrack result;
		uint time1InTicks = 0, lastTime1InTicks = 0;
		uint tempoChangeIndex = 0;
		double currentTickDuration = globalTickDuration;
		float currentVolume = 1;
		for(uint i=0; i<events.Count(); i++)
		{
			auto eventType = (events[i].status >> 4);
			//auto eventChannel=(events[i].status & 0xF);
			time1InTicks += events[i].delay;
			if(eventType==11 && events[i].data[0]==7)
				currentVolume = events[i].data[1]/127.0f;
			if(eventType==12) //Инструменты могут меняться несколько раз на дорожку, поэтому дорожку придётся разбивать на несколько
			{
				if(result.Instrument==null)
					result.Instrument = instruments[events[i].data[0]];
				if(result.Instrument==null)
					result.Instrument = instruments[0];
			}
			if(eventType!=9) continue;

			uint time2InTicks=0;
			for(uint j=i+1; j<events.Count(); j++)
			{
				time2InTicks+=events[j].delay;
				if(((events[j].status >> 4)==8 && events[j].data[0]==events[i].data[0]) ||
					((events[j].status >> 4)==9 && events[j].data[1]==0 && events[j].data[0]==events[i].data[0]))
				{
					while(tempoChangeIndex<tempoChanges.Count() &&
						time1InTicks>=tempoChanges[tempoChangeIndex].tick)
					{
						currentTickDuration = tempoChanges[tempoChangeIndex++].tickDuration;
					}
					MusicNote note;
					note.Octave = byte((events[i].data[0])/12);
					note.Note = MusicNote::NoteType((events[i].data[0])%12);
					//else note.Octave=4, note.Note=MusicNote::C;
					note.Duration = ushort(currentTickDuration*time2InTicks*2048);
					auto delayInTicks = time1InTicks-lastTime1InTicks;
					uint delay = uint(currentTickDuration*delayInTicks*2048);
					while(delay>65534)
					{
						result.Notes.EmplaceLast(MusicNote::Pause(65534), ushort(65534), 1.0f);
						delay -= 65534;
					}
					result.Notes.EmplaceLast(note, ushort(delay), currentVolume*events[i].data[1]/127.0f);
					lastTime1InTicks = time1InTicks;
					break;
				}
			}
		}
		events.SetCount(0);
		if(result.Notes==null) return null;
		if(trackIsDrum) result.Instrument = drumsInstrument;
		return result;
	}
};



Music ReadMidiFile(ArrayRange<const byte> fileData)
{
	MidiReader reader(fileData.Begin, fileData.Length());
	Music result;

#ifndef INTRA_NO_MIDI_SYNTH
	MusicalInstruments* instr = new MusicalInstruments;
	for(byte i=0; i<128; i++)
		reader.instruments[i] = &instr->Sine2Exp;
	reader.instruments[117] = null; //Melodic Tom не реализован, а пищание плохо звучит

	for(auto code: {0,1,2,3,6,7, 105, 107}) reader.instruments[code] = &instr->Piano;
	for(auto code: {4}) reader.instruments[code] = &instr->ElectricPiano;
	for(auto code: {5}) reader.instruments[code] = &instr->ElectricPiano2;
	for(auto code: {16, 18, 19, 20, 21, 22, 23}) reader.instruments[code] = &instr->Organ;
	for(auto code: {17}) reader.instruments[code] = &instr->PercussiveOrgan;
	for(auto code: {24, 26, 27, 28}) reader.instruments[code] = &instr->Guitar;
	for(auto code: {25}) reader.instruments[code] = &instr->GuitarSteel;
	for(auto code: {29, 30, 31, 46}) reader.instruments[code] = &instr->Guitar;
	for(auto code: {32}) reader.instruments[code] = &instr->Bass1;
	for(auto code: {33}) reader.instruments[code] = &instr->ElectricBassFinger;
	for(auto code: {34}) reader.instruments[code] = &instr->ElectricBassPick;
	for(auto code: {36, 37, 39}) reader.instruments[code] = &instr->Bass2;
	for(auto code: {47}) reader.instruments[code] = &instr->GunShot;
	for(auto code: {40, 41, 42, 43, 45}) reader.instruments[code] = &instr->Pad8Sweep;
	for(auto code: {49, 50, 51}) reader.instruments[code] = &instr->Pad8Sweep;
	for(auto code: {48}) reader.instruments[code] = &instr->StringEnsemble;
	for(auto code: {71,  73, 75}) reader.instruments[code] = &instr->PanFlute;
	for(auto code: {73,   60}) reader.instruments[code] = &instr->Flute;
	for(auto code: {74, 76, 77,  78,  79}) reader.instruments[code] = &instr->Whistle;
	for(auto code: {80}) reader.instruments[code] = &instr->LeadSquare;
	for(auto code: {81}) reader.instruments[code] = &instr->Sawtooth;
	for(auto code: {87}) reader.instruments[code] = &instr->BassLead;
	for(auto code: {94}) reader.instruments[code] = &instr->Pad7Halo;
	for(auto code: {89, 90, 93, 94, 95, 102}) reader.instruments[code] = &instr->Pad8Sweep;
	for(auto code: {119}) reader.instruments[code] = &instr->ReverseCymbal;
	for(auto code: {99}) reader.instruments[code] = &instr->Atmosphere;
	for(auto code: {8, 10,  11, 12, 88, 92, 108}) reader.instruments[code] = &instr->Vibraphone;
	for(auto code: {9}) reader.instruments[code] = &instr->Glockenspiel;
	for(auto code: {88, 92}) reader.instruments[code] = &instr->NewAge;
	for(auto code: {98}) reader.instruments[code] = &instr->Crystal;
	for(auto code: {13, 15, 108, 112}) reader.instruments[code] = &instr->Kalimba;
	for(auto code: {52, 53, 54, 83, 85, 100}) reader.instruments[code] = &instr->SynthVoice;
	for(auto code: {44, 97}) reader.instruments[code] = &instr->SoundTrackFX2;
	for(auto code: {56, 57, 58, 68, 69, 70, 72}) reader.instruments[code] = &instr->Trumpet;
	for(auto code: {68}) reader.instruments[code] = &instr->Oboe;
	for(auto code: {64, 65, 66, 67}) reader.instruments[code] = &instr->Sax;
	for(auto code: {61, 62, 63, 84}) reader.instruments[code] = &instr->SynthBrass;
	for(auto code: {84}) reader.instruments[code] = &instr->Lead5Charang;
	for(auto code: {82}) reader.instruments[code] = &instr->Calliope;
	reader.instruments[35] = &instr->FretlessBass;
	reader.instruments[55] = &instr->OrchestraHit;
	reader.instruments[38] = &instr->SynthBass1;
	reader.instruments[91] = &instr->PadChoir;
	reader.instruments[96] = &instr->Rain;
	for(auto code:{115, 118, 120}) reader.instruments[code] = &instr->DrumSound2;
	reader.instruments[122] = &instr->Seashore;
	reader.instruments[124] = &instr->PhoneRing;
	reader.instruments[125] = &instr->Helicopter;
	reader.instruments[126] = &instr->Applause;
	reader.instruments[127] = &instr->GunShot;
	reader.drumsInstrument = &instr->Drums;
#endif
	for(int i=0; i<reader.header.tracks; i++)
	{
		auto track = reader.ReadTrack();
		if(track.Notes==null) continue;
		result.Tracks.AddLast(core::move(track));
#ifndef INTRA_NO_MIDI_SYNTH
		if(result.Tracks.Last().Instrument==null)
			result.Tracks.Last().Instrument = &instr->Sine2Exp;
#endif
	}
	return result;
}


#include <IO/File.h>

Music ReadMidiFile(StringView path)
{
	bool opened;
	auto buf = DiskFile::ReadAsArray<byte>(path, &opened);
	if(!opened) return null;
	return ReadMidiFile(buf.AsConstRange());
}

}

