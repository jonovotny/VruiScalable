/***********************************************************************
PolygonModel - Abstract base class to represent different kinds of
polygonal models.
Copyright (c) 2009-2012 Oliver Kreylos
***********************************************************************/

#ifndef POLYGONMODEL_INCLUDED
#define POLYGONMODEL_INCLUDED

#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/Box.h>

/* Forward declarations: */
class GLContextData;

class PolygonModel
	{
	/* Embedded classes: */
	public:
	typedef double Scalar; // Scalar type
	typedef Geometry::Point<Scalar,3> Point; // Type for points
	typedef Geometry::Vector<Scalar,3> Vector; // Type for vectors
	typedef Geometry::Box<Scalar,3> Box; // Type for axis-aligned boxes
	
	/* Constructors and destructors: */
	public:
	virtual ~PolygonModel(void) // Destroys the polygon model
		{
		}
	
	/* Methods: */
	virtual Box calcBoundingBox(void) const =0; // Returns an axis-aligned box bounding the model
	virtual void glRenderAction(GLContextData& contextData) const =0; // Renders the polygon model into the current OpenGL context
	virtual Point intersect(const Point& p0,const Point& p1) const =0; // Intersects the polygon model with the given ray segment going from p0 to p1
	virtual Scalar traceBox(const Box& box,const Vector& displacement,Vector& hitNormal) const // Traces a box through the model and returns the relative position of the first intersection; sets hitNormal to intersection if result is < 1.0
		{
		/* Default: no intersections ever: */
		return Scalar(1);
		}
	virtual void loadBSPTree(const char* bspTreeFileName) // Loads a BSP tree to support intersection tests on the model
		{
		/* Default model don't support BSP trees */
		}
	};

#endif
