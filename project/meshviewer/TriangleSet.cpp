/***********************************************************************
TriangleSet - Class to represent 3D objects as sets of triangles, for
efficient rendering using OpenGL.
Copyright (c) 2009 Oliver Kreylos
***********************************************************************/

#define TRIANGLESET_IMPLEMENTATION

#include "TriangleSet.h"

// DEBUGGING
#include <iostream>
#include <GL/gl.h>
#include <GL/GLVertexArrayParts.h>
#include <GL/GLVertexArrayTemplates.h>
#include <GL/GLContextData.h>
#include <GL/GLExtensionManager.h>
#include <GL/Extensions/GLARBVertexBufferObject.h>
#include <GL/GLGeometryWrappers.h>

/**************************************
Methods of class TriangleSet::DataItem:
**************************************/

template <class MeshVertexParam>
inline
TriangleSet<MeshVertexParam>::DataItem::DataItem(
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
TriangleSet<MeshVertexParam>::DataItem::~DataItem(
	void)
	{
	if(vertexBufferId!=0)
		{
		/* Delete the vertex buffer object: */
		glDeleteBuffersARB(1,&vertexBufferId);
		}
	}

/****************************
Methods of class TriangleSet:
****************************/

template <class MeshVertexParam>
inline
TriangleSet<MeshVertexParam>::TriangleSet(
	void)
	{
	/* Initialize the current sub mesh: */
	currentSubMesh.materialIndex=-1;
	currentSubMesh.numTriangles=0;
	currentSubMesh.firstTriangleVertexIndex=0;
	}

template <class MeshVertexParam>
inline
TriangleSet<MeshVertexParam>::~TriangleSet(
	void)
	{
	}

template <class MeshVertexParam>
inline
PolygonModel::Box
TriangleSet<MeshVertexParam>::calcBoundingBox(
	void) const
	{
	/* Find the bounding box of all mesh vertices: */
	Box result=Box::empty;
	for(typename std::vector<MeshVertex>::const_iterator vIt=vertices.begin();vIt!=vertices.end();++vIt)
		result.addPoint(vIt->position);
	
	return result;
	}

template <class MeshVertexParam>
inline
void
TriangleSet<MeshVertexParam>::glRenderAction(
	GLContextData& contextData) const
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
	#if 0
	glDrawArrays(GL_TRIANGLES,0,vertices.size());
	#else
	for(typename std::vector<SubMesh>::const_iterator smIt=subMeshes.begin();smIt!=subMeshes.end();++smIt)
		{
		/* Install the sub mesh's material properties: */
		materials[smIt->materialIndex]->set(contextData);
		
		/* Draw the sub mesh's triangles: */
		glDrawArrays(GL_TRIANGLES,smIt->firstTriangleVertexIndex,smIt->numTriangles*3);
		
		/* Uninstall the sub mesh's material properties: */
		materials[smIt->materialIndex]->reset(contextData);
		}
	#endif
	
	/* Uninstall the vertex arrays: */
	if(dataItem->vertexBufferId!=0)
		{
		/* Unbind the vertex buffer object: */
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
		}
	GLVertexArrayParts::disable(GLVertexArrayParts::Position|GLVertexArrayParts::Normal|GLVertexArrayParts::TexCoord);
	}

template <class MeshVertexParam>
inline
typename TriangleSet<MeshVertexParam>::Point
TriangleSet<MeshVertexParam>::intersect(
	const typename TriangleSet<MeshVertexParam>::Point& p0,
	const typename TriangleSet<MeshVertexParam>::Point& p1) const
	{
	/* Intersect the ray segment with each triangle in the triangle set: */
	Point firstIntersection=p1;
	Card triangleVertexIndex;
	for(typename std::vector<SubMesh>::const_iterator smIt=subMeshes.begin();smIt!=subMeshes.end();++smIt)
		{
		const MeshVertex* vPtr=&vertices[smIt->firstTriangleVertexIndex];
		for(Card triangleIndex=0;triangleIndex<smIt->numTriangles;++triangleIndex,vPtr+=3)
			{
			/* Intersect the ray with the triangle's plane: */
			Vector t0,t1;
			for(int i=0;i<3;++i)
				{
				t0[i]=Scalar(vPtr[1].position[i])-Scalar(vPtr[0].position[i]);
				t1[i]=Scalar(vPtr[2].position[i])-Scalar(vPtr[0].position[i]);
				}
			Vector normal=Geometry::cross(t0,t1);
			Scalar offset=normal[0]*Scalar(vPtr[0].position[0])+normal[1]*Scalar(vPtr[0].position[1])+normal[2]*Scalar(vPtr[0].position[2]);
			Scalar d0=normal*p0-offset;
			Scalar d1=normal*firstIntersection-offset;
			if((d0<=Scalar(0)&&d1>Scalar(0))||(d0>=Scalar(0)&&d1<Scalar(0)))
				{
				/* Check whether the intersection point lies within the triangle: */
				Point intersection=Geometry::affineCombination(p0,firstIntersection,(Scalar(0)-d0)/(d1-d0));
				bool inside=true;
				for(int i=0;i<3&&inside;++i)
					{
					Vector edge;
					for(int j=0;j<3;++j)
						edge[j]=Scalar(vPtr[(i+1)%3].position[j])-Scalar(vPtr[i].position[j]);
					Vector edgeNormal=Geometry::cross(normal,edge);
					Scalar edgeOffset=edgeNormal[0]*Scalar(vPtr[i].position[0])+edgeNormal[1]*Scalar(vPtr[i].position[1])+edgeNormal[2]*Scalar(vPtr[i].position[2]);
					inside=intersection*edgeNormal>=edgeOffset;
					}
				if(inside)
					{
					/* Update the first intersection: */
					firstIntersection=intersection;
					triangleVertexIndex=smIt->firstTriangleVertexIndex+triangleIndex*3;
					}
				}
			}
		}
	
	#if 0
	if(firstIntersection!=p1)
		{
		/* Print debugging information about the picked triangle: */
		const MeshVertex* vs=&vertices[triangleVertexIndex];
		
		for(int i=0;i<3;++i)
			{
			std::cout<<"Vertex "<<i<<": ";
			std::cout<<"("<<vs[i].position[0]<<", "<<vs[i].position[1]<<", "<<vs[i].position[2]<<"), ";
			std::cout<<"("<<vs[i].normal[0]<<", "<<vs[i].normal[1]<<", "<<vs[i].normal[2]<<")"<<std::endl;
			}
		
		MVector normal=Geometry::cross(vs[1].position-vs[0].position,vs[2].position-vs[0].position);
		normal.normalize();
		std::cout<<"Face normal: "<<normal[0]<<", "<<normal[1]<<", "<<normal[2]<<std::endl;
		std::cout<<std::endl;
		}
	#endif
	
	return firstIntersection;
	}

