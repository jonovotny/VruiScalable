/***********************************************************************
RenderBSPTree - Class to render triangle meshes efficiently using a BSP
tree and and portals.
Copyright (c) 2009 Oliver Kreylos
***********************************************************************/

#include "RenderBSPTree.h"

// DEBUGGING
#include <iostream>

#include <string.h>
#include <utility>
#include <Misc/ThrowStdErr.h>
#include <Misc/File.h>
#include <Geometry/ProjectiveTransformation.h>
#include <GL/gl.h>
#include <GL/GLVertexArrayParts.h>
#include <GL/GLVertexArrayTemplates.h>
#include <GL/GLContextData.h>
#include <GL/GLExtensionManager.h>
#include <GL/Extensions/GLARBVertexBufferObject.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>

namespace {

/***************
HelperFunctions:
***************/

template <class ScalarParam>
inline
void
splitPolygon(
	const Geometry::Plane<ScalarParam,3>& plane,
	const std::vector<Geometry::Point<ScalarParam,3> >& polygon,
	std::vector<Geometry::Point<ScalarParam,3> > parts[2])
	{
	typedef ScalarParam Scalar;
	typedef Geometry::Point<Scalar,3> Point;
	typedef std::vector<Point> Polygon;
	
	typename Polygon::const_iterator pIt0=polygon.end()-1;
	Scalar d0=plane.calcDistance(*pIt0);
	for(typename Polygon::const_iterator pIt1=polygon.begin();pIt1!=polygon.end();++pIt1)
		{
		Scalar d1=plane.calcDistance(*pIt1);
		
		/* Intersect the edge with the plane: */
		if(d0*d1<Scalar(0))
			{
			/* Add the edge's intersection point to both polygons: */
			Point intersection=Geometry::affineCombination(*pIt0,*pIt1,d0/(d0-d1));
			parts[0].push_back(intersection);
			parts[1].push_back(intersection);
			}
		
		/* Add the edge's end vertex to the front and/or back polygon parts: */
		if(d1<=Scalar(0))
			parts[0].push_back(*pIt1);
		if(d1>=Scalar(0))
			parts[1].push_back(*pIt1);
		
		/* Go to the next edge: */
		pIt0=pIt1;
		d0=d1;
		}
	}

template <class ScalarParam>
inline
Geometry::HVector<ScalarParam,3>
affineCombination(
	const Geometry::HVector<ScalarParam,3>& v0,
	const Geometry::HVector<ScalarParam,3>& v1,
	ScalarParam w1)
	{
	return Geometry::HVector<ScalarParam,3>(
		v0[0]+(v1[0]-v0[0])*w1,
		v0[1]+(v1[1]-v0[1])*w1,
		v0[2]+(v1[2]-v0[2])*w1,
		v0[3]+(v1[3]-v0[3])*w1);
	}

template <class ScalarParam>
inline
Geometry::Box<ScalarParam,2>
projectPortal(
	const std::vector<Geometry::Point<ScalarParam,3> >& portal,
	const Geometry::ProjectiveTransformation<ScalarParam,3>& pmv,
	const Geometry::Box<ScalarParam,2>& viewport)
	{
	typedef ScalarParam Scalar;
	typedef Geometry::Point<Scalar,3> Point;
	typedef Geometry::HVector<Scalar,3> HVector;
	typedef Geometry::Box<Scalar,2> ScreenBox;
	
	/* Create temporary arrays to hold clip-space vertices: */
	HVector* vertices[2];
	for(int i=0;i<2;++i)
		vertices[i]=new HVector[portal.size()+6]; // At most one new point per clip for convex polygons
	
	/* Transform the portal polygon to clip space: */
	HVector* dPtr=vertices[0];
	for(typename std::vector<Point>::const_iterator pIt=portal.begin();pIt!=portal.end();++pIt)
		*(dPtr++)=pmv.transform(HVector(*pIt));
	
	/* Clip against the front plane (z>=-w): */
	HVector* sEnd=dPtr;
	dPtr=vertices[1]; 
	HVector* s0=sEnd-1;
	for(HVector* s1=vertices[0];s1!=sEnd;++s1)
		{
		Scalar d0=(*s0)[3]+(*s0)[2];
		Scalar d1=(*s1)[3]+(*s1)[2];
		if(d0*d1<Scalar(0))
			*(dPtr++)=affineCombination(*s0,*s1,d0/(d0-d1));
		if(d1>=Scalar(0))
			*(dPtr++)=*s1;
		s0=s1;
		}
	
	/* Clip against the back plane (z<=w): */
	sEnd=dPtr;
	dPtr=vertices[0];
	s0=sEnd-1;
	for(HVector* s1=vertices[1];s1!=sEnd;++s1)
		{
		Scalar d0=(*s0)[3]-(*s0)[2];
		Scalar d1=(*s1)[3]-(*s1)[2];
		if(d0*d1<Scalar(0))
			*(dPtr++)=affineCombination(*s0,*s1,d0/(d0-d1));
		if(d1>=Scalar(0))
			*(dPtr++)=*s1;
		s0=s1;
		}
	
	/* Clip against the left viewport plane (x>=min*w): */
	sEnd=dPtr;
	dPtr=vertices[1];
	s0=sEnd-1;
	for(HVector* s1=vertices[0];s1!=sEnd;++s1)
		{
		Scalar d0=(*s0)[0]-viewport.min[0]*(*s0)[3];
		Scalar d1=(*s1)[0]-viewport.min[0]*(*s1)[3];
		if(d0*d1<Scalar(0))
			*(dPtr++)=affineCombination(*s0,*s1,d0/(d0-d1));
		if(d1>=Scalar(0))
			*(dPtr++)=*s1;
		s0=s1;
		}
	
	/* Clip against the right viewport plane (x<=max*w): */
	sEnd=dPtr;
	dPtr=vertices[0];
	s0=sEnd-1;
	for(HVector* s1=vertices[1];s1!=sEnd;++s1)
		{
		Scalar d0=viewport.max[0]*(*s0)[3]-(*s0)[0];
		Scalar d1=viewport.max[0]*(*s1)[3]-(*s1)[0];
		if(d0*d1<Scalar(0))
			*(dPtr++)=affineCombination(*s0,*s1,d0/(d0-d1));
		if(d1>=Scalar(0))
			*(dPtr++)=*s1;
		s0=s1;
		}
	
	/* Clip against the bottom viewport plane (y>=min*w): */
	sEnd=dPtr;
	dPtr=vertices[1];
	s0=sEnd-1;
	for(HVector* s1=vertices[0];s1!=sEnd;++s1)
		{
		Scalar d0=(*s0)[1]-viewport.min[1]*(*s0)[3];
		Scalar d1=(*s1)[1]-viewport.min[1]*(*s1)[3];
		if(d0*d1<Scalar(0))
			*(dPtr++)=affineCombination(*s0,*s1,d0/(d0-d1));
		if(d1>=Scalar(0))
			*(dPtr++)=*s1;
		s0=s1;
		}
	
	/* Clip against the top viewport plane (y<=max*w): */
	sEnd=dPtr;
	dPtr=vertices[0];
	s0=sEnd-1;
	for(HVector* s1=vertices[1];s1!=sEnd;++s1)
		{
		Scalar d0=viewport.max[1]*(*s0)[3]-(*s0)[1];
		Scalar d1=viewport.max[1]*(*s1)[3]-(*s1)[1];
		if(d0*d1<Scalar(0))
			*(dPtr++)=affineCombination(*s0,*s1,d0/(d0-d1));
		if(d1>=Scalar(0))
			*(dPtr++)=*s1;
		s0=s1;
		}
	
	ScreenBox result;
	if(dPtr-vertices[0]>=3)
		{
		/* Calculate the bounding box of the clipped polygon: */
		sEnd=dPtr;
		HVector* s=vertices[0];
		for(int i=0;i<2;++i)
			result.min[i]=result.max[i]=(*s)[i]/(*s)[3];
		for(++s;s!=sEnd;++s)
			for(int i=0;i<2;++i)
				{
				if(result.min[i]*(*s)[3]>(*s)[i])
					result.min[i]=(*s)[i]/(*s)[3];
				if(result.max[i]*(*s)[3]<(*s)[i])
					result.max[i]=(*s)[i]/(*s)[3];
				}
		}
	else
		result=ScreenBox::empty;
	
	/* Clean up: */
	for(int i=0;i<2;++i)
		delete[] vertices[i];
	
	return result;
	}

}

