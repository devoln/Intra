#pragma once

#include "Extra/IO/FormattedWriter.h"

void TestFileSyncIO(Intra::FormattedWriter& output);
void TestFileAsyncIO(Intra::FormattedWriter& output);
void TestFileSearching(Intra::FormattedWriter& output);

void TestSocketIO(Intra::FormattedWriter& output);
void TestHttpServer(Intra::FormattedWriter& output);
