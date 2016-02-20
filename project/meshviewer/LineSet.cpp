/***********************************************************************
LineSet - Class to represent sets of 3D lines.
Copyright (c) 2009-2011 Oliver Kreylos
***********************************************************************/

#define LINESET_IMPLEMENTATION

#include "LineSet.h"

#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLVertexArrayParts.h>
#include <GL/GLVertexArrayTemplates.h>
#include <GL/GLContextData.h>
#include <GL/GLExtensionManager.h>
#include <GL/Extensions/GLARBVertexBufferObject.h>
#include <GL/GLGeometryWrappers.h>

/**********************************
Methods of class LineSet::DataItem:
**********************************/

template <class MeshVertexParam>
inline
LineSet<MeshVertexParam>::DataItem::DataItem(
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
LineSet<MeshVertexParam>::DataItem::~DataItem(
	void)
	{
	if(vertexBufferId!=0)
		{
		/* Delete the vertex buffer object: */
		glDeleteBuffersARB(1,&vertexBufferId);
		}
	}

/************************
Methods of class LineSet:
************************/

template <class MeshVertexParam>
inline
LineSet<MeshVertexParam>::LineSet(
	void)
	{
	/* Initialize the current mesh part: */
	currentSubMesh.color=Color(1.0f,1.0f,1.0f);
	currentSubMesh.firstLineVertexIndex=0;
	currentSubMesh.numVertices=0;
	}

template <class MeshVertexParam>
inline
LineSet<MeshVertexParam>::~LineSet(
	void)
	{
	}

template <class MeshVertexParam>
inline
PolygonModel::Box
LineSet<MeshVertexParam>::calcBoundingBox(
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
LineSet<MeshVertexParam>::glRenderAction(
	GLContextData& contextData) const
	{
	/* Get the context data item: */
	DataItem* dataItem=contextData.template retrieveDataItem<DataItem>(this);
	
	/* Initialize OpenGL state: */
	glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
	glDisable(GL_LIGHTING);
	glLineWidth(1.0f);
	
	/* Install the vertex arrays: */
	GLVertexArrayParts::enable(GLVertexArrayParts::Position);
	if(dataItem->vertexBufferId!=0)
		{
		/* Bind the vertex buffer object: */
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,dataItem->vertexBufferId);
		
		/* Render from the vertex buffer object: */
		const Scalar* vertexPointer=0;
		size_t vertexSize=sizeof(Scalar)*3;
		glVertexPointer(3,vertexSize,vertexPointer+0);
		}
	else
		{
		/* Render directly from application memory: */
		const MeshVertex* vertexPointer=&vertices.front();
		glVertexPointer(3,sizeof(MeshVertex),vertexPointer[0].position.getComponents());
		}
	
	/* Render all mesh parts: */
	for(typename std::vector<SubMesh>::const_iterator smIt=subMeshes.begin();smIt!=subMeshes.end();++smIt)
		{
		glColor(smIt->color);
		glDrawArrays(GL_LINES,smIt->firstLineVertexIndex,smIt->numVertices);
		}
	
	/* Uninstall the vertex arrays: */
	if(dataItem->vertexBufferId!=0)
		{
		/* Unbind the vertex buffer object: */
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
		}
	GLVertexArrayParts::disable(GLVertexArrayParts::Position);
	
	/* Restore OpenGL state: */
	glPopAttrib();
	}

template <class MeshVertexParam>
inline
PolygonModel::Point
LineSet<MeshVertexParam>::intersect(
	const PolygonModel::Point& p0,
	const PolygonModel::Point& p1) const
	{
	return p1;
	}

template <class MeshVertexParam>
inline
void
LineSet<MeshVertexParam>::initContext(
	GLContextData& contextData) const
	{
	/* Create a new context data item: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	
	if(dataItem->vertexBufferId!=0)
		{
		/* Upload the line set into the vertex buffer object: */
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,dataItem->vertexBufferId);
		size_t vertexSize=sizeof(Scalar)*3;
		glBufferDataARB(GL_ARRAY_BUFFER_ARB,vertices.size()*vertexSize,0,GL_STATIC_DRAW_ARB);
		Scalar* bufferPtr=static_cast<Scalar*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
		
		for(typename std::vector<MeshVertex>::const_iterator vIt=vertices.begin();vIt!=vertices.end();++vIt)
			{
			for(int i=0;i<3;++i)
				*(bufferPtr++)=vIt->position[i];
			}
		
		glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
		}
	}

template <class MeshVertexParam>
inline
typename LineSet<MeshVertexParam>::Card
LineSet<MeshVertexParam>::addVertex(
	const typename LineSet<MeshVertexParam>::MeshVertex& newVertex)
	{
	/* Get the new vertex's index: */
	Card result=vertices.size();
	
	/* Add the vertex to the current mesh part: */
	vertices.push_back(newVertex);
	
	return result;
	}

template <class MeshVertexParam>
inline
void
LineSet<MeshVertexParam>::setSubMeshColor(
	const typename LineSet<MeshVertexParam>::Color& newColor)
	{
	currentSubMesh.color=newColor;
	}

template <class MeshVertexParam>
inline
typename LineSet<MeshVertexParam>::Card
LineSet<MeshVertexParam>::finishSubMesh(
	void)
	{
	/* Get the current number of vertices: */
	Card numVertices=vertices.size();
	
	/* Store the current mesh part in the list: */
	Card result=subMeshes.size();
	currentSubMesh.numVertices=numVertices-currentSubMesh.firstLineVertexIndex;
	subMeshes.push_back(currentSubMesh);
	
	/* Re-initialize the current mesh part: */
	currentSubMesh.color=Color(1.0f,1.0f,1.0);
	currentSubMesh.firstLineVertexIndex=numVertices;
	currentSubMesh.numVertices=0;
	
	return result;
	}

template <class MeshVertexParam>
inline
void
LineSet<MeshVertexParam>::setSubMeshColor(
	typename LineSet<MeshVertexParam>::Card subMeshIndex,
	const typename LineSet<MeshVertexParam>::Color& newColor)
	{
	subMeshes[subMeshIndex].color=newColor;
	}
