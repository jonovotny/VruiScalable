/***********************************************************************
HierarchicalTriangleSet - Class to represent 3D objects as a tree of
sets of triangles, for efficient intersection tests and rendering using
OpenGL.
Copyright (c) 2009-2012 Oliver Kreylos
***********************************************************************/

#ifndef HIERARCHICALTRIANGLESET_INCLUDED
#define HIERARCHICALTRIANGLESET_INCLUDED

#include <string>
#include <vector>
#include <Misc/HashTable.h>
#include <Geometry/Box.h>
#include <Geometry/Ray.h>
#include <Geometry/Plane.h>
#include <GL/gl.h>
#include <GL/GLObject.h>

#include "HierarchicalTriangleSetBase.h"
#include "TriangleKdTree.h"
#include "RenderBSPTree.h"

/* Forward declarations: */
namespace Misc {
class File;
}
template <class ScalarParam>
class GLFrustum;

template <class MeshVertexParam>
class HierarchicalTriangleSet:public HierarchicalTriangleSetBase,public GLObject
	{
	/* Embedded classes: */
	public:
	typedef MeshVertexParam MeshVertex; // Type for mesh vertices
	typedef typename MeshVertex::Scalar MScalar; // Scalar type for points and vectors
	typedef typename MeshVertex::Point MPoint; // Type for points
	typedef typename MeshVertex::Vector MVector; // Type for vectors
	typedef typename MeshVertex::TPoint MTPoint; // Type for points in texture space
	typedef Geometry::Box<MScalar,3> MBox; // Type for axis-aligned boxes
	typedef Geometry::Ray<MScalar,3> MRay; // Type for rays
	typedef Geometry::Plane<MScalar,3> MPlane; // Type for planes
	using HierarchicalTriangleSetBase::Card; // Cardinal integer type
	typedef Misc::HashTable<Card,void> CardSet; // Set of cardinals
	typedef GLFrustum<MScalar> Frustum; // Type for view frusta
	
	class SubMesh:public HierarchicalTriangleSetBase::SubMesh // Structure for nodes in the submesh tree
		{
		friend class HierarchicalTriangleSet;
		
		/* Elements: */
		private:
		MBox boundingBox; // Bounding box of all triangles in this submesh, and all child submeshes
		
		/* Methods from HierarchicalTriangleSetBase::SubMesh: */
		virtual Box getBoundingBox(void) const
			{
			return Box(boundingBox);
			}
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
	std::vector<MeshVertex> vertices; // List of mesh vertices
	std::vector<SubMesh> subMeshes; // List of mesh parts
	SubMesh currentSubMesh; // The currently added submesh
	TriangleKdTree triangleKdTree; // Kd-tree to support intersection tests
	RenderBSPTree* bspTree; // BSP tree to support view-dependent rendering
	
	// DEBUGGING
	/* Elements to indicate picking state: */
	mutable SubMesh* lastIntersected; // Pointer to last intersected submesh
	mutable MaterialPointer lastIntersectedMaterial; // Original material properties of last intersected submesh
	mutable MaterialPointer intersectedMaterial; // Material properties used to indicate last intersected submesh
	mutable unsigned int numTestedSubMeshes;
	mutable Point lastP0,lastP1;
	
	/* Private methods: */
	bool limitRay(Point& p0,Point& p1) const; // Limits a ray to the extents of the model's bounding box; returns true if ray intersects domain
	MScalar intersectSubMesh(Card subMeshIndex,const MRay& ray,MScalar lambdaMin,MScalar lambdaMax) const;
	
	/* Constructors and destructors: */
	public:
	HierarchicalTriangleSet(void); // Creates empty indexed triangle set
	virtual ~HierarchicalTriangleSet(void); // Destroys the indexed triangle set
	
	/* Methods from PolygonModel: */
	virtual Box calcBoundingBox(void) const;
	virtual void glRenderAction(GLContextData& contextData) const;
	virtual Point intersect(const Point& p0,const Point& p1) const;
	virtual Scalar traceBox(const Box& box,const Vector& displacement,Vector& hitNormal) const;
	virtual void loadBSPTree(const char* bspTreeFileName);
	
	/* Methods from HierarchicalTriangleSetBase: */
	virtual const HierarchicalTriangleSetBase::SubMesh* findSubMesh(const Point& p0,const Point& p1) const;
	virtual const HierarchicalTriangleSetBase::SubMesh& getParent(const HierarchicalTriangleSetBase::SubMesh& mesh) const
		{
		return subMeshes[mesh.parentIndex];
		}
	virtual const SubMesh& getChild(const HierarchicalTriangleSetBase::SubMesh& mesh,size_t childIndex) const
		{
		return subMeshes[mesh.childIndices[childIndex]];
		}
	virtual void drawSubMesh(const HierarchicalTriangleSetBase::SubMesh& mesh,GLContextData& contextData) const;
	
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
	Card addVertex(const MeshVertex& newVertex); // Adds a new vertex to the current submesh and returns its index
	void setSubMeshParentIndex(Card parentIndex); // Sets the parent node index of the current submesh
	void setSubMeshName(const std::string& name); // Sets the name of the current submesh
	void setSubMeshMaterial(MaterialPointer material); // Sets the material of the current submesh
	Card finishSubMesh(void); // Finishes the current submesh, adds it to the given parent submesh, and returns its index
	void sortSubMeshes(void); // Sorts submeshes by material properties
	void createKdTree(void); // Initializes the mesh's collision kd-tree
	};

#ifndef HIERARCHICALTRIANGLESET_IMPLEMENTATION
#include "HierarchicalTriangleSet.cpp"
#endif

#endif
