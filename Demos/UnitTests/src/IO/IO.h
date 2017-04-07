#pragma once

#include "IO/FormattedWriter.h"

void TestFileSyncIO(Intra::IO::FormattedWriter& output);
void TestFileAsyncIO(Intra::IO::FormattedWriter& output);
void TestFileSearching(Intra::IO::FormattedWriter& output);

void TestSocketIO(Intra::IO::FormattedWriter& output);
void TestHttpServer(Intra::IO::FormattedWriter& output);
