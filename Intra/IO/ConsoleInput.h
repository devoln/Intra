#pragma once

#include "Range/Stream/InputStreamMixin.h"

namespace Intra { namespace IO {

//! Диапазон ввода с консоли.
//! Может не работать, если stdin перенаправлен.
//! Если нужно перенаправление, используйте Std.
class ConsoleInput: public Range::InputStreamMixin<ConsoleInput, char>
{
public:
	enum: bool {RangeIsInfinite = true};

	//! Все экземпляры этого класса указывают на одну и ту же основную консоль.
	ConsoleInput() {}

	char First() const;
	void PopFirst();
	forceinline bool Empty() const {return false;}
	size_t ReadToAdvance(Span<char>& dst);

	dchar GetChar();

	//Несмотря на то, что копировать нечего, это не Forward Range
	ConsoleInput(const ConsoleInput&) = delete;
	ConsoleInput& operator=(const ConsoleInput&) = delete;

	ConsoleInput(ConsoleInput&&) {}
	ConsoleInput& operator=(ConsoleInput&&) {return *this;}
};

extern ConsoleInput ConsoleIn;

}}
