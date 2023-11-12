#pragma once

#include "Intra/Range/Stream/InputStreamMixin.h"

namespace Intra { INTRA_BEGIN
/// �������� ����� � �������.
/// ����� �� ��������, ���� stdin �������������.
/// ���� ����� ���������������, ����������� Std.
class ConsoleInput: public InputStreamMixin<ConsoleInput, char>
{
public:
	using TagAnyInstanceInfinite = TTag<>;

	/// ��� ���������� ����� ������ ��������� �� ���� � �� �� �������� �������.
	ConsoleInput() {}

	char First() const;
	void PopFirst();
	INTRA_FORCEINLINE bool Empty() const {return false;}
	index_t StreamTo(Span<char>& dst);

	char32_t GetChar();

	ConsoleInput(const ConsoleInput&) = delete;
	ConsoleInput& operator=(const ConsoleInput&) = delete;

	ConsoleInput(ConsoleInput&&) = default;
	ConsoleInput& operator=(ConsoleInput&&) = default;
};

extern ConsoleInput ConsoleIn;
} INTRA_END
