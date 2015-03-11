/***********************************************************************
TriangleSet - Class to represent 3D objects as sets of triangles, for
efficient rendering using OpenGL.
Copyright (c) 2009 Oliver Kreylos
***********************************************************************/

#ifndef TRIANGLESET_INCLUDED
#define TRIANGLESET_INCLUDED

#include <vector>
#include <Geometry/AffineTransformation.h>
#include <GL/gl.h>
#include <GL/GLObject.h>

#include "Material.h"
#include "PolygonModel.h"

template <class MeshVertexParam>
class TriangleSet:public PolygonModel,public GLObject
	{
	/* Embedded classes: */
	public:
	typedef MeshVertexParam MeshVertex; // Type for mesh vertices
	typedef typename MeshVertex::Scalar MScalar; // Scalar type for points and vectors
	typedef typename MeshVertex::Point MPoint; // Type for points
	typedef typename MeshVertex::Vector MVector; // Type for vectors
	typedef typename MeshVertex::TPoint MTPoint; // Type for points in texture space
	typedef unsigned int Card; // Cardinal integer type
	typedef Geometry::AffineTransformation<MScalar,3> Transform; // Type for model transformations
	
	struct SubMesh // Structure for mesh parts sharing common material properties
		{
		/* Elements: */
		public:
		Card materialIndex; // Index of the material properties of this mesh part
		Card numTriangles; // Number of triangles belonging to this mesh part, stored consecutively in the vertex index array
		Card firstTriangleVertexIndex; // Index of first triangle vertex belonging to this mesh part
		};
	
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
	std::vector<MaterialPointer> materials; // List of materials used by mesh parts
	std::vector<MeshVertex> vertices; // List of mesh vertices
	std::vector<SubMesh> subMeshes; // List of mesh parts
	SubMesh currentSubMesh; // The currently added submesh
	
	/* Constructors and destructors: */
	public:
	TriangleSet(void); // Creates empty indexed triangle set
	virtual ~TriangleSet(void); // Destroys the indexed triangle set
	
	/* Methods from PolygonModel: */
	virtual Box calcBoundingBox(void) const;
	virtual void glRenderAction(GLContextData& contextData) const;
	virtual Point intersect(const Point& p0,const Point& p1) const;
	
	/* Methods from GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	
	/* New Methods: */
	Card getNumMaterials(void) const // Returns the current number of materials in the triangle set
		{
		return materials.size();
		}
	MaterialPointer getMaterial(Card materialIndex) const // Returns the material of the given index
		{
		return materials[materialIndex];
		}
	Card addMaterial(MaterialPointer newMaterial); // Adds a new material to the triangle set and returns its index
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
	Card addVertex(const MeshVertex& newVertex); // Adds a new vertex to the triangle set and returns its index
	void setSubMeshMaterial(Card materialIndex); // Sets the material index of the current submesh
	void finishSubMesh(void); // Finishes the current submesh
	void addTriangleSet(const TriangleSet& other,const Transform& vertexTransform); // Transforms the given triangle set and joins it with this triangle set
	};

#ifndef TRIANGLESET_IMPLEMENTATION
#include "TriangleSet.cpp"
#endif

#endif
