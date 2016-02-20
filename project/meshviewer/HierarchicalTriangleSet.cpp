/***********************************************************************
HierarchicalTriangleSet - Class to represent 3D objects as a tree of
sets of triangles, for efficient intersection tests and rendering using
OpenGL.
Copyright (c) 2009-2012 Oliver Kreylos
***********************************************************************/

#define HIERARCHICALTRIANGLESET_IMPLEMENTATION

#include "HierarchicalTriangleSet.h"

// DEBUGGING
#include <iostream>
#include <Misc/ThrowStdErr.h>
#include <Misc/File.h>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <GL/gl.h>
#include <GL/GLVertexArrayParts.h>
#include <GL/GLVertexArrayTemplates.h>
#include <GL/GLContextData.h>
#include <GL/GLExtensionManager.h>
#include <GL/Extensions/GLARBVertexBufferObject.h>
#include <GL/GLGeometryWrappers.h>

// DEBUGGING
#include "PhongMaterial.h"

/**************************************************
Methods of class HierarchicalTriangleSet::DataItem:
**************************************************/

template <class MeshVertexParam>
inline
HierarchicalTriangleSet<MeshVertexParam>::DataItem::DataItem(
	void)
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

template <class MeshVertexParam>
inline
HierarchicalTriangleSet<MeshVertexParam>::DataItem::~DataItem(
	void)
	{
	if(vertexBufferId!=0)
		{
		/* Delete the vertex buffer object: */
		glDeleteBuffersARB(1,&vertexBufferId);
		}
	}

/****************************************
Methods of class HierarchicalTriangleSet:
****************************************/

template <class MeshVertexParam>
inline
bool
HierarchicalTriangleSet<MeshVertexParam>::limitRay(
	typename HierarchicalTriangleSet<MeshVertexParam>::Point& p0,
	typename HierarchicalTriangleSet<MeshVertexParam>::Point& p1) const
	{
	const MBox& bb=subMeshes[0].boundingBox;
	Scalar lambda0=Scalar(0);
	Scalar lambda1=Scalar(1);
	for(int dim=0;dim<3;++dim)
		{
		if(p0[dim]<bb.min[dim])
			{
			if(p1[dim]>=bb.min[dim])
				{
				Scalar lambda=(Scalar(bb.min[dim])-p0[dim])/(p1[dim]-p0[dim]);
				if(lambda0<lambda)
					lambda0=lambda;
				}
			else
				lambda0=Scalar(1);
			}
		else if(p0[dim]>bb.max[dim])
			{
			if(p1[dim]<=bb.max[dim])
				{
				Scalar lambda=(Scalar(bb.max[dim])-p0[dim])/(p1[dim]-p0[dim]);
				if(lambda0<lambda)
					lambda0=lambda;
				}
			else
				lambda0=Scalar(1);
			}
		if(p1[dim]<bb.min[dim])
			{
			if(p0[dim]>=bb.min[dim])
				{
				Scalar lambda=(Scalar(bb.min[dim])-p0[dim])/(p1[dim]-p0[dim]);
				if(lambda1>lambda)
					lambda1=lambda;
				}
			else
				lambda1=Scalar(0);
			}
		else if(p1[dim]>bb.max[dim])
			{
			if(p0[dim]<=bb.max[dim])
				{
				Scalar lambda=(Scalar(bb.max[dim])-p0[dim])/(p1[dim]-p0[dim]);
				if(lambda1>lambda)
					lambda1=lambda;
				}
			else
				lambda1=Scalar(1);
			}
		}
	
	/* Check if the ray missed the bounding box: */
	if(lambda0>lambda1)
		return false;
	
	if(lambda0>Scalar(0)||lambda1<Scalar(1))
		{
		/* Update the ray's end points: */
		Point cp0=lambda1>Scalar(0)?Geometry::affineCombination(p0,p1,lambda0):p0;
		Point cp1=lambda1<Scalar(1)?Geometry::affineCombination(p0,p1,lambda1):p1;
		p0=cp0;
		p1=cp1;
		}
	
	return true;
	}

