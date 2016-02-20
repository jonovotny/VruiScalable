/***********************************************************************
CurveSet - Class to represent sets of 3D curves.
Copyright (c) 2011 Oliver Kreylos
***********************************************************************/

#define CURVESET_IMPLEMENTATION

#include "CurveSet.h"

#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLVertexArrayParts.h>
#include <GL/GLVertexArrayTemplates.h>
#include <GL/GLContextData.h>
#include <GL/GLExtensionManager.h>
#include <GL/Extensions/GLARBVertexBufferObject.h>
#include <GL/GLGeometryWrappers.h>

/***********************************
Methods of class CurveSet::DataItem:
***********************************/

template <class ScalarParam>
inline
CurveSet<ScalarParam>::DataItem::DataItem(
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

template <class ScalarParam>
inline
CurveSet<ScalarParam>::DataItem::~DataItem(
	void)
	{
	if(vertexBufferId!=0)
		{
		/* Delete the vertex buffer object: */
		glDeleteBuffersARB(1,&vertexBufferId);
		}
	}

/*************************
Methods of class CurveSet:
*************************/

template <class ScalarParam>
inline
CurveSet<ScalarParam>::CurveSet(
	void)
	{
	/* Initialize the current mesh part: */
	currentSubMesh.firstCurveIndex=0;
	currentSubMesh.numCurves=0;
	}

template <class ScalarParam>
inline
CurveSet<ScalarParam>::~CurveSet(
	void)
	{
	}

template <class ScalarParam>
inline
PolygonModel::Box
CurveSet<ScalarParam>::calcBoundingBox(
	void) const
	{
	/* Find the bounding box of all curve vertices: */
	Box result=Box::empty;
	for(typename std::vector<CurveVertex>::const_iterator vIt=vertices.begin();vIt!=vertices.end();++vIt)
		result.addPoint(vIt->position);
	
	return result;
	}

template <class ScalarParam>
inline
void
CurveSet<ScalarParam>::glRenderAction(
	GLContextData& contextData) const
	{
	/* Get the context data item: */
	DataItem* dataItem=contextData.template retrieveDataItem<DataItem>(this);
	
	/* Initialize OpenGL state: */
	glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
	glDisable(GL_LIGHTING);
	glLineWidth(1.0f);
	glColor3f(1.0f,1.0f,1.0f);
	
	/* Install the vertex arrays: */
	GLVertexArrayParts::enable(CurveVertex::getPartsMask());
	if(dataItem->vertexBufferId!=0)
		{
		/* Bind the vertex buffer object: */
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,dataItem->vertexBufferId);
		
		/* Render from the vertex buffer object: */
		glVertexPointer(static_cast<const CurveVertex*>(0));
		}
	else
		{
		/* Render directly from application memory: */
		glVertexPointer(&vertices.front());
		}
	
	/* Render all mesh parts: */
	for(typename std::vector<SubMesh>::const_iterator smIt=subMeshes.begin();smIt!=subMeshes.end();++smIt)
		{
		for(Card i=0;i<smIt->numCurves;++i)
			{
			const Curve& c=curves[smIt->firstCurveIndex+i];
			glDrawArrays(GL_LINE_STRIP,c.firstVertexIndex,c.numVertices);
			}
		}
	
	/* Uninstall the vertex arrays: */
	if(dataItem->vertexBufferId!=0)
		{
		/* Unbind the vertex buffer object: */
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
		}
	GLVertexArrayParts::disable(CurveVertex::getPartsMask());
	
	/* Restore OpenGL state: */
	glPopAttrib();
	}

template <class ScalarParam>
inline
PolygonModel::Point
CurveSet<ScalarParam>::intersect(
	const PolygonModel::Point& p0,
	const PolygonModel::Point& p1) const
	{
	return p1;
	}

template <class ScalarParam>
inline
void
CurveSet<ScalarParam>::initContext(
	GLContextData& contextData) const
	{
	/* Create a new context data item: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	
	if(dataItem->vertexBufferId!=0)
		{
		/* Upload the curve set's vertices into the vertex buffer object: */
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,dataItem->vertexBufferId);
		glBufferDataARB(GL_ARRAY_BUFFER_ARB,vertices.size()*sizeof(CurveVertex),0,GL_STATIC_DRAW_ARB);
		CurveVertex* bufferPtr=static_cast<CurveVertex*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
		
		for(typename std::vector<CurveVertex>::const_iterator vIt=vertices.begin();vIt!=vertices.end();++vIt,++bufferPtr)
			*bufferPtr=*vIt;
		
		glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
		}
	}

