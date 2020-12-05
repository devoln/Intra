#include "TestRun.h"

#include "Intra/Assert.h"
#include "IntraX/Utils/Logger.h"
#include "IntraX/IO/FormattedWriter.h"

INTRA_BEGIN
void RunUnitTests(FormattedWriter& logger)
{
	//TODO
}
INTRA_END

#ifdef INTRA_TEST_MAIN
#include "IntraX/IO/ConsoleInput.h"
#include "IntraX/IO/ConsoleOutput.h"
#include "IntraX/IO/Std.h"
INTRA_BEGIN
int main()
{
	return RunUnitTests(Std);
}
INTRA_END
#endif
