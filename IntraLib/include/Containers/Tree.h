#pragma once

#include "Array.h"

namespace Intra {

template<typename T> struct Tree
{
	explicit Tree(const T& value=T(), size_t nodeCount=0):
		Nodes(nodeCount), Value(value) {AddNodes(nodeCount);}

	Tree(const Tree& rhs) = default;

	Tree(const T& value, ArrayRange<const T> nodeValues):
		Nodes(nodeValues.Count()), Value(value) {AddNodes(nodeValues);}

	void AddNodes(size_t nodeCount=0, const T& value=T())
	{
		for(size_t i=0; i<nodeCount; i++)
			Nodes.AddLast(Tree(value));
	}

	void AddNodes(ArrayRange<const T> nodeValues) {for(auto& v: nodeValues) Nodes.AddLast(Tree(v));}
	void AddNodes(const Tree& rootOfNodes) {for(auto& n: rootOfNodes) Nodes.AddLast(n);}

	Tree& operator=(const Tree& rhs) = default;

	Array<Tree>* operator->() {return &Nodes;}
	Tree& operator[](size_t index) {return Nodes[index];}
	const Array<Tree>* operator->() const {return &Nodes;}
	const Tree& operator[](size_t index) const {return Nodes[index];}

	Array<Tree> Nodes;
	T Value;
};

}
