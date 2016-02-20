/***********************************************************************
MultiModel - Class to represent polygon models consisting of several
independent parts.
Copyright (c) 2009-2010 Oliver Kreylos
***********************************************************************/

#include "MultiModel.h"

#include "HierarchicalTriangleSetBase.h"

/***************************
Methods of class MultiModel:
***************************/

MultiModel::MultiModel(void)
	{
	}

MultiModel::~MultiModel(void)
	{
	/* Delete all parts: */
	for(std::vector<PolygonModel*>::iterator pIt=parts.begin();pIt!=parts.end();++pIt)
		delete *pIt;
	}

PolygonModel::Box MultiModel::calcBoundingBox(void) const
	{
	/* Calculate the union of all part bounding boxes: */
	Box result=Box::empty;
	for(std::vector<PolygonModel*>::const_iterator pIt=parts.begin();pIt!=parts.end();++pIt)
		result.addBox((*pIt)->calcBoundingBox());
	return result;
	}

void MultiModel::glRenderAction(GLContextData& contextData) const
	{
	/* Render all parts: */
	for(std::vector<PolygonModel*>::const_iterator pIt=parts.begin();pIt!=parts.end();++pIt)
		(*pIt)->glRenderAction(contextData);
	}

MultiModel::Point MultiModel::intersect(const MultiModel::Point& p0,const MultiModel::Point& p1) const
	{
	/* Intersect the ray with all model parts: */
	Point firstIntersection=p1;
	for(std::vector<PolygonModel*>::const_iterator pIt=parts.begin();pIt!=parts.end();++pIt)
		{
		Point intersection=(*pIt)->intersect(p0,firstIntersection);
		if(firstIntersection!=intersection)
			firstIntersection=intersection;
		}
	
	return firstIntersection;
	}

MultiModel::Scalar MultiModel::traceBox(const MultiModel::Box& box,const MultiModel::Vector& displacement,MultiModel::Vector& hitNormal) const
	{
	/* Trace the box through all model parts: */
	Scalar minLambda(1);
	for(std::vector<PolygonModel*>::const_iterator pIt=parts.begin();pIt!=parts.end();++pIt)
		{
		Vector normal;
		Scalar lambda=(*pIt)->traceBox(box,displacement,normal);
		if(minLambda>lambda)
			{
			minLambda=lambda;
			hitNormal=normal;
			}
		}
	
	return minLambda;
	}

void MultiModel::loadBSPTree(const char* bspTreeFileName)
	{
	/* Load the BSP tree into all model parts: */
	for(std::vector<PolygonModel*>::iterator pIt=parts.begin();pIt!=parts.end();++pIt)
		(*pIt)->loadBSPTree(bspTreeFileName);
	}

void MultiModel::addPart(PolygonModel* newPart)
	{
	parts.push_back(newPart);
	}

const HierarchicalTriangleSetBase* MultiModel::getHierarchicalTriangleSet(void) const
	{
	const HierarchicalTriangleSetBase* result=0;
	for(std::vector<PolygonModel*>::const_iterator pIt=parts.begin();pIt!=parts.end()&&result==0;++pIt)
		result=dynamic_cast<const HierarchicalTriangleSetBase*>(*pIt);
	return result;
	}
