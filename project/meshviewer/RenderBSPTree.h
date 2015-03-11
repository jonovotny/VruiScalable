/***********************************************************************
RenderBSPTree - Class to render triangle meshes efficiently using a BSP
tree and and portals.
Copyright (c) 2009 Oliver Kreylos
***********************************************************************/

#ifndef RENDERBSPTREE_INCLUDED
#define RENDERBSPTREE_INCLUDED

#include <vector>
#include <Geometry/Point.h>
#include <Geometry/Box.h>
#include <Geometry/Plane.h>
#include <GL/gl.h>
#include <GL/GLObject.h>

#include "Material.h"
#include "MeshVertex.h"

/* Forward declarations: */
namespace Misc {
class File;
}
namespace Geometry {
template <class ScalarParam,int dimensionParam>
class ProjectiveTransformation;
}

class RenderBSPTree:public GLObject
	{
	/* Embedded classes: */
	public:
	typedef unsigned int Card;
	typedef std::vector<Card> CardList;
	typedef float Scalar;
	typedef Geometry::Point<Scalar,3> Point;
	typedef Geometry::Box<Scalar,3> Box;
	typedef Geometry::Plane<Scalar,3> Plane;
	typedef MeshVertex<Scalar> Vertex;
	typedef std::vector<Vertex> VertexList;
	typedef Geometry::ProjectiveTransformation<Scalar,3> PTransform;
	typedef Geometry::Box<Scalar,2> ScreenBox;
	
	private:
	struct TriangleFragment // Helper structure to represent triangle fragments during BSP tree creation
		{
		/* Elements: */
		public:
		Card originalIndex; // Index of triangle from which this fragment originated
		Point v[3]; // Triangle vertices
		};
	
	typedef std::vector<TriangleFragment> TriangleFragmentList; // Type for lists of triangle fragments
	typedef std::vector<Point> Polygon; // Type for polygons
	
	struct Node // Structure to represent BSP tree nodes
		{
		/* Elements: */
		public:
		Node* children; // Pointer to array of two child nodes for interior nodes
		Plane plane; // Separating plane for interior nodes
		std::vector<Polygon> portals; // List of portal polygons for interior nodes
		Card leafIndex; // Index of leaf node structure for leaf nodes
		
		/* Constructors and destructors: */
		Node(void) // Creates an empty leaf node
			:children(0)
			{
			}
		~Node(void) // Destroys the node and its descendants
			{
			delete[] children;
			}
		};
	
	struct Leaf // Structure to explicitly represent BSP tree leaf nodes
		{
		/* Embedded classes: */
		public:
		struct SubMesh // Structure to represent sets of polygons with the same material
			{
			/* Elements: */
			public:
			Card firstVertexIndex; // Index of the submesh's first vertex in the vertex array
			Card firstTriangleIndex; // Sub mesh's first triangle index in the node's triangle index array
			Card numTriangles; // Number of triangles in this submesh
			MaterialPointer material; // Material for this submesh
			};
		
		typedef std::vector<SubMesh> SubMeshList; // Type for lists of submeshes
		
		struct Portal // Structure to represent portal polygons
			{
			/* Elements: */
			public:
			Card otherLeafIndex; // Index of leaf node on other side of portal
			Plane plane; // The portal's plane, oriented such that its normal vector points out of the leaf
			Polygon portal; // The portal polygon
			};
		
		/* Elements: */
		public:
		CardList triangleIndices; // List of indices of contained triangles
		SubMeshList subMeshes; // List of submeshes
		std::vector<Portal> portals; // List of portals connecting to other leaf nodes
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
	private:
	const VertexList& vertices; // List of mesh vertices defining the triangles
	Node root; // Root node of BSP tree
	Card totalNumVertices; // Total number of vertices in the BSP tree nodes
	std::vector<Leaf> leaves; // List of BSP tree leaf nodes
	
	// DEBUGGING
	Card numAddedTriangles;
	mutable Card numRenderedNodes;
	mutable Card numRenderedTriangles;
	
	/* Private methods: */
	void loadNode(Node& node,Misc::File& file);
	void assignPortal(int pass,const Plane& plane,Node& node,Node& portalNode,const Polygon& portal);
	void createPortals(Node& node);
	static void splitTriangle(Card triangleIndex,const Point& v0,const Point& v1,const Point& v2,const Plane& splitPlane,TriangleFragmentList triangleFragments[2]);
	void addNodeTriangles(Node& node,const CardList& triangleIndices,const TriangleFragmentList& triangleFragments,Material* material);
	void finalizeNode(Node& node);
	void uploadNodeTriangles(const Node& node,Scalar* vertexBuffer) const;
	void renderLeaf(Card leafIndex,const Point& traversalStart,const PTransform& pmv,const ScreenBox& viewport,bool renderedLeaves[],GLContextData& contextData,Material*& currentMaterial) const;
	
	/* Constructors and destructors: */
	public:
	RenderBSPTree(const VertexList& sVertices); // Creates BSP tree for given vertex list without initialization
	
	/* Methods from GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	
	/* Methods: */
	void loadTree(const char* bspTreeFileName); // Loads the BSP tree's structure from the given file
	void addTriangles(const CardList& triangleIndices,Material* material); // Adds the given set of triangles to the BSP tree
	void finalizeTree(void); // Finalizes the BSP tree after all triangles have been added
	
	void glRenderAction(GLContextData& contextData) const; // Renders the BSP tree in the given view frustum
	};

#endif
