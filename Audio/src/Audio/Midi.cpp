#include "Audio/Midi.h"
#include "Audio/Synth/InstrumentLibrary.h"
#include "IO/FileSystem.h"
#include "IO/FileMapping.h"
#include "Algo/Mutation/Fill.h"
#include "Platform/CppWarnings.h"
#include "Range/Polymorphic/InputRange.h"
#include "Platform/Endianess.h"
#include "Range/Output/OutputArrayRange.h"

namespace Intra { namespace Audio {

using namespace Intra::IO;

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

class MidiReader
{
public:
	InputStream s;

	Synth::IMusicalInstrument* instruments[128];
	Synth::IMusicalInstrument* drumsInstrument = null;

	struct MidiHeader
	{
		MidiHeader(): type(0), tracks(0), timeFormat(0) {}
		shortBE type, tracks, timeFormat;
	};
	MidiHeader header;

	float startTickDuration = 0;
	float globalTickDuration = 0;
	float speed = 1;
	byte status = 0;
	byte track = 0;

#ifdef _MSC_VER
#if(_MSC_VER < 1900)
#pragma warning(disable: 4351)
#endif
#endif
	explicit MidiReader(InputStream stream): s(Meta::Move(stream)),
		instruments{}
	{
		if(!Algo::StartsAdvanceWith(s, "MThd"))
		{
			s = null;
			return;
		}
		const uint chunkSize = s.ReadRaw<uintBE>();
		if(chunkSize!=6)
		{
			s = null;
			return;
		}
		s.ReadRaw<MidiHeader>(header);
		if(header.type>2)
		{
			s = null;
			return;
		}
		const short timeFormat = header.timeFormat;
		if(timeFormat<0)
		{
			const float framesPerSecond = (timeFormat >> 8)==29? 29.97f: float(timeFormat >> 8);
			startTickDuration = globalTickDuration = 1.0f/framesPerSecond/float(timeFormat & 0xFF);
		}
		else startTickDuration = globalTickDuration = 60.0f/120/float(timeFormat);
	}

	MidiReader(const MidiReader&) = delete;
	MidiReader& operator=(const MidiReader&) = delete;

