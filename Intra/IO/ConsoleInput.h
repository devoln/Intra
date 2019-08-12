#pragma once

#include "Core/Range/Stream/InputStreamMixin.h"

INTRA_BEGIN
namespace IO {

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
	size_t ReadWrite(Span<char>& dst);

	char32_t GetChar();

	ConsoleInput(const ConsoleInput&) = delete;
	ConsoleInput& operator=(const ConsoleInput&) = delete;

	ConsoleInput(ConsoleInput&&) = default;
	ConsoleInput& operator=(ConsoleInput&&) = default;
};

extern ConsoleInput ConsoleIn;

}}
