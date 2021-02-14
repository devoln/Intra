#include "Intra/Functional.h"
#include "Intra/Range/Reduce.h"
#include "Intra/Range/Span.h"
#include "Intra/Range/Cycle.h"
#include "Intra/Range/Map.h"
#include "Intra/Range/Recurrence.h"
#include "Intra/Range/Generate.h"
#include "Intra/Range/Filter.h"
#include "Intra/Range/Stream/ToString.h"
#include "Intra/Range/Polymorphic/InputRange.h"
#include "Intra/Range/Polymorphic/RandomAccessRange.h"

#include "IntraX/System/Debug.h"
#include "IntraX/Container/Sequential/List.h"
#include "IntraX/Unstable/Random/FastUniform.h"

namespace Intra { INTRA_BEGIN

static int SumPolymorphicRange(InputRange<int> ints)
{
	int sum = 0;
	while(!ints.Empty())
		sum += ints.Next();
	return sum;
}


struct ivec3
{
	int x, y, z;
	template<typename V> void ForEachField(V&& f) const {f(x), f(y), f(z);}
};

INTRA_MODULE_UNITTEST
{
	int ints[] = {3, 53, 63, 3, 321, 34253, 35434, 2};
	int sum = SumPolymorphicRange(ints);
	output.PrintLine("sum of ", ints, " = ", sum);

	InputRange<const char> myRange = StringView("Диапазон");
	char c[40];
	auto r = Span<char>(c);
	r << Move(myRange);

	ivec3 vectors[] = {{1, 2, 3}, {1, 64, 7}, {43, 5, 342}, {5, 45, 4}};
	RandomAccessRange<ivec3&> vectors1;
	vectors1 = vectors;
	vectors1[1] = {2, 3, 4};
	InputRange<int> xvectors = Map(vectors, [](const ivec3& v) {return v.x;});
	int xsum = SumPolymorphicRange(Move(xvectors));
	output.PrintLine("x sum of ", vectors, " = ", xsum);


	auto someRecurrence = Take(Drop(Cycle(Take(Recurrence(
		[](int a, int b) {return a*2+b; }, 1, 1
	), 17)), 3), 22);
	output.PrintLine("Представляем сложную последовательность в виде полиморфного input-диапазона:");
	InputRange<int> someRecurrencePolymorphic = someRecurrence;
	PrintPolymorphicRange(output, Move(someRecurrencePolymorphic));

	output.PrintLine("Полиморфный диапазон seq содержит генератор 100 случайных чисел от 0 до 999 с отбором квадратов тех из них, которые делятся на 7: ");
	InputRange<unsigned> seq = Map(
		Filter(
			Take(Generate(Bind(FastUniform<unsigned>(), 1000)), 500),
			[](unsigned x) {return x % 7 == 0;}),
		Sqr<unsigned>);
	PrintPolymorphicRange(output, Move(seq));

	output.LineBreak();
	output.PrintLine("Присвоили той же переменной seq диапазон другого типа и выведем его снова:");
	seq = Take(Generate(rand), 50);
	PrintPolymorphicRange(output, Move(seq));
}

void TestPolymorphicRange(FormattedWriter& output)
{
	TestSumRange(output);
	TestComposedPolymorphicRange(output);
}