	uint ReadVarInt(uint* oReadBytes=null)
	{
		uint result=0;
		if(oReadBytes!=null) *oReadBytes=0;
		byte l = 0;
		do
		{
			if(s.Empty()) return result;
			l = s.ReadRaw<byte>();
			if(oReadBytes!=null) ++*oReadBytes;
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

	struct TempoChange {uint tick; float tickDuration;};
	Array<TempoChange> tempoChanges;

	static size_t GetEventLength(byte status)
	{
		INTRA_DEBUG_ASSERT(status & 0x80);
		if(status<0xC0) return 2;
		if(status<0xE0) return 1;
		if(status<0xF0) return 2;
		return status==0xF2? 2u: status==0xF3? 1u: 0u;
	}

	MidiEvent ReadEvent(uint* oReadBytes)
	{
		MidiEvent event;
		event.track = track;
		uint delayBytes;
		event.delay = ReadVarInt(&delayBytes);
		byte firstByte = s.ReadRaw<byte>();
		if(firstByte & 0x80) status = firstByte;
		event.status = status;

		OutputArrayRange<byte> eventData(Range::Take(event.data, GetEventLength(status)));
		if((firstByte & 0x80)==0) eventData.Put(firstByte);
		s.ReadRawToAdvance(eventData);
		if(oReadBytes!=null) *oReadBytes = uint(((firstByte & 0x80)!=0)+delayBytes+eventData.ElementsWritten());

		if(status==0xF0 || status==0xF7 || status==0xFF)
		{
			if(status==0xFF) event.data[0] = s.ReadRaw<byte>();
			uint sizeRead;
			uint msgLength = ReadVarInt(&sizeRead);
			event.metadata.SetCountUninitialized(msgLength);
			s.ReadRawTo(event.metadata);
			if(status==0xFF) event.metadata.AddLast(0);
			if(oReadBytes!=null) *oReadBytes += sizeRead+msgLength + (status==0xFF);
		}

		return event;
	}

	MusicTrack ReadTrack()
	{
		if(startTickDuration==0) return null; //Файл имеет неверный формат или не открыт
		char chunkType[4];
		s.ReadRawTo(chunkType);
		uint size = s.ReadRaw<uintBE>();
		if(!Algo::Equals(chunkType, "MTrk"))
		{
			s.PopFirstN(size);
			return null;
		}
		uint bytesRemaining=size;
		uint timeInTicks = 0;
		bool trackIsDrum = false;
		Array<MidiEvent> events;
		while(bytesRemaining>0 && !s.Empty())
		{
			uint eventSize;
			MidiEvent event = ReadEvent(&eventSize);
			timeInTicks += event.delay;
			events.AddLast(event);

			if(event.status==0xFF && event.data[0]==0x51)
				if(header.timeFormat>0)
			{
				const int value = (event.metadata[0] << 16)|(event.metadata[1] << 8)|event.metadata[2];
				tempoChanges.AddLast(TempoChange{timeInTicks, float(value)/1000000.0f/float(header.timeFormat)*speed});
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
			const byte eventType = byte(events[i].status >> 4);
			//auto eventChannel=(events[i].status & 0xF);
			time1InTicks += events[i].delay;
			if(eventType==11 && events[i].data[0]==7)
				currentVolume = events[i].data[1]/127.0f;
			if(eventType==12) //Инструменты могут меняться несколько раз на дорожку, поэтому дорожку придётся разбивать на несколько
			{
				if(result.Instrument==null && events[i].data[0]<128)
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
	MidiReader reader(fileData);
	Music result;

#ifndef INTRA_NO_AUDIO_SYNTH
	static Synth::MusicalInstruments instr;

	ArrayRange<Synth::IMusicalInstrument*> instruments = reader.instruments;

	for(byte i=0; i<128; i++)
		instruments[i] = &instr.Sine2Exp;
	instruments[117] = null; //Melodic Tom не реализован, а стандартное пищание плохо звучит. Поэтому просто отключим этот инструмент

	static const byte pianos[] = {0,1,2,3,6,7, 105, 107};
	for(const byte code: pianos) instruments[code] = &instr.Piano;

	instruments[4] = &instr.ElectricPiano;
	instruments[5] = &instr.ElectricPiano2;

	static const byte organs[] = {16, 18, 19, 20, 21, 22, 23};
	for(const byte code: organs) instruments[code] = &instr.Organ;

	instruments[17] = &instr.PercussiveOrgan;

	static const byte guitars[] = {24, 26, 27, 28, 29, 30, 31, 46};
	for(const byte code: guitars) instruments[code] = &instr.Guitar;

	instruments[25] = &instr.GuitarSteel;
	instruments[32] = &instr.Bass1;
	instruments[33] = &instr.ElectricBassFinger;
	instruments[34] = &instr.ElectricBassPick;

	static const byte basses2[] = {36, 37, 39};
	for(const byte code: basses2) instruments[code] = &instr.Bass2;

	instruments[47] = &instr.GunShot;

	static const byte padSweeps[] = {40, 41, 42, 43, 45, 49, 50, 51,  89, 90, 93, 94, 95, 102};
	for(const byte code: padSweeps) instruments[code] = &instr.Pad8Sweep;

	instruments[48] = &instr.StringEnsemble;

	static const byte panFlutes[] = {71, 75};
	for(const byte code: panFlutes) instruments[code] = &instr.PanFlute;

	static const byte flutes[] = {73, 60};
	for(const byte code: flutes) instruments[code] = &instr.Flute;

	static const byte whistles[] = {74, 76, 77,  78,  79};
	for(const byte code: whistles) instruments[code] = &instr.Whistle;

	instruments[80] = &instr.LeadSquare;
	instruments[81] = &instr.Sawtooth;
	instruments[87] = &instr.BassLead;
	instruments[94] = &instr.Pad7Halo;
	instruments[119] = &instr.ReverseCymbal;
	instruments[99] = &instr.Atmosphere;

	static const byte vibraphones[] = {8, 10,  11, 12, 88, 92, 108};
	for(const byte code: vibraphones) reader.instruments[code] = &instr.Vibraphone;

	instruments[9] = &instr.Glockenspiel;

	static const byte newAges[] = {88, 92};
	for(const byte code: newAges) instruments[code] = &instr.NewAge;

	instruments[98] = &instr.Crystal;

	static const byte kalimbas[] = {13, 15, 108, 112};
	for(const byte code: kalimbas) instruments[code] = &instr.Kalimba;

	static const byte synthVoices[] = {52, 53, 54, 83, 85, 100};
	for(const byte code: synthVoices) instruments[code] = &instr.SynthVoice;

	static const byte soundTrackFx[] = {44, 97};
	for(const byte code: soundTrackFx) instruments[code] = &instr.SoundTrackFX2;

	static const byte trumpets[] = {56, 57, 58, 68, 69, 70, 72};
	for(const byte code: trumpets) instruments[code] = &instr.Trumpet;

	instruments[68] = &instr.Oboe;

	static const byte sax[] = {64, 65, 66, 67};
	for(const byte code: sax) instruments[code] = &instr.Sax;

	static const byte synthBrasses[] = {61, 62, 63};
	for(const byte code: synthBrasses) instruments[code] = &instr.SynthBrass;

	instruments[84] = &instr.Lead5Charang;
	instruments[82] = &instr.Calliope;
	instruments[35] = &instr.FretlessBass;
	instruments[55] = &instr.OrchestraHit;
	instruments[38] = &instr.SynthBass1;
	instruments[91] = &instr.PadChoir;
	instruments[96] = &instr.Rain;

	static const byte drumSounds[] = {115, 118, 120};
	for(const byte code: drumSounds) instruments[code] = &instr.DrumSound2;

	instruments[122] = &instr.Seashore;
	instruments[124] = &instr.PhoneRing;
	instruments[125] = &instr.Helicopter;
	instruments[126] = &instr.Applause;
	instruments[127] = &instr.GunShot;
	reader.drumsInstrument = &instr.Drums;
#endif
	for(int i=0; i<reader.header.tracks; i++)
	{
		auto track = reader.ReadTrack();
		if(track.Notes==null) continue;
		result.Tracks.AddLast(Meta::Move(track));
#ifndef INTRA_NO_AUDIO_SYNTH
		if(result.Tracks.Last().Instrument==null)
			result.Tracks.Last().Instrument = &instr.Sine2Exp;
#endif
	}
	return result;
}


Music ReadMidiFile(StringView path)
{
	auto fileMapping = OS.MapFile(path);
	if(fileMapping==null) return null;
	return ReadMidiFile(fileMapping.AsRange());
}

INTRA_WARNING_POP

}}
