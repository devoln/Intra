#pragma once

#include "IO/FormattedWriter.h"

void TestComposedRange(Intra::IO::IFormattedWriter& output);
void TestPolymorphicRange(Intra::IO::IFormattedWriter& output);
void TestStreamRange(Intra::IO::IFormattedWriter& output);
void TestRangeStlInterop(Intra::IO::IFormattedWriter& output);
void TestUnicodeConversion(Intra::IO::IFormattedWriter& output);
