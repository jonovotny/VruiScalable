/***********************************************************************
HierarchicalTriangleSetBase - Base class to represent 3D objects as a
tree of sets of triangles, for efficient intersection tests and
rendering using OpenGL.
Copyright (c) 2010 Oliver Kreylos
***********************************************************************/

#ifndef HIERARCHICALTRIANGLESETBASE_INCLUDED
#define HIERARCHICALTRIANGLESETBASE_INCLUDED

#include <string>
#include <vector>

#include "Material.h"
#include "PolygonModel.h"

/* Forward declarations: */
template <class MeshVertexParam>
class HierarchicalTriangleSet;

class HierarchicalTriangleSetBase:public PolygonModel
	{
	/* Embedded classes: */
	public:
	typedef unsigned int Card; // Cardinal integer type
	typedef std::vector<Card> CardList; // List of cardinals
	
	class SubMesh // Base class for nodes in a triangle set's mesh graph
		{
		friend class HierarchicalTriangleSetBase;
		template <class MeshVertexParam>
		friend class HierarchicalTriangleSet;
		
		/* Elements: */
		private:
		Card parentIndex; // Index of submesh's parent
		std::string name; // Name of this submesh, mostly for debugging purposes
		MaterialPointer material; // The material properties of this submesh
		Card numTriangles; // Number of triangles belonging to this submesh, stored consecutively in the vertex index array
		Card firstTriangleVertexIndex; // Index of first triangle vertex belonging to this submesh
		CardList childIndices; // List of indices of submesh's children
		
		/* Methods: */
		public:
		std::string getName(void) const
			{
			return name;
			}
		Card getNumTriangles(void) const
			{
			return numTriangles;
			}
		size_t getNumChildren(void) const
			{
			return childIndices.size();
			}
		virtual Box getBoundingBox(void) const =0;
		};
	
	/* Methods: */
	virtual const SubMesh* findSubMesh(const Point& p0,const Point& p1) const =0; // Returns the first submesh intersected by the given ray
	virtual const SubMesh& getParent(const SubMesh& mesh) const =0; // Returns the given submesh's parent
	virtual const SubMesh& getChild(const SubMesh& mesh,size_t childIndex) const =0; // Returns the submesh's child of the given index
	virtual void drawSubMesh(const SubMesh& mesh,GLContextData& contextData) const =0; // Draws the given submesh
	};

#endif