template <class MeshVertexParam>
inline
typename HierarchicalTriangleSet<MeshVertexParam>::MScalar
HierarchicalTriangleSet<MeshVertexParam>::intersectSubMesh(
	typename HierarchicalTriangleSet<MeshVertexParam>::Card subMeshIndex,
	const typename HierarchicalTriangleSet<MeshVertexParam>::MRay& ray,
	typename HierarchicalTriangleSet<MeshVertexParam>::MScalar lambdaMin,
	typename HierarchicalTriangleSet<MeshVertexParam>::MScalar lambdaMax) const
	{
	const SubMesh& sm=subMeshes[subMeshIndex];
	
	/* Check the ray against the submesh's bounding box: */
	std::pair<MScalar,MScalar> boxLambdas=sm.boundingBox.getRayParameters(ray);
	boxLambdas.first-=MScalar(1.0e-3);
	if(boxLambdas.first<lambdaMin)
		boxLambdas.first=lambdaMin;
	boxLambdas.second+=MScalar(1.0e-3);
	if(boxLambdas.second>lambdaMax)
		boxLambdas.second=lambdaMax;
	if(boxLambdas.first>=boxLambdas.second)
		return lambdaMax;
	
	/* Intersect the ray interval with all triangles in the submesh: */
	const MeshVertex* vPtr=&vertices[sm.firstTriangleVertexIndex];
	for(Card triangleIndex=0;triangleIndex<sm.numTriangles;++triangleIndex,vPtr+=3)
		{
		/* Intersect the ray with the triangle's plane: */
		MVector normal=Geometry::cross(vPtr[1].position-vPtr[0].position,vPtr[2].position-vPtr[0].position);
		MScalar lambdaD=ray.getDirection()*normal;
		if(lambdaD!=MScalar(0))
			{
			MScalar lambda=(vPtr[0].position-ray.getOrigin())*normal/lambdaD;
			if(lambda>=boxLambdas.first&&lambda<boxLambdas.second)
				{
				/* Check whether the intersection point lies within the triangle: */
				MPoint intersection=ray(lambda);
				bool inside=true;
				for(int i=0;i<3&&inside;++i)
					{
					MVector edgeNormal=Geometry::cross(normal,vPtr[(i+1)%3].position-vPtr[i].position);
					inside=(intersection-vPtr[i].position)*edgeNormal>=MScalar(0);
					}
				if(inside)
					{
					/* Update the minimum intercept: */
					boxLambdas.second=lambda;
					lambdaMax=lambda;
					
					// DEBUGGING
					/* Remember the last intersected submesh: */
					lastIntersected=const_cast<SubMesh*>(&sm);
					}
				}
			}
		}
	
	// DEBUGGING
	++numTestedSubMeshes;
	
	/* Intersect the ray interval with all children of this submesh: */
	for(CardList::const_iterator ciIt=sm.childIndices.begin();ciIt!=sm.childIndices.end();++ciIt)
		{
		MScalar lambda=intersectSubMesh(*ciIt,ray,boxLambdas.first,boxLambdas.second);
		if(boxLambdas.second>lambda)
			{
			boxLambdas.second=lambda;
			lambdaMax=lambda;
			}
		}
	
	return lambdaMax;
	}

template <class MeshVertexParam>
inline
HierarchicalTriangleSet<MeshVertexParam>::HierarchicalTriangleSet(
	void)
	:triangleKdTree(vertices),
	 bspTree(0)
	{
	/* Create the triangle set's root submesh node: */
	SubMesh root;
	root.parentIndex=0;
	root.name="Root";
	root.numTriangles=0;
	root.firstTriangleVertexIndex=0;
	root.boundingBox=MBox::empty;
	subMeshes.push_back(root);
	
	/* Initialize the current sub mesh: */
	currentSubMesh.parentIndex=0;
	currentSubMesh.name="";
	currentSubMesh.numTriangles=0;
	currentSubMesh.firstTriangleVertexIndex=0;
	
	// DEBUGGING
	lastIntersected=0;
	
	/* Create the indicator material: */
	GLMaterial m;
	m.ambient=GLMaterial::Color(1.0f,0.0f,0.0f);
	m.diffuse=GLMaterial::Color(1.0f,0.0f,0.0f);
	m.specular=GLMaterial::Color(1.0f,1.0f,1.0f);
	m.shininess=25.0f;
	intersectedMaterial=new PhongMaterial(m);
	}