/****************************************
Methods of class RenderBSPTree::DataItem:
****************************************/

RenderBSPTree::DataItem::DataItem(void)
	:vertexBufferId(0)
	{
	if(GLARBVertexBufferObject::isSupported())
		{
		/* Initialize the vertex buffer object extension: */
		GLARBVertexBufferObject::initExtension();
		
		/* Create a vertex buffer object: */
		glGenBuffersARB(1,&vertexBufferId);
		}
	}

RenderBSPTree::DataItem::~DataItem(void)
	{
	if(vertexBufferId!=0)
		{
		/* Delete the vertex buffer object: */
		glDeleteBuffersARB(1,&vertexBufferId);
		}
	}

/******************************
Methods of class RenderBSPTree:
******************************/

void RenderBSPTree::loadNode(RenderBSPTree::Node& node,Misc::File& file)
	{
	/* Read the node's plane equation: */
	double center[3];
	file.read<double>(center,3);
	double normal[3];
	file.read<double>(normal,3);
	node.plane=Plane(normal,center);
	
	/* Read the portal polygons: */
	unsigned int numPortals=file.read<unsigned int>();
	for(unsigned int i=0;i<numPortals;++i)
		{
		Polygon newPortal;
		unsigned int numVertices=file.read<unsigned int>();
		for(unsigned int j=0;j<numVertices;++j)
			{
			double p[3];
			file.read<double>(p,3);
			newPortal.push_back(Point(p));
			}
		node.portals.push_back(newPortal);
		}
	
	/* Read the left and right subtrees: */
	if(node.children==0)
		node.children=new Node[2];
	for(int i=0;i<2;++i)
		if(file.read<unsigned char>()!=0)
			{
			/* Load the interior child node: */
			loadNode(node.children[i],file);
			}
		else
			{
			/* Create a new leaf structure for the leaf child node: */
			node.children[i].leafIndex=leaves.size();
			leaves.push_back(Leaf());
			}
	}

