/***********************************************************************
CurveSet - Class to represent sets of 3D curves.
Copyright (c) 2011 Oliver Kreylos
***********************************************************************/

#ifndef CURVESET_INCLUDED
#define CURVESET_INCLUDED

#include <vector>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/HVector.h>
#include <Geometry/BSpline.h>
#include <GL/gl.h>
#include <GL/GLColor.h>
#include <GL/GLObject.h>
#include <GL/GLGeometryVertex.h>

#include "PolygonModel.h"

template <class ScalarParam>
class CurveSet:public PolygonModel,public GLObject
	{
	/* Embedded classes: */
	public:
	typedef ScalarParam CScalar; // Scalar type for points and vectors
	typedef Geometry::Point<CScalar,3> CPoint; // Type for curve points
	typedef Geometry::Vector<CScalar,3> CVector; // Type for curve vectors
	typedef Geometry::HVector<CScalar,3> CHVector; // Type for projective curve points
	typedef unsigned int Card; // Cardinal integer type
	typedef Geometry::BSpline<CPoint,1> BSC; // Type for non-uniform non-rational B-spline curves
	typedef Geometry::BSpline<CHVector,1> RBSC; // Type for non-uniform rational B-spline curves
	
	struct SubMesh // Structure for mesh parts sharing common material properties
		{
		/* Elements: */
		public:
		Card firstCurveIndex; // Index of first curve in submesh
		Card numCurves; // Number of curves in submesh
		};
	
	private:
	typedef GLGeometry::Vertex<void,0,void,0,CScalar,CScalar,3> CurveVertex; // Type for discretized curve vertices
	
	struct Curve // Structure for discretized curves
		{
		/* Elements: */
		public:
		Card firstVertexIndex; // Index of curve's first vertex
		Card numVertices; // Number of vertices
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
	std::vector<CurveVertex> vertices; // List of curve vertices
	std::vector<Curve> curves; // List of curves
	std::vector<SubMesh> subMeshes; // List of mesh parts
	SubMesh currentSubMesh; // Submesh receiving added curves
	
	/* Constructors and destructors: */
	public:
	CurveSet(void); // Creates empty curve set
	virtual ~CurveSet(void); // Destroys the curve set
	
	/* Methods from PolygonModel: */
	virtual Box calcBoundingBox(void) const;
	virtual void glRenderAction(GLContextData& contextData) const;
	virtual Point intersect(const Point& p0,const Point& p1) const;
	
	/* Methods from GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	
	/* New Methods: */
	void addCurve(const BSC& newCurve); // Adds the given curve to the curve set
	void addCurve(const RBSC& newCurve); // Adds the given curve to the curve set
	Card finishSubMesh(void); // Finishes adding curves to a mesh part and returns its index
	Card getNumCurves(void) const // Returns the total number of curves in the curve set
		{
		return curves.size();
		}
	Card getNumSubMeshes(void) const // Returns the number of mesh parts in the curve set
		{
		return subMeshes.size();
		}
	const SubMesh& getSubMesh(Card subMeshIndex) const // Returns the mesh part of the given index
		{
		return subMeshes[subMeshIndex];
		}
	};

#ifndef CURVESET_IMPLEMENTATION
#include "CurveSet.cpp"
#endif

#endif
