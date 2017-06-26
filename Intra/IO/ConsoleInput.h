#pragma once

#include "Range/Stream/InputStreamMixin.h"

namespace Intra { namespace IO {

//! �������� ����� � �������.
//! ����� �� ��������, ���� stdin �������������.
//! ���� ����� ���������������, ����������� Std.
class ConsoleInput: public Range::InputStreamMixin<ConsoleInput, char>
{
public:
	enum: bool {RangeIsInfinite = true};

	//! ��� ���������� ����� ������ ��������� �� ���� � �� �� �������� �������.
	ConsoleInput() {}

	char First() const;
	void PopFirst();
	forceinline bool Empty() const {return false;}
	size_t ReadToAdvance(Span<char>& dst);

	dchar GetChar();

	//�������� �� ��, ��� ���������� ������, ��� �� Forward Range
	ConsoleInput(const ConsoleInput&) = delete;
	ConsoleInput& operator=(const ConsoleInput&) = delete;

	ConsoleInput(ConsoleInput&&) {}
	ConsoleInput& operator=(ConsoleInput&&) {return *this;}
};

extern ConsoleInput ConsoleIn;

}}
