#include "TestRun.h"

#include "Intra/Assert.h"
#include "Extra/Utils/Logger.h"
#include "Extra/IO/FormattedWriter.h"

INTRA_BEGIN
void RunUnitTests(FormattedWriter& logger)
{
	//TODO
}
INTRA_END

#ifdef INTRA_TEST_MAIN
#include "Extra/IO/ConsoleInput.h"
#include "Extra/IO/ConsoleOutput.h"
#include "Extra/IO/Std.h"
INTRA_BEGIN
int main()
{
	return RunUnitTests(Std);
}
INTRA_END
#endif
