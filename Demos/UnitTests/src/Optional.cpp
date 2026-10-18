#include "Core.h"
#include "Utils/Optional.h"
#include "IO/FormattedWriter.h"

using namespace Intra;

void TestOptionalAssignments(IO::FormattedWriter& output)
{
	(void)output;
	int source = 17;
	Utils::Optional<int> copyAssigned;
	copyAssigned = source;
	INTRA_ASSERT(copyAssigned != null);
	INTRA_ASSERT_EQUALS(copyAssigned.Value(), source);

	Utils::Optional<int> moveAssigned;
	moveAssigned = 23;
	INTRA_ASSERT(moveAssigned != null);
	INTRA_ASSERT_EQUALS(moveAssigned.Value(), 23);

	Utils::Optional<int> optionalAssigned;
	optionalAssigned = copyAssigned;
	INTRA_ASSERT(optionalAssigned != null);
	INTRA_ASSERT_EQUALS(optionalAssigned.Value(), source);
}