void RenderBSPTree::assignPortal(int pass,const RenderBSPTree::Plane& plane,RenderBSPTree::Node& node,RenderBSPTree::Node& portalNode,const RenderBSPTree::Polygon& portal)
	{
	if(node.children==0)
		{
		if(pass==0)
			{
			/* Start the second pass: */
			assignPortal(1,plane,portalNode.children[1],node,portal);
			}
		else
			{
			/* Calculate the relative orientation of the portal polygon and the portal plane: */
			Scalar po=plane.getNormal()*Geometry::cross(portal[1]-portal[0],portal[2]-portal[0]);
			
			/* Create a flipped version of the portal polygon: */
			Polygon flippedPortal;
			for(Polygon::const_reverse_iterator pIt=portal.rbegin();pIt!=portal.rend();++pIt)
				flippedPortal.push_back(*pIt);
			
			/* Create the portal from the front side node to the back side node: */
			Leaf::Portal portal0;
			portal0.otherLeafIndex=node.leafIndex;
			portal0.plane=plane;
			portal0.portal=po>Scalar(0)?flippedPortal:portal;
			leaves[portalNode.leafIndex].portals.push_back(portal0);
			
			/* Create the portal from the back side node to the front side node: */
			Leaf::Portal portal1;
			portal1.otherLeafIndex=portalNode.leafIndex;
			portal1.plane=Plane(-plane.getNormal(),-plane.getOffset());
			portal1.portal=po>Scalar(0)?portal:flippedPortal;
			leaves[node.leafIndex].portals.push_back(portal1);
			}
		}
	else
		{
		/* Check the portal polygon against the node's plane: */
		Scalar min,max;
		Polygon::const_iterator pIt=portal.begin();
		min=max=node.plane.calcDistance(*pIt);
		for(++pIt;pIt!=portal.end();++pIt)
			{
			Scalar d=node.plane.calcDistance(*pIt);
			if(min>d)
				min=d;
			if(max<d)
				max=d;
			}
		
		if(max<=Scalar(0))
			{
			/* Recurse into front child: */
			assignPortal(pass,plane,node.children[0],portalNode,portal);
			}
		else if(min>=Scalar(0))
			{
			/* Recurse into back child: */
			assignPortal(pass,plane,node.children[1],portalNode,portal);
			}
		else
			{
			/* Split the portal: */
			Polygon parts[2];
			splitPolygon(node.plane,portal,parts);
			
			/* Recurse into both children: */
			for(int i=0;i<2;++i)
				assignPortal(pass,plane,node.children[i],portalNode,parts[i]);
			}
		}
	}

void RenderBSPTree::createPortals(RenderBSPTree::Node& node)
	{
	if(node.children!=0)
		{
		/* Assign this node's portal polygons to its descendant leaves: */
		for(std::vector<Polygon>::const_iterator pIt=node.portals.begin();pIt!=node.portals.end();++pIt)
			assignPortal(0,node.plane,node.children[0],node,*pIt);
		
		/* Recurse into the node's children: */
		for(int i=0;i<2;++i)
			createPortals(node.children[i]);
		}
	}

