#pragma once

#include "Cpp/Warnings.h"
#include "Cpp/Fundamental.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio {

struct MusicNote
{
	enum Type: byte {C, CSharp, D, DSharp, E, F, FSharp, G, GSharp, A, ASharp, B};

	//Таблица соответствия нот субконтроктавы частотам
	static const float BasicFrequencies[12];

	forceinline MusicNote(byte octave, Type note): NoteOctave(byte((octave << 4) | byte(note))) {}
	forceinline MusicNote(null_t=null): NoteOctave(255) {}

	forceinline Type Note() const {return Type(NoteOctave & 15);}
	forceinline byte Octave() const {return byte(NoteOctave >> 4);}
	
	forceinline bool operator==(const MusicNote& rhs) const {return NoteOctave == rhs.NoteOctave;}
	forceinline bool operator!=(const MusicNote& rhs) const {return !operator==(rhs);}
	
	forceinline bool operator==(null_t) const noexcept {return NoteOctave == 255;}
	forceinline bool operator!=(null_t) const noexcept {return !operator==(null);}

	forceinline float Frequency() const {return BasicFrequencies[byte(Note())]*float(1 << Octave());}

	//! Запакованные в один байт нота и октава.
	//! Младшие 4 бита - нота, старшие биты - октава, начиная с субконтроктавы.
	byte NoteOctave;
};

}}

INTRA_WARNING_POP
