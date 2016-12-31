#pragma once

#include "Core/Core.h"
#include "Math/Shapes.h"
#include "Range/ArrayRange.h"
#include "Containers/Array.h"

namespace Intra {

class AabbTree
{
public:
	typedef Math::Triangle<float> Tri;
	typedef Math::AABB<float> Box;

	AabbTree(): rootId(-1), nodes() {}
	~AabbTree() {}

	void Build(ArrayRange<Tri> tris);

	const AabbTree::Tri* GetIntersection(const Math::Ray<float>& ray, Math::Vec3& position, Math::Vec3& normal) const;

	template<class GEOM_PRIMITIVE> void GetIntersection(const GEOM_PRIMITIVE& gp, Array<Tri>& contacts) const
  	 {
		 if(rootId==-1) return;
		 recursive_get_intersection(rootId, gp, contacts);
	}

	void GetIntersection(const AabbTree& tree, const Math::Mat4& m, Array<Tri>& contacts) const;

	AabbTree(AabbTree&& rhs): rootId(rhs.rootId), nodes(Meta::Move(rhs.nodes)) {}
	AabbTree(const AabbTree& rhs): rootId(rhs.rootId), nodes(rhs.nodes) {}

	AabbTree& operator=(AabbTree&& rhs)
	{
		rootId = rhs.rootId;
		nodes = Meta::Move(rhs.nodes);
		return *this;
	}

	AabbTree& operator=(const AabbTree& rhs) = default;

private:
	struct Node
	{
		union
		{
			Box box;
			Tri tri;
		};
		int positiveId=-1, negativeId=-1;

		int Id() const {return positiveId+1;}
		bool IsLeaf() const {return negativeId==-1;}
	};
	int rootId;
	Array<Node> nodes;

	float middle_point_of_triangle_proj(const Tri& tri, size_t indexOfMaxBoxDimension) const;
	void recursive_build(int& pNode, ArrayRange<Tri> tris);
	template<class GEOM_PRIMITIVE> void recursive_get_intersection(int nodeId, const GEOM_PRIMITIVE& gp, Array<Tri>& contacts) const;
	void recursive_get_intersection(int myNodeId, const AabbTree* outside,
		int outsideNodeId, const Math::Mat4& m, Array<Tri>& contacts) const;
};



// Шаблонная функция-член класса, проверяет рекурсивно пересечение примитива с AABB tree
template<class GEOM_PRIMITIVE> void AabbTree::recursive_get_intersection(
	int nodeId, const GEOM_PRIMITIVE& gp, Array<Tri>& contacts) const
{
	INTRA_ASSERT(nodeId!=-1);

	auto pNode = &nodes[size_t(nodeId)];
	if(pNode->IsLeaf())
	{
		if(!TestIntersection(pNode->tri, gp)) return;
		contacts.AddLast(pNode->tri);
		return;
	}
	if(TestIntersection(pNode->box, gp))
	{
		recursive_get_intersection(pNode->positiveId, gp, contacts);
		if(pNode->negativeId!=-1) recursive_get_intersection(pNode->negativeId, gp, contacts);
	}
}

}