void RenderBSPTree::splitTriangle(RenderBSPTree::Card triangleIndex,const RenderBSPTree::Point& v0,const RenderBSPTree::Point& v1,const RenderBSPTree::Point& v2,const RenderBSPTree::Plane& splitPlane,RenderBSPTree::TriangleFragmentList triangleFragments[2])
	{
	/* Create arrays to hold the new front and back vertices: */
	Point front[4];
	Point* fPtr=front;
	Point back[4];
	Point* bPtr=back;
	
	/* Calculate the vertices' plane distances: */
	Scalar d0=splitPlane.calcDistance(v0);
	Scalar d1=splitPlane.calcDistance(v1);
	Scalar d2=splitPlane.calcDistance(v2);
	
	/* Process the first edge: */
	if(d0<=Scalar(0))
		*(fPtr++)=v0;
	if(d0>=Scalar(0))
		*(bPtr++)=v0;
	if(d0*d1<Scalar(0))
		{
		/* Calculate the intersection point: */
		Point s=Geometry::affineCombination(v0,v1,d0/(d0-d1));
		*(fPtr++)=s;
		*(bPtr++)=s;
		}
	
	/* Process the second edge: */
	if(d1<=Scalar(0))
		*(fPtr++)=v1;
	if(d1>=Scalar(0))
		*(bPtr++)=v1;
	if(d1*d2<Scalar(0))
		{
		/* Calculate the intersection point: */
		Point s=Geometry::affineCombination(v1,v2,d1/(d1-d2));
		*(fPtr++)=s;
		*(bPtr++)=s;
		}
	
	/* Process the third edge: */
	if(d2<=Scalar(0))
		*(fPtr++)=v2;
	if(d2>=Scalar(0))
		*(bPtr++)=v2;
	if(d2*d0<Scalar(0))
		{
		/* Calculate the intersection point: */
		Point s=Geometry::affineCombination(v2,v0,d2/(d2-d0));
		*(fPtr++)=s;
		*(bPtr++)=s;
		}
	
	/* Assemble the front result triangle(s): */
	if(fPtr-front>=3)
		{
		TriangleFragment f0;
		f0.originalIndex=triangleIndex;
		for(int i=0;i<3;++i)
			f0.v[i]=front[i];
		triangleFragments[0].push_back(f0);
		}
	if(fPtr-front>=4)
		{
		TriangleFragment f1;
		f1.originalIndex=triangleIndex;
		for(int i=0;i<3;++i)
			f1.v[i]=front[(i+2)%4];
		triangleFragments[0].push_back(f1);
		}
	
	/* Assemble the back result triangle(s): */
	if(bPtr-back>=3)
		{
		TriangleFragment b0;
		b0.originalIndex=triangleIndex;
		for(int i=0;i<3;++i)
			b0.v[i]=back[i];
		triangleFragments[1].push_back(b0);
		}
	if(bPtr-back>=4)
		{
		TriangleFragment b1;
		b1.originalIndex=triangleIndex;
		for(int i=0;i<3;++i)
			b1.v[i]=back[(i+2)%4];
		triangleFragments[1].push_back(b1);
		}
	}

