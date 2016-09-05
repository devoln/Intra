#include "Physics/AabbTree.h"
#include "IO/LogSystem.h"
//#include "Core/Time.h"

namespace Intra {

using namespace core;
using namespace Math;

void AabbTree::Build(ArrayRange<Tri> tris)
{
	//Timer tim;

	nodes.SetCount(0);
	nodes.Reserve(tris.Count()*2-1);
	rootId=-1;

	if(tris!=null) recursive_build(rootId, tris);

	//double t=tim.GetTime();
	//IO::InfoLog << "Время построения AabbTree: " << t*1000 << " мс. "
	//	"Число треугольников: " << tris.Count() << ", узлов: " << nodes.Count() << IO::endl;
}

// Относит треугольник к позитивному или негативному множеству
float AabbTree::middle_point_of_triangle_proj(const Tri& tri, int maxDimIndex) const
{
	const Vec3* const V = tri.vertices;
	const float c0 = V[0][maxDimIndex], c1 = V[1][maxDimIndex], c2 = V[2][maxDimIndex];
	const float minC = Min(Min(c0, c1), c2);
	const float maxC = Max(Max(c0, c1), c2);
	const float middlePointOfTriangleProj=(minC+maxC)*0.5f;

	return middlePointOfTriangleProj;
}

static float triangle_proj(const Triangle<float>& tri, int maxDimIndex)
{
	const Vec3* const V = tri.vertices;
	const float c0 = V[0][maxDimIndex], c1 = V[1][maxDimIndex], c2 = V[2][maxDimIndex];
	const float minC = Min(Min(c0, c1), c2), maxC = Max(Max(c0, c1), c2);
	return (maxC-minC)*0.5f;
}

// Рекурсивно строит дерево
void AabbTree::recursive_build(int& nodeId, ArrayRange<Tri> tris)
{
	INTRA_ASSERT(tris!=null);
	INTRA_ASSERT(nodeId==-1);

	Node node;

	if(tris.Count()==1) //Это лист
	{
		node.tri = tris[0];
		nodeId = (int)nodes.Count();
		node.positiveId=nodeId-1;
		nodes.AddLast(node);
		return;
	}

	// Вычисляем ограничивающий бокс
	node.box.min = Vec3(Meta::NumericLimits<float>::Max());
	node.box.max = Vec3(-Meta::NumericLimits<float>::Max());
	for(auto& tri: tris) node.box.AddTriangle(tri);

	// Ищем максимальный размер
	int maxDimIndex = 0;
	node.box.MaxSizeAxis(&maxDimIndex);

	//Делим треугольники на два подмножества
	auto pEndNegative = tris.Begin;
	auto pBeginPositive = tris.End;
	const float middlePointOfBox = (node.box.min[maxDimIndex] + node.box.max[maxDimIndex])*0.5f;
	while(pEndNegative!=pBeginPositive)
	{
		if(middle_point_of_triangle_proj(*pEndNegative, maxDimIndex)>middlePointOfBox)
		{
			pBeginPositive--;
			swap(*pEndNegative, *pBeginPositive);
			continue;
		}
		pEndNegative++;
	}

	// Подмножество может оказаться пустым.
	// Для борьбы с эти ассертом надо выбрать наибольший треугольник
	// и отправить его в пустое подмножество.
	if(pBeginPositive==tris.End)
	{
		float maxproj=0;
		Tri* maxtri=tris.Begin;
		for(Tri* tri=tris.Begin; tri<tris.End; tri++)
		{
			const float proj = triangle_proj(*pEndNegative, maxDimIndex);
			if(proj>maxproj)
			{
				maxproj = proj;
				maxtri=tri;
			}
		}
		pBeginPositive--, pEndNegative--;
		swap(*maxtri, *pBeginPositive);
	}
	INTRA_ASSERT(tris.Begin<pEndNegative && pEndNegative==pBeginPositive && pBeginPositive<tris.End);


	recursive_build(node.negativeId, {tris.Begin, pEndNegative});
	recursive_build(node.positiveId, {pBeginPositive, tris.End});

	nodeId = node.Id();
	INTRA_ASSERT(nodes.Count()==(uint)node.Id());
	nodes.AddLast(node);
}



void AabbTree::GetIntersection(const AabbTree& ABBTree, const Mat4& m, Array<Tri>& contacts) const
{
	if(rootId==-1 || ABBTree.rootId==-1) return;
	recursive_get_intersection(rootId, &ABBTree, ABBTree.rootId, m, contacts);
}


// Рекурсивно проверяет пересечение двух деревьев.
// m - матрица перехода из системы координат pOutsideNode в систему координат pMyNode.
void AabbTree::recursive_get_intersection(int myNodeId, const AabbTree* outside,
	int outsideNodeId, const Mat4& m, Array<Tri>& contacts) const
{
	const auto pMyNode=&nodes[myNodeId];
	const auto pOutsideNode=&outside->nodes[outsideNodeId];
	if(pMyNode->IsLeaf())
	{
		// pMyNode-лист, pOutsideNode-узел или лист
		Tri triangle = pMyNode->tri * Inverse(m);
		outside->recursive_get_intersection(outsideNodeId, triangle, contacts);
		return;
	}

	if(pOutsideNode->IsLeaf())
	{// pMyNode-узел, pOutsideNode-лист
		Tri triangle = pOutsideNode->tri * m;
		recursive_get_intersection(myNodeId, triangle, contacts);
	}
	// оба узлa
	if(pMyNode->box>pOutsideNode->box) //не совсем корректное (без учета матрицы m) сравнение размеров "боксов"
	{
		recursive_get_intersection(pMyNode->positiveId, outside, outsideNodeId, m, contacts);
		recursive_get_intersection(pMyNode->negativeId, outside, outsideNodeId, m, contacts);
		return;
	}
	recursive_get_intersection(myNodeId, outside, pOutsideNode->positiveId, m, contacts);
	recursive_get_intersection(myNodeId, outside, pOutsideNode->negativeId, m, contacts);
}

}

