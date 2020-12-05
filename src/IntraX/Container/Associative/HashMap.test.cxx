#include "HashMap.h"
#include "IntraX/Container/Associative/HashMap.h"

using namespace Intra;

void TestMaps(FormattedWriter& output)
{
	HashMap<String, int> map;
	map["Строка"] = 6;
	map["Тест"] = 4;
	map["Вывод"] = 5;
	map["Ассоциативного"] = 14;
	map["Массива"] = 7;
	map["HashMap"] = 11;

	output.PrintLine("Заполнили HashMap, выведем его:");
	output.PrintLine(map);
	output.LineBreak();

	auto mapRange = map.Find("Вывод");
	mapRange.PopLast();

	output.PrintLine("Выведем все элементы, вставленные начиная от \"Вывод\" и до предпоследнего элемента:");
	output.PrintLine(mapRange);
	output.LineBreak();

	map.SortByKey();
	output.PrintLine("Отсортируем элементы HashMap на месте:");
	output.PrintLine(map);
}