void RenderBSPTree::addNodeTriangles(RenderBSPTree::Node& node,const RenderBSPTree::CardList& triangleIndices,const RenderBSPTree::TriangleFragmentList& triangleFragments,Material* material)
	{
	if(node.children==0)
		{
		/* Get the node's leaf structure: */
		Leaf& leaf=leaves[node.leafIndex];
		
		/* Prepare a new submesh: */
		Leaf::SubMesh newSubMesh;
		newSubMesh.firstVertexIndex=0;
		newSubMesh.firstTriangleIndex=Card(leaf.triangleIndices.size());
		newSubMesh.numTriangles=0;
		newSubMesh.material=material;
		
		/* Store all complete triangles: */
		for(CardList::const_iterator tiIt=triangleIndices.begin();tiIt!=triangleIndices.end();++tiIt,++newSubMesh.numTriangles)
			leaf.triangleIndices.push_back(*tiIt);
		
		if(!triangleFragments.empty())
			{
			/* Store all distinct triangles fragments: */
			TriangleFragmentList::const_iterator tfIt=triangleFragments.begin();
			leaf.triangleIndices.push_back(tfIt->originalIndex);
			++newSubMesh.numTriangles;
			Card lastIndex=tfIt->originalIndex;
			for(++tfIt;tfIt!=triangleFragments.end();++tfIt)
				if(lastIndex!=tfIt->originalIndex)
					{
					leaf.triangleIndices.push_back(tfIt->originalIndex);
					++newSubMesh.numTriangles;
					lastIndex=tfIt->originalIndex;
					}
			}
		
		/* Add the new submesh to the node: */
		leaf.subMeshes.push_back(newSubMesh);
		}
	else
		{
		/* Distribute the given triangles and fragments between the node's children: */
		CardList subTriangleIndices[2];
		TriangleFragmentList subTriangleFragments[2];

		/* Process all complete triangles: */
		for(CardList::const_iterator tiIt=triangleIndices.begin();tiIt!=triangleIndices.end();++tiIt)
			{
			const Vertex* vPtr=&vertices[*tiIt];
			Scalar s,e;
			s=e=node.plane.calcDistance(vPtr[0].position);
			for(int i=1;i<3;++i)
				{
				Scalar p=node.plane.calcDistance(vPtr[i].position);
				if(s>p)
					s=p;
				else if(e<p)
					e=p;
				}
			
			if(s<Scalar(0)&&e>Scalar(0))
				{
				/* Split the triangle and assign the fragments to the subdomains: */
				splitTriangle(*tiIt,vPtr[0].position,vPtr[1].position,vPtr[2].position,node.plane,subTriangleFragments);
				}
			else
				{
				/* Assign the complete triangle to one subdomain: */
				if(e<=Scalar(0))
					subTriangleIndices[0].push_back(*tiIt);
				else
					subTriangleIndices[1].push_back(*tiIt);
				}
			}
		
		/* Process all triangle fragments: */
		for(TriangleFragmentList::const_iterator tfIt=triangleFragments.begin();tfIt!=triangleFragments.end();++tfIt)
			{
			Scalar s,e;
			s=e=node.plane.calcDistance(tfIt->v[0]);
			for(int i=1;i<3;++i)
				{
				Scalar p=node.plane.calcDistance(tfIt->v[i]);
				if(s>p)
					s=p;
				else if(e<p)
					e=p;
				}
			
			if(s<Scalar(0)&&e>Scalar(0))
				{
				/* Split the triangle fragment and assign the resulting fragments to the subdomains: */
				splitTriangle(tfIt->originalIndex,tfIt->v[0],tfIt->v[1],tfIt->v[2],node.plane,subTriangleFragments);
				}
			else
				{
				/* Assign the unchanged triangle fragment to one subdomain: */
				if(e<=Scalar(0))
					subTriangleFragments[0].push_back(*tfIt);
				else
					subTriangleFragments[1].push_back(*tfIt);
				}
			}
		
		/* Add the triangles and triangle fragments to the node's children: */
		for(int childIndex=0;childIndex<2;++childIndex)
			addNodeTriangles(node.children[childIndex],subTriangleIndices[childIndex],subTriangleFragments[childIndex],material);
		}
	}

