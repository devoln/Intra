#pragma once

#include "Platform/CppWarnings.h"
#include "Core/FundamentalTypes.h"

namespace Intra { namespace Audio {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

struct MusicNote
{
	enum NoteType: byte {C, CSharp, D, DSharp, E, F, FSharp, G, GSharp, A, ASharp, H};
	static const float BasicFrequencies[12]; //Таблица соответствия нот субконтроктавы частотам

	MusicNote(byte octave, NoteType note, ushort duration):
		Octave(octave), Note(note), Duration(duration) {}

	MusicNote(null_t=null):
		Octave(255), Note(NoteType(255)), Duration(0) {}

	static MusicNote Pause(ushort duration)
	{
		MusicNote result;
		result.Duration = duration;
		return result;
	}

	bool IsPause() const {return Octave==255 && Duration!=0;}

	operator NoteType() const {return Note;}
	bool operator==(const MusicNote& rhs) const
	{return Octave==rhs.Octave && Note==rhs.Note && Duration==rhs.Duration;}
	
	bool operator!=(const MusicNote& rhs) const {return !operator==(rhs);}
	
	bool operator==(null_t) const
	{return Duration==0 || (Octave!=255 && (Octave>=8 || Note>=12));}
	
	bool operator!=(null_t) const {return !operator==(null);}

	float Frequency() const {return BasicFrequencies[Note]*float(1 << Octave);}
	float AbsDuration(float tempo) const {return Duration*tempo/2048;}

	byte Octave; //0 - субконтроктава, дальше по порядку. Если Octave==255, то это не нота, а пауза
	NoteType Note;
	ushort Duration; //Относительная длительность ноты в 1/2048 долях
};

INTRA_WARNING_POP

}}
