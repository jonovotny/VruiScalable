/***********************************************************************
MeshVertex - Structure to represent mesh vertices with a set of vertex
attributes.
Copyright (c) 2009 Oliver Kreylos
***********************************************************************/

#ifndef MESHVERTEX_INCLUDED
#define MESHVERTEX_INCLUDED

#include <Geometry/Point.h>
#include <Geometry/Vector.h>

template <class ScalarParam>
class MeshVertex
	{
	/* Embedded classes: */
	public:
	typedef ScalarParam Scalar; // Scalar type for points and vectors
	typedef Geometry::Point<Scalar,3> Point; // Type for points
	typedef Geometry::Vector<Scalar,3> Vector; // Type for vectors
	typedef Geometry::Point<Scalar,2> TPoint; // Type for points in texture space
	
	/* Elements: */
	TPoint texCoord; // Vertex texture coordinates
	Vector tangentS,tangentT; // Vertex tangent vectors in texture s and t directions
	Vector normal; // Vertex normal vector
	Point position; // Vertex position
	
	/* Constructors and destructors: */
	MeshVertex(void) // Dummy constructor
		{
		}
	MeshVertex(const Point& sPosition) // Creates a vertex from a position only
		:position(sPosition)
		{
		}
	MeshVertex(Scalar sX,Scalar sY,Scalar sZ) // Ditto
		:position(sX,sY,sZ)
		{
		}
	
	/* Operators: */
	bool operator!=(const MeshVertex& other) const // Inequality operator
		{
		/* Test for position equality: */
		return position!=other.position;
		}
	};

#endif