template <class MeshVertexParam>
inline
HierarchicalTriangleSet<MeshVertexParam>::~HierarchicalTriangleSet(
	void)
	{
	delete bspTree;
	}

template <class MeshVertexParam>
inline
PolygonModel::Box
HierarchicalTriangleSet<MeshVertexParam>::calcBoundingBox(
	void) const
	{
	/* Return the root submesh node's bounding box: */
	return subMeshes[0].boundingBox;
	}

template <class MeshVertexParam>
inline
void
HierarchicalTriangleSet<MeshVertexParam>::glRenderAction(
	GLContextData& contextData) const
	{
	if(bspTree!=0)
		{
		/* Render the BSP tree: */
		bspTree->glRenderAction(contextData);
		}
	else
		{
		/* Get the context data item: */
		DataItem* dataItem=contextData.template retrieveDataItem<DataItem>(this);
		
		/* Install the vertex arrays: */
		GLVertexArrayParts::enable(GLVertexArrayParts::Position|GLVertexArrayParts::Normal|GLVertexArrayParts::TexCoord);
		if(dataItem->vertexBufferId!=0)
			{
			/* Bind the vertex buffer object: */
			glBindBufferARB(GL_ARRAY_BUFFER_ARB,dataItem->vertexBufferId);
			
			/* Render from the vertex buffer object: */
			const MScalar* vertexPointer=0;
			size_t vertexSize=sizeof(MScalar)*(2+3+3);
			glTexCoordPointer(2,vertexSize,vertexPointer+0);
			glNormalPointer(vertexSize,vertexPointer+2);
			glVertexPointer(3,vertexSize,vertexPointer+5);
			}
		else
			{
			/* Render directly from application memory: */
			const MeshVertex* vertexPointer=&vertices.front();
			glTexCoordPointer(2,sizeof(MeshVertex),vertexPointer[0].texCoord.getComponents());
			glNormalPointer(sizeof(MeshVertex),vertexPointer[0].normal.getComponents());
			glVertexPointer(3,sizeof(MeshVertex),vertexPointer[0].position.getComponents());
			}
		
		/* Render the triangle set: */
		Material* currentMaterial=0;
		for(typename std::vector<SubMesh>::const_iterator smIt=subMeshes.begin();smIt!=subMeshes.end();++smIt)
			{
			if(smIt->numTriangles!=0)
				{
				/* Install the sub mesh's material properties: */
				if(currentMaterial!=smIt->material.getPointer())
					{
					if(currentMaterial!=0)
						currentMaterial->reset(contextData);
					currentMaterial=smIt->material.getPointer();
					if(currentMaterial!=0)
						currentMaterial->set(contextData);
					}
				
				/* Draw the sub mesh's triangles: */
				glDrawArrays(GL_TRIANGLES,smIt->firstTriangleVertexIndex,smIt->numTriangles*3);
				}
			}
		if(currentMaterial!=0)
			currentMaterial->reset(contextData);
		
		/* Uninstall the vertex arrays: */
		if(dataItem->vertexBufferId!=0)
			{
			/* Unbind the vertex buffer object: */
			glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
			}
		GLVertexArrayParts::disable(GLVertexArrayParts::Position|GLVertexArrayParts::Normal|GLVertexArrayParts::TexCoord);
		}
	
	#if 0
	
	/* Visualize the most recent intersection test: */
	glPushAttrib(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_ENABLE_BIT|GL_LIGHTING_BIT|GL_LINE_BIT);
	
	glDisable(GL_CULL_FACE);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_TRUE);
	glMaterial(GLMaterialEnums::FRONT_AND_BACK,GLMaterial::Color(0.5f,0.5f,1.0f));
	triangleKdTree.drawIntersection(lastP0,lastP1,true);
	
	glDisable(GL_LIGHTING);
	glLineWidth(3.0f);
	glBegin(GL_LINES);
	glColor3f(1.0f,0.0f,0.0f);
	glVertex(lastP0);
	glVertex(lastP1);
	glEnd();
	
	glEnable(GL_CULL_FACE);
	glEnable(GL_LIGHTING);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);
	glMaterial(GLMaterialEnums::FRONT,GLMaterial::Color(0.5f,1.0f,0.5f,0.25f));
	triangleKdTree.drawIntersection(lastP0,lastP1,false);
	
	glPopAttrib();
	
	#endif
	}