void RenderBSPTree::finalizeNode(RenderBSPTree::Node& node)
	{
	if(node.children==0)
		{
		/* Get the node's leaf structure: */
		Leaf& leaf=leaves[node.leafIndex];
		
		/* Order submeshes by their material properties to minimize the number of material changes: */
		bool orderChanged=false;
		Card newNumSubMeshes=0;
		for(Leaf::SubMeshList::iterator smIt0=leaf.subMeshes.begin();smIt0!=leaf.subMeshes.end();++smIt0)
			{
			++newNumSubMeshes;
			
			/* Move all following submeshes with the same material to the front: */
			for(Leaf::SubMeshList::iterator smIt1=smIt0+1;smIt1!=leaf.subMeshes.end();++smIt1)
				{
				if(smIt1->material==smIt0->material)
					{
					/* Move the material to the front: */
					++smIt0;
					if(smIt0!=smIt1)
						{
						std::swap(*smIt0,*smIt1);
						orderChanged=true;
						}
					}
				}
			}
		
		if(orderChanged)
			{
			/* Adjust the triangle index array: */
			CardList newTriangleIndices;
			newTriangleIndices.reserve(leaf.triangleIndices.size());
			for(Leaf::SubMeshList::iterator smIt=leaf.subMeshes.begin();smIt!=leaf.subMeshes.end();++smIt)
				{
				/* Copy the submesh's triangle indices to a new list: */
				Card newFirstTriangleIndex=newTriangleIndices.size();
				for(Card i=0;i<smIt->numTriangles;++i)
					newTriangleIndices.push_back(leaf.triangleIndices[smIt->firstTriangleIndex+i]);
				
				/* Adjust the submesh's first triangle index: */
				smIt->firstTriangleIndex=newFirstTriangleIndex;
				}
			std::swap(leaf.triangleIndices,newTriangleIndices);
			
			/* Collapse submeshes with identical materials: */
			Leaf::SubMeshList newSubMeshes;
			newSubMeshes.reserve(newNumSubMeshes);
			Leaf::SubMeshList::iterator smIt=leaf.subMeshes.begin();
			newSubMeshes.push_back(*smIt);
			for(++smIt;smIt!=leaf.subMeshes.end();++smIt)
				if(smIt->material==newSubMeshes.back().material&&smIt->firstTriangleIndex==newSubMeshes.back().firstTriangleIndex+newSubMeshes.back().numTriangles)
					{
					/* Add the submesh's triangles to the current submesh: */
					newSubMeshes.back().numTriangles+=smIt->numTriangles;
					}
				else
					{
					/* Start a new submesh: */
					newSubMeshes.push_back(*smIt);
					}
			std::swap(leaf.subMeshes,newSubMeshes);
			}
		
		/* Set the starting vertex indices of all submeshes: */
		for(Leaf::SubMeshList::iterator smIt=leaf.subMeshes.begin();smIt!=leaf.subMeshes.end();++smIt)
			{
			smIt->firstVertexIndex=totalNumVertices;
			totalNumVertices+=smIt->numTriangles*3;
			}
		
		// DEBUGGING
		std::cout<<"Leaf node contains "<<leaf.triangleIndices.size()<<" triangles in "<<leaf.subMeshes.size()<<" submeshes"<<std::endl;
		}
	else
		{
		/* Recurse into the child nodes: */
		for(int i=0;i<2;++i)
			finalizeNode(node.children[i]);
		}
	}

void RenderBSPTree::uploadNodeTriangles(const RenderBSPTree::Node& node,RenderBSPTree::Scalar* vertexBuffer) const
	{
	if(node.children==0)
		{
		/* Get the node's leaf structure: */
		const Leaf& leaf=leaves[node.leafIndex];
		
		/* Upload all triangles in this leaf node: */
		CardList::const_iterator tiIt=leaf.triangleIndices.begin();
		for(Leaf::SubMeshList::const_iterator smIt=leaf.subMeshes.begin();smIt!=leaf.subMeshes.end();++smIt)
			{
			Scalar* vbPtr=vertexBuffer+smIt->firstVertexIndex*(3+3);
			for(Card triangleIndex=0;triangleIndex<smIt->numTriangles;++triangleIndex,++tiIt)
				{
				const Vertex* vPtr=&vertices[*tiIt];
				for(int i=0;i<3;++i,++vPtr)
					{
					for(int j=0;j<3;++j)
						vbPtr[j]=vPtr->normal[j];
					vbPtr+=3;
					for(int j=0;j<3;++j)
						vbPtr[j]=vPtr->position[j];
					vbPtr+=3;
					}
				}
			}
		}
	else
		{
		/* Recurse into the children: */
		for(int i=0;i<2;++i)
			uploadNodeTriangles(node.children[i],vertexBuffer);
		}
	}

