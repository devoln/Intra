#pragma once

#include "Intra/Core.h"

INTRA_BEGIN
struct MusicNote
{
	enum Type: byte {C, CSharp, D, DSharp, E, F, FSharp, G, GSharp, A, ASharp, B};

	//Таблица соответствия нот субконтроктавы частотам
	static const float BasicFrequencies[12];

	INTRA_FORCEINLINE MusicNote(byte octave, Type note): NoteOctave(byte((octave << 4) | byte(note))) {}
	INTRA_FORCEINLINE MusicNote(decltype(null)=null): NoteOctave(255) {}

	INTRA_FORCEINLINE Type Note() const {return Type(NoteOctave & 15);}
	INTRA_FORCEINLINE byte Octave() const {return byte(NoteOctave >> 4);}
	
	INTRA_FORCEINLINE bool operator==(const MusicNote& rhs) const {return NoteOctave == rhs.NoteOctave;}
	INTRA_FORCEINLINE bool operator!=(const MusicNote& rhs) const {return !operator==(rhs);}
	
	INTRA_FORCEINLINE bool operator==(decltype(null)) const noexcept {return NoteOctave == 255;}
	INTRA_FORCEINLINE bool operator!=(decltype(null)) const noexcept {return !operator==(null);}

	INTRA_FORCEINLINE float Frequency() const {return BasicFrequencies[byte(Note())]*float(1 << Octave());}

	/// Запакованные в один байт нота и октава.
	/// Младшие 4 бита - нота, старшие биты - октава, начиная с субконтроктавы.
	byte NoteOctave;
};
INTRA_END