template <class MeshVertexParam>
inline
typename HierarchicalTriangleSet<MeshVertexParam>::Point
HierarchicalTriangleSet<MeshVertexParam>::intersect(
	const typename HierarchicalTriangleSet<MeshVertexParam>::Point& p0,
	const typename HierarchicalTriangleSet<MeshVertexParam>::Point& p1) const
	{
	// DEBUGGING
	/* Reset the indicator: */
	if(lastIntersected!=0)
		{
		lastIntersected->material=lastIntersectedMaterial;
		lastIntersected=0;
		}
	
	/* Restrict the search ray to the model's bounding box: */
	Point cp0=p0;
	Point cp1=p1;
	if(!limitRay(cp0,cp1))
		return p1;
	
	// DEBUGGING
	numTestedSubMeshes=0;
	
	/* Intersect the ray interval with the kd-tree: */
	TriangleKdTree::Point kdp0=cp0;
	TriangleKdTree::Point kdp1=cp1;
	TriangleKdTree::IntersectResult kdResult=triangleKdTree.intersect(kdp0,kdp1);
	Point result;
	if(kdResult.triangleIndex==TriangleKdTree::nil)
		result=p1;
	else
		result=kdResult.intersection;
	
	// DEBUGGING
	// std::cout<<"Tested "<<numTestedSubMeshes<<" submeshes"<<std::endl;
	lastP0=cp0;
	if(result!=p1)
		{
		// std::cout<<"Have intersection at "<<result[0]<<", "<<result[1]<<", "<<result[2]<<std::endl;
		lastP1=result;
		}
	else
		lastP1=cp1;
	
	// DEBUGGING
	/* Change the last intersected submesh's material: */
	if(lastIntersected!=0)
		{
		lastIntersectedMaterial=lastIntersected->material;
		lastIntersected->material=intersectedMaterial;
		}
	
	return result;
	}

template <class MeshVertexParam>
inline
typename HierarchicalTriangleSet<MeshVertexParam>::Scalar
HierarchicalTriangleSet<MeshVertexParam>::traceBox(
	const typename HierarchicalTriangleSet<MeshVertexParam>::Box& box,
	const typename HierarchicalTriangleSet<MeshVertexParam>::Vector& displacement,
	typename HierarchicalTriangleSet<MeshVertexParam>::Vector& hitNormal) const
	{
	/* Trace the box through the kd-tree: */
	TriangleKdTree::Vector myHitNormal;
	Scalar result=triangleKdTree.traceBox(box,displacement,myHitNormal);
	if(result<Scalar(1))
		hitNormal=myHitNormal;
	return result;
	}

template <class MeshVertexParam>
inline
void
HierarchicalTriangleSet<MeshVertexParam>::loadBSPTree(
	const char* bspTreeFileName)
	{
	/* Load the BSP tree from the given file: */
	delete bspTree;
	bspTree=new RenderBSPTree(vertices);
	bspTree->loadTree(bspTreeFileName);
	
	/* Add all submeshes to the BSP tree: */
	for(typename std::vector<SubMesh>::const_iterator smIt=subMeshes.begin();smIt!=subMeshes.end();++smIt)
		{
		/* Create a list of triangle indices in this submesh: */
		RenderBSPTree::CardList triangleIndices;
		Card vertexIndex=smIt->firstTriangleVertexIndex;
		for(Card i=0;i<smIt->numTriangles;++i,vertexIndex+=3)
			triangleIndices.push_back(vertexIndex);
		
		/* Add the submesh's triangle to the BSP tree: */
		bspTree->addTriangles(triangleIndices,smIt->material.getPointer());
		}
	
	/* Finalize the BSP tree: */
	bspTree->finalizeTree();
	}

