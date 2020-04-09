#pragma once

#include "Intra/Range/Stream/InputStreamMixin.h"

INTRA_BEGIN
//! Диапазон ввода с консоли.
//! Может не работать, если stdin перенаправлен.
//! Если нужно перенаправление, используйте Std.
class ConsoleInput: public InputStreamMixin<ConsoleInput, char>
{
public:
	static constexpr bool IsAnyInstanceInfinite = true;

	//! Все экземпляры этого класса указывают на одну и ту же основную консоль.
	ConsoleInput() {}

	char First() const;
	void PopFirst();
	INTRA_FORCEINLINE bool Empty() const { return false; }
	index_t ReadWrite(Span<char>& dst);

	char32_t GetChar();

	ConsoleInput(const ConsoleInput&) = delete;
	ConsoleInput& operator=(const ConsoleInput&) = delete;

	ConsoleInput(ConsoleInput&&) = default;
	ConsoleInput& operator=(ConsoleInput&&) = default;
};

extern ConsoleInput ConsoleIn;
INTRA_END
