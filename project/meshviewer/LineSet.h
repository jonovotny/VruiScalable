/***********************************************************************
LineSet - Class to represent sets of 3D lines.
Copyright (c) 2009-2011 Oliver Kreylos
***********************************************************************/

#ifndef LINESET_INCLUDED
#define LINESET_INCLUDED

#include <vector>
#include <GL/gl.h>
#include <GL/GLColor.h>
#include <GL/GLObject.h>

#include "PolygonModel.h"

template <class MeshVertexParam>
class LineSet:public PolygonModel,public GLObject
	{
	/* Embedded classes: */
	public:
	typedef MeshVertexParam MeshVertex; // Type for mesh vertices
	typedef typename MeshVertex::Scalar MScalar; // Scalar type for points and vectors
	typedef typename MeshVertex::Point MPoint; // Type for points
	typedef GLColor<GLfloat,3> Color; // Type for colors
	typedef unsigned int Card; // Cardinal integer type
	
	struct SubMesh // Structure for mesh parts sharing common material properties
		{
		/* Elements: */
		public:
		Color color; // Color for this mesh part
		Card firstLineVertexIndex; // Index of first line vertex belonging to this mesh part
		Card numVertices; // Number of vertices belonging to this mesh part, stored consecutively in the vertex array
		};
	
	private:
	struct DataItem:public GLObject::DataItem
		{
		/* Elements: */
		public:
		GLuint vertexBufferId; // ID of vertex buffer object for point data (or 0 if extension is not supported)
		
		/* Constructors and destructors: */
		DataItem(void);
		virtual ~DataItem(void);
		};
	
	/* Elements: */
	std::vector<MeshVertex> vertices; // List of line vertices
	std::vector<SubMesh> subMeshes; // List of mesh parts
	SubMesh currentSubMesh; // Submesh receiving added vertices
	
	/* Constructors and destructors: */
	public:
	LineSet(void); // Creates empty line set
	virtual ~LineSet(void); // Destroys the line set
	
	/* Methods from PolygonModel: */
	virtual Box calcBoundingBox(void) const;
	virtual void glRenderAction(GLContextData& contextData) const;
	virtual Point intersect(const Point& p0,const Point& p1) const;
	
	/* Methods from GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	
	/* New Methods: */
	Card getNumVertices(void) const // Returns the current number of vertices in the triangle set
		{
		return vertices.size();
		}
	const MeshVertex& getVertex(Card vertexIndex) const // Returns a vertex
		{
		return vertices[vertexIndex];
		}
	MeshVertex& getVertex(Card vertexIndex) // Ditto
		{
		return vertices[vertexIndex];
		}
	Card addVertex(const MeshVertex& newVertex); // Adds a new vertex to the line set and returns its index
	void setSubMeshColor(const Color& newColor); // Sets the color of the current mesh part
	Card finishSubMesh(void); // Finishes adding vertices to a mesh part and returns its index
	Card getNumSubMeshes(void) const // Returns the number of mesh parts in the line set
		{
		return subMeshes.size();
		}
	const SubMesh& getSubMesh(Card subMeshIndex) const // Returns the mesh part of the given index
		{
		return subMeshes[subMeshIndex];
		}
	void setSubMeshColor(Card subMeshIndex,const Color& newColor); // Sets the color of a mesh part
	};

#ifndef LINESET_IMPLEMENTATION
#include "LineSet.cpp"
#endif

#endif