template <class MeshVertexParam>
inline
void
HierarchicalTriangleSet<MeshVertexParam>::drawSubMesh(
	const HierarchicalTriangleSetBase::SubMesh& mesh,
	GLContextData& contextData) const
	{
	const SubMesh* myMesh=dynamic_cast<const SubMesh*>(&mesh);
	if(myMesh!=0)
		{
		/* Draw the mesh's bounding box: */
		glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
		glDisable(GL_LIGHTING);
		glLineWidth(3.0f);
		glColor3f(0.0f,1.0f,0.0f);
		
		glBegin(GL_LINE_STRIP);
		glVertex(myMesh->boundingBox.getVertex(0));
		glVertex(myMesh->boundingBox.getVertex(1));
		glVertex(myMesh->boundingBox.getVertex(3));
		glVertex(myMesh->boundingBox.getVertex(2));
		glVertex(myMesh->boundingBox.getVertex(0));
		glVertex(myMesh->boundingBox.getVertex(4));
		glVertex(myMesh->boundingBox.getVertex(5));
		glVertex(myMesh->boundingBox.getVertex(7));
		glVertex(myMesh->boundingBox.getVertex(6));
		glVertex(myMesh->boundingBox.getVertex(4));
		glEnd();
		glBegin(GL_LINES);
		glVertex(myMesh->boundingBox.getVertex(1));
		glVertex(myMesh->boundingBox.getVertex(5));
		glVertex(myMesh->boundingBox.getVertex(3));
		glVertex(myMesh->boundingBox.getVertex(7));
		glVertex(myMesh->boundingBox.getVertex(2));
		glVertex(myMesh->boundingBox.getVertex(6));
		glEnd();
		
		glPopAttrib();
		}
	
	if(myMesh!=0&&myMesh->numTriangles!=0)
		{
		/* Get the context data item: */
		DataItem* dataItem=contextData.template retrieveDataItem<DataItem>(this);
		
		/* Install the vertex arrays: */
		GLVertexArrayParts::enable(GLVertexArrayParts::Position|GLVertexArrayParts::Normal|GLVertexArrayParts::TexCoord);
		if(dataItem->vertexBufferId!=0)
			{
			/* Bind the vertex buffer object: */
			glBindBufferARB(GL_ARRAY_BUFFER_ARB,dataItem->vertexBufferId);
			
			/* Render from the vertex buffer object: */
			const MScalar* vertexPointer=0;
			size_t vertexSize=sizeof(MScalar)*(2+3+3);
			glTexCoordPointer(2,vertexSize,vertexPointer+0);
			glNormalPointer(vertexSize,vertexPointer+2);
			glVertexPointer(3,vertexSize,vertexPointer+5);
			}
		else
			{
			/* Render directly from application memory: */
			const MeshVertex* vertexPointer=&vertices.front();
			glTexCoordPointer(2,sizeof(MeshVertex),vertexPointer[0].texCoord.getComponents());
			glNormalPointer(sizeof(MeshVertex),vertexPointer[0].normal.getComponents());
			glVertexPointer(3,sizeof(MeshVertex),vertexPointer[0].position.getComponents());
			}
		
		/* Install the sub mesh's material properties: */
		// myMesh->material->set(contextData);
		
		/* Draw the sub mesh's triangles: */
		glDrawArrays(GL_TRIANGLES,myMesh->firstTriangleVertexIndex,myMesh->numTriangles*3);
		
		/* Uninstall the mesh's material properties: */
		// myMesh->material->reset(contextData);
		
		/* Uninstall the vertex arrays: */
		if(dataItem->vertexBufferId!=0)
			{
			/* Unbind the vertex buffer object: */
			glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
			}
		GLVertexArrayParts::disable(GLVertexArrayParts::Position|GLVertexArrayParts::Normal|GLVertexArrayParts::TexCoord);
		}
	}

template <class MeshVertexParam>
inline
void
HierarchicalTriangleSet<MeshVertexParam>::initContext(
	GLContextData& contextData) const
	{
	/* Create a new context data item: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	
	if(dataItem->vertexBufferId!=0)
		{
		/* Upload the triangle set into the vertex buffer object: */
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,dataItem->vertexBufferId);
		size_t vertexSize=sizeof(MScalar)*(2+3+3);
		glBufferDataARB(GL_ARRAY_BUFFER_ARB,vertices.size()*vertexSize,0,GL_STATIC_DRAW_ARB);
		MScalar* bufferPtr=static_cast<MScalar*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
		
		for(typename std::vector<MeshVertex>::const_iterator vIt=vertices.begin();vIt!=vertices.end();++vIt)
			{
			for(int i=0;i<2;++i)
				*(bufferPtr++)=vIt->texCoord[i];
			for(int i=0;i<3;++i)
				*(bufferPtr++)=vIt->normal[i];
			for(int i=0;i<3;++i)
				*(bufferPtr++)=vIt->position[i];
			}
		
		glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
		}
	}

