/***********************************************************************
MultiModel - Class to represent polygon models consisting of several
independent parts.
Copyright (c) 2009-2010 Oliver Kreylos
***********************************************************************/

#ifndef MULTIMODEL_INCLUDED
#define MULTIMODEL_INCLUDED

#include <vector>

#include "PolygonModel.h"

/* Forward declarations: */
class HierarchicalTriangleSetBase;

class MultiModel:public PolygonModel
	{
	/* Elements: */
	private:
	std::vector<PolygonModel*> parts; // List of part models
	
	/* Constructors and destructors: */
	public:
	MultiModel(void); // Creates a multi model with no part models
	virtual ~MultiModel(void); // Destroys the multi model and its parts
	
	/* Methods from PolygonModel: */
	virtual Box calcBoundingBox(void) const;
	virtual void glRenderAction(GLContextData& contextData) const;
	virtual Point intersect(const Point& p0,const Point& p1) const;
	virtual Scalar traceBox(const Box& box,const Vector& displacement,Vector& hitNormal) const;
	virtual void loadBSPTree(const char* bspTreeFileName);
	
	/* New methods: */
	void addPart(PolygonModel* newPart); // Adds the given model to the multi model
	const HierarchicalTriangleSetBase* getHierarchicalTriangleSet(void) const; // Returns a pointer to the first hierarchical triangle set contained in the model
	};

#endif