void RenderBSPTree::renderLeaf(RenderBSPTree::Card leafIndex,const RenderBSPTree::Point& traversalStart,const RenderBSPTree::PTransform& pmv,const RenderBSPTree::ScreenBox& viewport,bool renderedLeaves[],GLContextData& contextData,Material*& currentMaterial) const
	{
	/* Get the leaf structure: */
	const Leaf& leaf=leaves[leafIndex];
	
	if(!renderedLeaves[leafIndex])
		{
		// DEBUGGING
		++numRenderedNodes;
		
		/* Render the leaf's triangles: */
		for(Leaf::SubMeshList::const_iterator smIt=leaf.subMeshes.begin();smIt!=leaf.subMeshes.end();++smIt)
			{
			/* Install the submesh's material: */
			if(smIt->material!=currentMaterial)
				{
				if(currentMaterial!=0)
					currentMaterial->reset(contextData);
				currentMaterial=smIt->material.getPointer();
				if(currentMaterial!=0)
					currentMaterial->set(contextData);
				}
			
			/* Render the submesh's triangles: */
			glDrawArrays(GL_TRIANGLES,smIt->firstVertexIndex,smIt->numTriangles*3);
			
			// DEBUGGING
			numRenderedTriangles+=smIt->numTriangles;
			}
		
		renderedLeaves[leafIndex]=true;
		}
	
	/* Clip the leaf's portals against the viewport: */
	for(std::vector<Leaf::Portal>::const_iterator pIt=leaf.portals.begin();pIt!=leaf.portals.end();++pIt)
		{
		/* Check if the portal points away from the traversal starting point: */
		if(pIt->plane.calcDistance(traversalStart)<Scalar(0))
			{
			/* Calculate the portal's viewport: */
			ScreenBox portalViewport=projectPortal(pIt->portal,pmv,viewport);
			if(!portalViewport.isNull())
				{
				// DEBUGGING
				#if 0
				
				/* Render the portal's viewport: */
				GLint vp[4];
				glGetIntegerv(GL_VIEWPORT,vp);
				glDisable(GL_LIGHTING);
				glLineWidth(3.0f);
				glColor3f(1.0f,0.0f,0.0f);
				
				glPushMatrix();
				glLoadIdentity();
				glMatrixMode(GL_PROJECTION);
				glPushMatrix();
				glLoadIdentity();
				glBegin(GL_LINE_LOOP);
				glVertex3f(portalViewport.min[0],portalViewport.min[1],-1.0f);
				glVertex3f(portalViewport.max[0],portalViewport.min[1],-1.0f);
				glVertex3f(portalViewport.max[0],portalViewport.max[1],-1.0f);
				glVertex3f(portalViewport.min[0],portalViewport.max[1],-1.0f);
				glEnd();
				glPopMatrix();
				glMatrixMode(GL_MODELVIEW);
				glPopMatrix();
				
				glLineWidth(1.0f);
				glEnable(GL_LIGHTING);
				
				#endif
				
				/* Render the leaf on the other side of the portal: */
				renderLeaf(pIt->otherLeafIndex,traversalStart,pmv,portalViewport,renderedLeaves,contextData,currentMaterial);
				
				// DEBUGGING
				
				#if 0
				
				/* Render the portal: */
				glPushAttrib(GL_COLOR_BUFFER_BIT|GL_ENABLE_BIT);
				glDisable(GL_LIGHTING);
				glColor4f(0.75f,0.25f,0.75f,0.5f);
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
				glBegin(GL_POLYGON);
				for(Polygon::const_iterator vIt=pIt->portal.begin();vIt!=pIt->portal.end();++vIt)
					glVertex(*vIt);
				glEnd();
				glPopAttrib();
				
				#endif
				}
			}
		}
	}

RenderBSPTree::RenderBSPTree(const RenderBSPTree::VertexList& sVertices)
	:vertices(sVertices)
	{
	}

void RenderBSPTree::initContext(GLContextData& contextData) const
	{
	/* Create a new context data item: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	
	if(dataItem->vertexBufferId!=0)
		{
		/* Prepare the vertex buffer object: */
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,dataItem->vertexBufferId);
		size_t vertexSize=sizeof(Scalar)*(3+3);
		glBufferDataARB(GL_ARRAY_BUFFER_ARB,totalNumVertices*vertexSize,0,GL_STATIC_DRAW_ARB);
		Scalar* bufferPtr=static_cast<Scalar*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
		
		/* Upload all nodes' triangles into the vertex buffer object: */
		uploadNodeTriangles(root,bufferPtr);
		
		/* Finalize and protect the vertex buffer object: */
		glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
		}
	}

void RenderBSPTree::loadTree(const char* bspTreeFileName)
	{
	/* Open the BSP tree file: */
	Misc::File file(bspTreeFileName,"rb",Misc::File::LittleEndian);
	
	/* Read the BSP tree file header: */
	static const char* bspTreeFileHeader="BSP Tree File V1.0";
	char header[32];
	file.read(header,strlen(bspTreeFileHeader)+1);
	if(strcmp(bspTreeFileHeader,header)!=0)
		Misc::throwStdErr("RenderBSPTree::loadTree: File %s is not a BSP tree file",bspTreeFileName);
	
	/* Destroy the current BSP tree: */
	if(root.children!=0)
		{
		delete[] root.children;
		root.children=0;
		root.portals.clear();
		}
	leaves.clear();
	
	// DEBUGGING
	numAddedTriangles=0;
	
	/* Check if the file contains a root node: */
	if(file.read<unsigned char>()!=0)
		{
		/* Load the BSP tree's root node: */
		loadNode(root,file);
		
		/* Create the portal polygons connecting all leaf nodes: */
		createPortals(root);
		}
	
	// DEBUGGING
	size_t totalNumPortals=0;
	for(std::vector<Leaf>::const_iterator lIt=leaves.begin();lIt!=leaves.end();++lIt)
		totalNumPortals+=lIt->portals.size();
	std::cout<<"BSP tree contains "<<leaves.size()<<" leaves and "<<totalNumPortals<<" portal polygons"<<std::endl;
	}