template <class MeshVertexParam>
inline
typename HierarchicalTriangleSet<MeshVertexParam>::Card
HierarchicalTriangleSet<MeshVertexParam>::addVertex(
	const typename HierarchicalTriangleSet<MeshVertexParam>::MeshVertex& newVertex)
	{
	/* Get the new vertex's index: */
	Card result=vertices.size();
	
	/* Add the vertex: */
	vertices.push_back(newVertex);
	
	return result;
	}

template <class MeshVertexParam>
inline
void
HierarchicalTriangleSet<MeshVertexParam>::setSubMeshParentIndex(
	typename HierarchicalTriangleSet<MeshVertexParam>::Card parentIndex)
	{
	currentSubMesh.parentIndex=parentIndex;
	}

template <class MeshVertexParam>
inline
void
HierarchicalTriangleSet<MeshVertexParam>::setSubMeshName(
	const std::string& name)
	{
	currentSubMesh.name=name;
	}

template <class MeshVertexParam>
inline
void
HierarchicalTriangleSet<MeshVertexParam>::setSubMeshMaterial(
	MaterialPointer material)
	{
	currentSubMesh.material=material;
	}

template <class MeshVertexParam>
inline
typename HierarchicalTriangleSet<MeshVertexParam>::Card
HierarchicalTriangleSet<MeshVertexParam>::finishSubMesh(
	void)
	{
	/* Finalize the sub mesh: */
	Card nextFirstTriangleVertexIndex=vertices.size();
	currentSubMesh.numTriangles=(nextFirstTriangleVertexIndex-currentSubMesh.firstTriangleVertexIndex)/3;
	
	/* Calculate the sub mesh's bounding box: */
	currentSubMesh.boundingBox=MBox::empty;
	const MeshVertex* vPtr=&vertices[currentSubMesh.firstTriangleVertexIndex];
	for(Card triangleIndex=0;triangleIndex<currentSubMesh.numTriangles;++triangleIndex)
		for(int i=0;i<3;++i,++vPtr)
			currentSubMesh.boundingBox.addPoint(vPtr->position);
	
	/* Store the sub mesh: */
	Card subMeshIndex=subMeshes.size();
	subMeshes.push_back(currentSubMesh);
	
	/* Add the submesh to its parent: */
	subMeshes[currentSubMesh.parentIndex].childIndices.push_back(subMeshIndex);
	
	/* Update all parent submeshes' bounding boxes: */
	Card nodeIndex=subMeshIndex;
	while(nodeIndex!=0)
		{
		Card parentIndex=subMeshes[nodeIndex].parentIndex;
		subMeshes[parentIndex].boundingBox.addBox(subMeshes[nodeIndex].boundingBox);
		nodeIndex=parentIndex;
		}
	
	/* Prepare the next sub mesh: */
	currentSubMesh.parentIndex=0;
	currentSubMesh.name="";
	currentSubMesh.material=0;
	currentSubMesh.numTriangles=0;
	currentSubMesh.firstTriangleVertexIndex=nextFirstTriangleVertexIndex;
	
	return subMeshIndex;
	}

template <class MeshVertexParam>
inline
void
HierarchicalTriangleSet<MeshVertexParam>::sortSubMeshes(
	void)
	{
	std::vector<SubMesh> newSubMeshes;
	std::vector<Card> newSubMeshIndices;
	size_t numSubMeshes=subMeshes.size();
	newSubMeshes.reserve(numSubMeshes);
	newSubMeshIndices.reserve(numSubMeshes);
	bool* used=new bool[numSubMeshes];
	for(size_t i=0;i<numSubMeshes;++i)
		used[i]=false;
	
	/* Add all submeshes to the new list: */
	for(size_t i=0;i<numSubMeshes;++i)
		if(!used[i])
			{
			/* Add the unused submesh: */
			newSubMeshes.push_back(subMeshes[i]);
			newSubMeshIndices.push_back(i);
			
			/* Add all other submeshes that have the same material properties: */
			for(size_t j=i+1;j<numSubMeshes;++j)
				{
				if(subMeshes[j].material==subMeshes[i].material)
					{
					newSubMeshes.push_back(subMeshes[j]);
					newSubMeshIndices.push_back(j);
					used[j]=true;
					}
				}
			used[i]=true;
			}
	delete[] used;
	
	/* Correct the submesh hierarchy linkages: */
	for(size_t i=0;i<numSubMeshes;++i)
		{
		newSubMeshes[i].parentIndex=newSubMeshIndices[newSubMeshes[i].parentIndex];
		for(CardList::iterator ciIt=newSubMeshes[i].childIndices.begin();ciIt!=newSubMeshes[i].childIndices.end();++ciIt)
			*ciIt=newSubMeshIndices[*ciIt];
		}
	
	/* Install the new submesh list: */
	std::swap(subMeshes,newSubMeshes);
	}