template <class ScalarParam>
inline
void
CurveSet<ScalarParam>::addCurve(
	const typename CurveSet<ScalarParam>::BSC& newCurve)
	{
	Card degree=Card(newCurve.getDegree());
	Card numKnots=Card(newCurve.getNumKnots());
	
	/* Initialize the left limit of the current knot interval: */
	Card i0=degree-1; // Index of interval's left knot
	typename BSC::Parameter p0=newCurve.getKnot(i0); // Interval's left parameter value
	Card mult0; // Multiplicity of interval's left knot
	for(mult0=1;i0>=mult0&&newCurve.getKnot(i0-mult0)==p0;++mult0)
		;
	while(i0+1<numKnots&&newCurve.getKnot(i0+1)==p0)
		{
		++i0;
		++mult0;
		}
	
	/* Iterate through the spline's knot intervals: */
	typename BSC::EvaluationCache* ec=newCurve.createEvaluationCache();
	while(i0+1<numKnots)
		{
		/* Generate the right limit of the current knot interval: */
		Card i1=i0+1;
		typename BSC::Parameter p1=newCurve.getKnot(i1);
		Card mult1;
		for(mult1=1;i1+mult1<numKnots&&newCurve.getKnot(i1+mult1)==p1;++mult1)
			;
		
		/* Generate the interval's curve: */
		Curve c;
		c.firstVertexIndex=vertices.size();
		c.numVertices=0;
		
		/* Generate the interval's first vertex: */
		CurveVertex cv;
		if(mult0>=degree)
			cv.position=newCurve.getPoint(i0-degree+1);
		else
			cv.position=newCurve.evaluate(p0,ec);
		vertices.push_back(cv);
		++c.numVertices;
		
		/* Generate the interval's interior vertices: */
		for(int i=1;i<15;++i)
			{
			typename BSC::Parameter p=p0+(p1-p0)*(CScalar(i)/CScalar(16));
			cv.position=newCurve.evaluate(p,ec);
			vertices.push_back(cv);
			++c.numVertices;
			}
		
		/* Generate the interval's last vertex: */
		if(mult1>=degree)
			cv.position=newCurve.getPoint(i1);
		else
			cv.position=newCurve.evaluate(p1,ec);
		vertices.push_back(cv);
		++c.numVertices;
		
		/* Finalize the interval's curve: */
		curves.push_back(c);
		++currentSubMesh.numCurves;
		
		/* Go to the next knot interval: */
		i0=i1+mult1-1;
		p0=p1;
		mult0=mult1;
		}
	delete ec;
	}

template <class ScalarParam>
inline
void
CurveSet<ScalarParam>::addCurve(
	const typename CurveSet<ScalarParam>::RBSC& newCurve)
	{
	Card degree=Card(newCurve.getDegree());
	Card numKnots=Card(newCurve.getNumKnots());
	
	/* Initialize the left limit of the current knot interval: */
	Card i0=degree-1; // Index of interval's left knot
	typename RBSC::Parameter p0=newCurve.getKnot(i0); // Interval's left parameter value
	Card mult0; // Multiplicity of interval's left knot
	for(mult0=1;i0>=mult0&&newCurve.getKnot(i0-mult0)==p0;++mult0)
		;
	while(i0+1<numKnots&&newCurve.getKnot(i0+1)==p0)
		{
		++i0;
		++mult0;
		}
	
	/* Iterate through the spline's knot intervals: */
	typename RBSC::EvaluationCache* ec=newCurve.createEvaluationCache();
	while(i0+1<numKnots)
		{
		/* Generate the right limit of the current knot interval: */
		Card i1=i0+1;
		typename RBSC::Parameter p1=newCurve.getKnot(i1);
		Card mult1;
		for(mult1=1;i1+mult1<numKnots&&newCurve.getKnot(i1+mult1)==p1;++mult1)
			;
		
		/* Generate the interval's curve: */
		Curve c;
		c.firstVertexIndex=vertices.size();
		c.numVertices=0;
		
		/* Generate the interval's first vertex: */
		CurveVertex cv;
		if(mult0>=degree)
			cv.position=newCurve.getPoint(i0-degree+1).toPoint();
		else
			cv.position=newCurve.evaluate(p0,ec).toPoint();
		vertices.push_back(cv);
		++c.numVertices;
		
		/* Generate the interval's interior vertices: */
		for(int i=1;i<15;++i)
			{
			typename RBSC::Parameter p=p0+(p1-p0)*(CScalar(i)/CScalar(16));
			cv.position=newCurve.evaluate(p,ec).toPoint();
			vertices.push_back(cv);
			++c.numVertices;
			}
		
		/* Generate the interval's last vertex: */
		if(mult1>=degree)
			cv.position=newCurve.getPoint(i1).toPoint();
		else
			cv.position=newCurve.evaluate(p1,ec).toPoint();
		vertices.push_back(cv);
		++c.numVertices;
		
		/* Finalize the interval's curve: */
		curves.push_back(c);
		++currentSubMesh.numCurves;
		
		/* Go to the next knot interval: */
		i0=i1+mult1-1;
		p0=p1;
		mult0=mult1;
		}
	delete ec;
	}

template <class ScalarParam>
inline
typename CurveSet<ScalarParam>::Card
CurveSet<ScalarParam>::finishSubMesh(
	void)
	{
	Card result=subMeshes.size();
	
	/* Bail out if the current mesh part is empty: */
	if(currentSubMesh.numCurves==0)
		return result;
	
	/* Store the current mesh part in the list: */
	subMeshes.push_back(currentSubMesh);
	
	/* Re-initialize the current mesh part: */
	currentSubMesh.firstCurveIndex+=currentSubMesh.numCurves;
	currentSubMesh.numCurves=0;
	
	return result;
	}