template <class MeshVertexParam>
inline
void
TriangleSet<MeshVertexParam>::initContext(
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
typename TriangleSet<MeshVertexParam>::Card
TriangleSet<MeshVertexParam>::addMaterial(
	MaterialPointer newMaterial)
	{
	/* Get the new material's index: */
	Card result=materials.size();
	
	/* Add the material: */
	materials.push_back(newMaterial);
	
	return result;
	}

template <class MeshVertexParam>
inline
typename TriangleSet<MeshVertexParam>::Card
TriangleSet<MeshVertexParam>::addVertex(
	const typename TriangleSet<MeshVertexParam>::MeshVertex& newVertex)
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
TriangleSet<MeshVertexParam>::setSubMeshMaterial(
	typename TriangleSet<MeshVertexParam>::Card materialIndex)
	{
	currentSubMesh.materialIndex=materialIndex;
	}

template <class MeshVertexParam>
inline
void
TriangleSet<MeshVertexParam>::finishSubMesh(
	void)
	{
	/* Finalize the sub mesh: */
	Card firstTriangleVertexIndex=vertices.size();
	currentSubMesh.numTriangles=(firstTriangleVertexIndex-currentSubMesh.firstTriangleVertexIndex)/3;
	
	/* Store the sub mesh: */
	if(currentSubMesh.numTriangles>0)
		subMeshes.push_back(currentSubMesh);
	
	/* Prepare the next sub mesh: */
	currentSubMesh.materialIndex=-1;
	currentSubMesh.numTriangles=0;
	currentSubMesh.firstTriangleVertexIndex=firstTriangleVertexIndex;
	}

template <class MeshVertexParam>
inline
void
TriangleSet<MeshVertexParam>::addTriangleSet(
	const TriangleSet<MeshVertexParam>& other,
	const typename TriangleSet<MeshVertexParam>::Transform& vertexTransform)
	{
	/* Calculate the normal transformation: */
	Geometry::Matrix<MScalar,3,3> normalTransform;
	for(int i=0;i<3;++i)
		for(int j=0;j<3;++j)
			normalTransform(i,j)=vertexTransform.getMatrix()(j,i);
	normalTransform=Geometry::invert(normalTransform);
	
	/* Create a hash table of materials: */
	typedef Misc::HashTable<Material*,Card> MaterialMap;
	MaterialMap materialMap(101);
	for(Card i=0;i<Card(materials.size());++i)
		materialMap.setEntry(MaterialMap::Entry(materials[i].getPointer(),i));
	
	/* Process all submeshes of the other triangle set: */
	for(typename std::vector<SubMesh>::const_iterator smIt=other.subMeshes.begin();smIt!=other.subMeshes.end();++smIt)
		{
		/* Start a new submesh: */
		Material* material=other.materials[smIt->materialIndex].getPointer();
		if(materialMap.isEntry(material))
			currentSubMesh.materialIndex=materialMap.getEntry(material).getDest();
		else
			{
			currentSubMesh.materialIndex=Card(materials.size());
			materials.push_back(material);
			materialMap.setEntry(MaterialMap::Entry(material,currentSubMesh.materialIndex));
			}
		currentSubMesh.numTriangles=smIt->numTriangles;
		currentSubMesh.firstTriangleVertexIndex=Card(vertices.size());
		
		/* Copy the submesh's vertices: */
		typename std::vector<MeshVertex>::const_iterator vIt=other.vertices.begin()+smIt->firstTriangleVertexIndex;
		Card numVertices=smIt->numTriangles*3;
		for(Card i=0;i<numVertices;++i,++vIt)
			{
			/* Copy and transform the source vertex: */
			MeshVertex v=*vIt;
			v.tangentS=vertexTransform.transform(v.tangentS);
			v.tangentT=vertexTransform.transform(v.tangentT);
			v.normal=MVector(normalTransform*v.normal);
			v.position=vertexTransform.transform(v.position);
			vertices.push_back(v);
			}
		
		subMeshes.push_back(currentSubMesh);
		}
	
	/* Prepare the next sub mesh: */
	currentSubMesh.materialIndex=-1;
	currentSubMesh.numTriangles=0;
	currentSubMesh.firstTriangleVertexIndex=Card(vertices.size());
	}
