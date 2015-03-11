/***********************************************************************
TriangleKdTree - Class to represent kd-trees to support intersection
tests with sets of triangles.
Copyright (c) 2009-2012 Oliver Kreylos
***********************************************************************/

#ifndef TRIANGLEKDTREE_INCLUDED
#define TRIANGLEKDTREE_INCLUDED

#include <vector>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/Box.h>

#include "MeshVertex.h"

class TriangleKdTree
	{
	/* Embedded classes: */
	public:
	typedef unsigned int Card;
	static const Card nil=~Card(0);
	typedef std::vector<Card> CardList;
	typedef float Scalar;
	typedef Geometry::Point<Scalar,3> Point;
	typedef Geometry::Vector<Scalar,3> Vector;
	typedef Geometry::Box<Scalar,3> Box;
	typedef MeshVertex<Scalar> Vertex;
	typedef std::vector<Vertex> VertexList;
	
	struct IntersectResult // Structure to report ray intersection results
		{
		/* Elements: */
		public:
		Point intersection; // Intersection point
		Card triangleIndex; // Index of intersected triangle
		Vector normal; // Normal vector of intersected triangle at intersection point
		};
	
	private:
	struct TriangleFragment // Helper structure to represent triangle fragments during kd-tree creation
		{
		/* Elements: */
		public:
		Card originalIndex; // Index of triangle from which this fragment originated
		Point v[3]; // Triangle vertices
		};
	
	typedef std::vector<TriangleFragment> TriangleFragmentList; // Type for lists of triangle fragments
	
	struct Node // Structure for kd-tree nodes
		{
		/* Elements: */
		public:
		Node* children; // Pointer to array of two child nodes for interior nodes
		int splitDimension; // Dimension along which the node is split
		Scalar plane; // Coordinate of splitting plane in node's dimension
		CardList triangleIndices; // List of triangles overlapping this leaf node's domain
		
		/* Constructors and destructors: */
		Node(void) // Creates empty leaf node
			:children(0)
			{
			}
		~Node(void) // Destroys the node and its descendants
			{
			delete[] children;
			}
		};
	
	/* Elements: */
	private:
	const VertexList& vertices; // List of mesh vertices defining the triangles
	Box boundingBox; // Bounding box around all triangles
	Card maxTrianglesPerNode; // Maximum number of triangles per kd-tree node
	Node root; // Root node of kd-tree
	
	// DEBUGGING
	// mutable Card numTestedTriangles;
	// mutable Card numTraversedNodes;
	
	/* Private methods: */
	Scalar findBestSplit(const Box& domain,int dimension,const CardList& triangleIndices,const TriangleFragmentList& triangleFragments) const;
	void splitTriangle(Card triangleIndex,const Point& v0,const Point& v1,const Point& v2,int dimension,Scalar splitPlane,TriangleFragmentList triangleFragments[2]);
	void initNode(Node& node,const Box& domain,const CardList& triangleIndices,const TriangleFragmentList& triangleFragments);
	void intersectNode(const Node& node,const Point& p0,const Point& p1,IntersectResult& result) const;
	void getTrianglesInBox(const Node& node,const Box& box,CardList& triangleIndices) const;
	void drawIntersectionNode(const Node& node,const Box& domain,const Point& p0,const Point& p1,bool drawTriangles) const;
	
	/* Constructors and destructors: */
	public:
	TriangleKdTree(const VertexList& sVertices); // Creates kd-tree for given vertex list without initialization
	
	/* Methods: */
	void createTree(const Box& sBoundingBox,Card sMaxTrianglesPerNode,const CardList& triangleIndices);
	IntersectResult intersect(const Point& p0,const Point& p1) const; // Intersects a ray segment with the kd-tree; returns intersection point; intersection is valid if result is not equal p1
	Scalar traceBox(const Box& box,const Vector& displacement,Vector& hitNormal) const; // Traces a box from the current position along the given displacement vector; returns amount of possible movement and sets hit normal vector if result smaller than 1.0
	void drawIntersection(const Point& p0,const Point& p1,bool drawTriangles) const; // Visualizes the intersection process of the given ray segment
	};

#endif
