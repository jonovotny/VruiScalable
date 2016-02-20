/***********************************************************************
Tesselator - Helper class to split simple polygons into triangles.
Copyright (c) 2010 Oliver Kreylos
***********************************************************************/

#ifndef TESSELATOR_INCLUDED
#define TESSELATOR_INCLUDED

#include <Geometry/Vector.h>

template <class VertexParam>
class Tesselator
	{
	/* Embedded classes: */
	public:
	typedef VertexParam Vertex; // Type for polygon vertices
	typedef typename Vertex::Point Point; // Type for points
	typedef typename Point::Scalar Scalar; // Scalar type
	typedef typename Point::Vector Vector; // Type for vectors
	typedef unsigned int Card; // Type for cardinal numbers
	typedef unsigned int Index; // Type for vertex indices;
	
	/* Elements: */
	private:
	const Vertex* vertices; // Array containing vertices on which the tesselator operates
	Card maxNumVertices; // Maximum number of polygon vertices in the current data structures
	Index* polygonVertexIndices; // Array of polygon vertex indices
	bool* concaveFlags; // Array of polygon vertex concavity flags
	Card numVertices; // Number of vertices left in current polygon
	Index* concaveVertexIndices; // Array of indices of concave vertices
	Card numConcaveVertices; // Number of concave vertices left in current polygon
	Index* triangleVertexIndices; // Array of vertex indices for each created triangle
	Card numTriangles; // Number of triangles already created from current polygon
	
	/* Private methods: */
	void setMaxNumVertices(Card newMaxNumVertices); // Re-allocates data structures to handle polygons up to the given size
	
	/* Constructors and destructors: */
	public:
	Tesselator(Card sMaxNumVertices =3); // Creates a tesselator capable of handling the given number of polygon vertices
	~Tesselator(void);
	
	/* Methods: */
	void setVertices(const Vertex* newVertices) // Sets the vertex array to be used by the tesselator
		{
		vertices=newVertices;
		}
	void reset(Card minNumVertices =3); // Resets the tesselator to receive another polygon of at least the given number of vertices
	void addVertex(Index vertexIndex) // Adds a vertex to the current polygon
		{
		/* Make room in the data structures: */
		if(numVertices==maxNumVertices)
			setMaxNumVertices((maxNumVertices*3)/2+1);
		
		/* Store the vertex: */
		polygonVertexIndices[numVertices]=vertexIndex;
		++numVertices;
		}
	void tesselate(const Vector& planeNormal); // Tesselates the current polygon; assumes polygon is counter-clockwise around the given plane normal vector
	Card getNumTriangles(void) const // Returns the number of generated triangles
		{
		return numTriangles;
		}
	const Index* getTriangleVertexIndices(void) const // Returns the triangle vertex index array
		{
		return triangleVertexIndices;
		}
	};

#ifndef TESSELATOR_IMPLEMENTATION
#include "Tesselator.icpp"
#endif

#endif