template <class MeshVertexParam>
inline
void
HierarchicalTriangleSet<MeshVertexParam>::createKdTree(
	void)
	{
	/* Create a list containing all triangle indices: */
	CardList triangleIndices;
	TriangleKdTree::Box domain=TriangleKdTree::Box::empty;
	for(typename std::vector<SubMesh>::const_iterator smIt=subMeshes.begin();smIt!=subMeshes.end();++smIt)
		if(smIt->numTriangles!=0)
			{
			/* Check if the name of the submesh contains "door" in some form, in which case don't add it (evil hack): */
			const char* nPtr=smIt->name.c_str();
			int state=0;
			while(*nPtr!='\0'&&state<4)
				{
				switch(state)
					{
					case 0: // "" recognized
						if(*nPtr=='D'||*nPtr=='d')
							state=1;
						break;
					
					case 1: // "d" recognized
						if(*nPtr=='O'||*nPtr=='o')
							state=2;
						else
							{
							state=0;
							--nPtr;
							}
						break;
					
					case 2: // "do" recognized
						if(*nPtr=='O'||*nPtr=='o')
							state=3;
						else
							{
							state=0;
							--nPtr;
							}
						break;
					
					case 3: // "doo" recognized
						if(*nPtr=='R'||*nPtr=='r')
							state=4;
						else
							{
							state=0;
							--nPtr;
							}
						break;
					}
				++nPtr;
				}
			
			if(state!=4)
				{
				/* Store all triangles of the submesh: */
				Card vertexIndex=smIt->firstTriangleVertexIndex;
				for(Card i=0;i<smIt->numTriangles;++i,vertexIndex+=3)
					triangleIndices.push_back(vertexIndex);
				
				/* Add the bounding box of the submesh: */
				domain.addBox(smIt->boundingBox);
				}
			}
	
	/* Initialize the kd-tree: */
	triangleKdTree.createTree(domain,25,triangleIndices);
	}

template <class MeshVertexParam>
inline
const typename HierarchicalTriangleSetBase::SubMesh*
HierarchicalTriangleSet<MeshVertexParam>::findSubMesh(
	const typename HierarchicalTriangleSet<MeshVertexParam>::Point& p0,
	const typename HierarchicalTriangleSet<MeshVertexParam>::Point& p1) const
	{
	/* Restrict the search ray to the model's bounding box: */
	Point cp0=p0;
	Point cp1=p1;
	if(!limitRay(cp0,cp1))
		return 0;
	
	/* Intersect the ray interval with the kd-tree: */
	TriangleKdTree::Point kdp0=cp0;
	TriangleKdTree::Point kdp1=cp1;
	TriangleKdTree::IntersectResult kdResult=triangleKdTree.intersect(kdp0,kdp1);
	if(kdResult.triangleIndex!=TriangleKdTree::nil)
		{
		/* Find the submesh containing the intersected triangle: */
		#if 0
		size_t l=0;
		size_t r=subMeshes.size();
		while(r-l>1)
			{
			size_t m=(l+r)>>1;
			if(subMeshes[m].firstTriangleVertexIndex>kdResult.triangleIndex)
				r=m;
			else
				l=m;
			}
		return &subMeshes[l];
		#else
		for(typename std::vector<SubMesh>::const_iterator smIt=subMeshes.begin();smIt!=subMeshes.end();++smIt)
			if(kdResult.triangleIndex>=smIt->firstTriangleVertexIndex&&kdResult.triangleIndex<smIt->firstTriangleVertexIndex+smIt->numTriangles*3)
				return &(*smIt);
		#endif
		}
	else
		return 0;
	}