void RenderBSPTree::addTriangles(const RenderBSPTree::CardList& triangleIndices,Material* material)
	{
	// DEBUGGING
	numAddedTriangles+=triangleIndices.size();
	
	/* Create an empty triangle fragment list: */
	TriangleFragmentList triangleFragments;
	
	/* Add the given triangles to the root node: */
	addNodeTriangles(root,triangleIndices,triangleFragments,material);
	}

void RenderBSPTree::finalizeTree(void)
	{
	/* Reset the total number of vertices: */
	totalNumVertices=0;
	
	/* Finalize the root node: */
	finalizeNode(root);
	
	// DEBUGGING
	std::cout<<"Finalized BSP tree contains "<<numAddedTriangles<<" original triangles and "<<totalNumVertices/3<<" split triangles"<<std::endl;
	}

void RenderBSPTree::glRenderAction(GLContextData& contextData) const
	{
	/* Get the context data item: */
	DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
	
	/* Install the vertex arrays: */
	GLVertexArrayParts::enable(GLVertexArrayParts::Position|GLVertexArrayParts::Normal);
	if(dataItem->vertexBufferId!=0)
		{
		/* Bind the vertex buffer object: */
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,dataItem->vertexBufferId);
		
		/* Render from the vertex buffer object: */
		const Scalar* vertexPointer=0;
		size_t vertexSize=sizeof(Scalar)*(3+3);
		glNormalPointer(vertexSize,vertexPointer+0);
		glVertexPointer(3,vertexSize,vertexPointer+3);
		}
	
	/* Query the projection and modelview matrices from OpenGL: */
	PTransform pmv=glGetProjectionMatrix<Scalar>();
	pmv*=glGetModelviewMatrix<Scalar>();
	
	/* Calculate the center point of the front plane: */
	Point traversalStart=pmv.inverseTransform(PTransform::HVector(0,0,-1,1)).toPoint();
	
	/* Find the BSP tree leaf containing the traversal start point: */
	const Node* startNode=&root;
	while(startNode->children!=0)
		startNode=&startNode->children[startNode->plane.calcDistance(traversalStart)>=Scalar(0)?1:0];
	
	/* Create the initial viewport: */
	ScreenBox viewport(ScreenBox::Point(-1,-1),ScreenBox::Point(1,1));
	
	// DEBUGGING
	//glPushAttrib(GL_LINE_BIT|GL_POLYGON_BIT);
	//glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
	//glLineWidth(1.0f);
	
	// DEBUGGING
	numRenderedNodes=0;
	numRenderedTriangles=0;
	
	/* Keep track of the current material to minimize OpenGL state changes: */
	Material* currentMaterial=0;
	
	/* Create a map of already rendered leaves: */
	unsigned int numLeaves=leaves.size();
	bool* renderedLeaves=new bool[numLeaves];
	for(unsigned int i=0;i<numLeaves;++i)
		renderedLeaves[i]=false;
	
	/* Render the traversal start node: */
	renderLeaf(startNode->leafIndex,traversalStart,pmv,viewport,renderedLeaves,contextData,currentMaterial);
	
	delete[] renderedLeaves;
	
	/* Uninstall the last used material: */
	if(currentMaterial!=0)
		currentMaterial->reset(contextData);
	
	// DEBUGGING
	//std::cout<<"Rendered "<<numRenderedNodes<<" nodes, "<<numRenderedTriangles<<" triangles"<<std::endl;
	
	// DEBUGGING
	//glPopAttrib();
	
	/* Uninstall the vertex arrays: */
	if(dataItem->vertexBufferId!=0)
		{
		/* Unbind the vertex buffer object: */
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
		}
	GLVertexArrayParts::disable(GLVertexArrayParts::Position|GLVertexArrayParts::Normal);
	}
