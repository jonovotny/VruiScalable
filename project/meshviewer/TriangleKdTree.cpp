/***********************************************************************
TriangleKdTree - Class to represent kd-trees to support intersection
tests with sets of triangles.
Copyright (c) 2009-2012 Oliver Kreylos
***********************************************************************/

#include "TriangleKdTree.h"

// DEBUGGING
#include <assert.h>
#include <iostream>
#include <Misc/Timer.h>

#include <algorithm>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <Geometry/IntersectionTests.h>
#include <GL/gl.h>
#include <GL/GLGeometryWrappers.h>

namespace {

/****************
Helper functions:
****************/

template <class Scalar>
inline
Scalar
increment(
	Scalar value)
	{
	Scalar delta=Scalar(1);
	while(value+delta!=value)
		delta*=Scalar(0.5);
	if(delta==Scalar(0))
		delta=Math::Constants<Scalar>::smallest;
	while(value+delta==value)
		delta*=Scalar(2);
	Scalar newValue=value+delta;
	assert(newValue!=value);
	return newValue;
	}

template <class Scalar>
inline
Scalar
decrement(
	Scalar value)
	{
	Scalar delta=Scalar(1);
	while(value-delta!=value)
		delta*=Scalar(0.5);
	if(delta==Scalar(0))
		delta=Math::Constants<Scalar>::smallest;
	while(value-delta==value)
		delta*=Scalar(2);
	Scalar newValue=value-delta;
	assert(newValue!=value);
	return newValue;
	}

template <class Scalar>
inline
Geometry::Point<Scalar,3>
affCo(
	const Geometry::Point<Scalar,3>& p0,
	const Geometry::Point<Scalar,3>& p1,
	Scalar w1)
	{
	return Geometry::Point<Scalar,3>(p0[0]+(p1[0]-p0[0])*w1,p0[1]+(p1[1]-p0[1])*w1,p0[2]+(p1[2]-p0[2])*w1);
	}

}

/*******************************
Methods of class TriangleKdTree:
*******************************/

TriangleKdTree::Scalar
TriangleKdTree::findBestSplit(
	const TriangleKdTree::Box& domain,
	int dimension,
	const TriangleKdTree::CardList& triangleIndices,
	const TriangleKdTree::TriangleFragmentList& triangleFragments) const
	{
	/*******************************************************************
	Create sorted arrays of triangle intervals to find an optimal split
	plane:
	*******************************************************************/
	
	Card totalNumTriangles=triangleIndices.size()+triangleFragments.size();
	Scalar* starts=new Scalar[totalNumTriangles+1];
	Scalar* sPtr=starts;
	Scalar* ends=new Scalar[totalNumTriangles+1];
	Scalar* ePtr=ends;
	
	/* Process complete triangles: */
	for(CardList::const_iterator tiIt=triangleIndices.begin();tiIt!=triangleIndices.end();++tiIt,++sPtr,++ePtr)
		{
		*sPtr=*ePtr=vertices[*tiIt+0].position[dimension];
		for(int i=1;i<3;++i)
			{
			Scalar p=vertices[*tiIt+i].position[dimension];
			if(*sPtr>p)
				*sPtr=p;
			else if(*ePtr<p)
				*ePtr=p;
			}
		}
	
	/* Process triangle fragments: */
	for(TriangleFragmentList::const_iterator tfIt=triangleFragments.begin();tfIt!=triangleFragments.end();++tfIt,++sPtr,++ePtr)
		{
		*sPtr=*ePtr=tfIt->v[0][dimension];
		for(int i=1;i<3;++i)
			{
			Scalar p=tfIt->v[i][dimension];
			if(*sPtr>p)
				*sPtr=p;
			else if(*ePtr<p)
				*ePtr=p;
			}
		}
	
	/* Sort both arrays and add sentinel elements: */
	std::sort(starts,starts+totalNumTriangles);
	starts[totalNumTriangles]=Math::Constants<Scalar>::max;
	std::sort(ends,ends+totalNumTriangles);
	ends[totalNumTriangles]=Math::Constants<Scalar>::max;
	
	/* Check whether it is better to split at the median or truncate empty space: */
	Scalar mid=Math::mid(domain.min[dimension],domain.max[dimension]);
	Scalar splitPlane;
	if(starts[0]>=mid)
		{
		/* Truncate empty space from the left: */
		splitPlane=decrement(starts[0]);
		}
	else if(ends[totalNumTriangles-1]<=mid)
		{
		/* Truncate empty space from the right: */
		splitPlane=increment(ends[totalNumTriangles-1]);
		}
	else
		{
		/* Find a split position that balances the subtree and minimizes triangle splits: */
		Card si=0;
		Card ei=0;
		Scalar pos=starts[0];
		Card numLeftTriangles=0;
		Card numRightTriangles=totalNumTriangles;
		double bestness=Math::Constants<Scalar>::min;
		while(ei<totalNumTriangles)
			{
			/* Process all "events" at the current position: */
			while(starts[si]==pos)
				{
				++numLeftTriangles;
				++si;
				}
			while(ends[ei]==pos)
				{
				--numRightTriangles;
				++ei;
				}
			
			/* Calculate the "goodness" of the tentative split position: */
			double goodness=-Math::sqr(double(numLeftTriangles)-double(numRightTriangles))-double(numLeftTriangles+numRightTriangles-totalNumTriangles)*double(totalNumTriangles);
			if(bestness<goodness)
				{
				splitPlane=pos;
				bestness=goodness;
				}
			
			/* Go to the next position: */
			pos=starts[si];
			if(pos>ends[ei])
				pos=ends[ei];
			}
		
		if(bestness>=Math::sqr(double(totalNumTriangles))*-0.4)
			{
			/* Nudge the split position to the right: */
			splitPlane=increment(splitPlane);
			}
		else
			{
			/* Signal an invalid split: */
			splitPlane=domain.min[dimension];
			}
		}
	
	/* Clean up: */
	delete[] starts;
	delete[] ends;
	
	return splitPlane;
	}

void
TriangleKdTree::splitTriangle(
	TriangleKdTree::Card triangleIndex,
	const TriangleKdTree::Point& v0,
	const TriangleKdTree::Point& v1,
	const TriangleKdTree::Point& v2,
	int dimension,
	TriangleKdTree::Scalar splitPlane,
	TriangleKdTree::TriangleFragmentList triangleFragments[2])
	{
	/* Create arrays to hold the new front and back vertices: */
	Point front[4];
	Point* fPtr=front;
	Point back[4];
	Point* bPtr=back;
	
	/* Process the first edge: */
	if(v0[dimension]<=splitPlane)
		*(fPtr++)=v0;
	if(v0[dimension]>=splitPlane)
		*(bPtr++)=v0;
	if(v0[dimension]<splitPlane&&v1[dimension]>splitPlane)
		{
		/* Calculate the intersection point: */
		Point s=affCo(v0,v1,(splitPlane-v0[dimension])/(v1[dimension]-v0[dimension]));
		s[dimension]=splitPlane;
		*(fPtr++)=s;
		*(bPtr++)=s;
		}
	else if(v0[dimension]>splitPlane&&v1[dimension]<splitPlane)
		{
		/* Calculate the intersection point: */
		Point s=affCo(v1,v0,(splitPlane-v1[dimension])/(v0[dimension]-v1[dimension]));
		s[dimension]=splitPlane;
		*(fPtr++)=s;
		*(bPtr++)=s;
		}
	
	/* Process the second edge: */
	if(v1[dimension]<=splitPlane)
		*(fPtr++)=v1;
	if(v1[dimension]>=splitPlane)
		*(bPtr++)=v1;
	if(v1[dimension]<splitPlane&&v2[dimension]>splitPlane)
		{
		/* Calculate the intersection point: */
		Point s=affCo(v1,v2,(splitPlane-v1[dimension])/(v2[dimension]-v1[dimension]));
		s[dimension]=splitPlane;
		*(fPtr++)=s;
		*(bPtr++)=s;
		}
	else if(v1[dimension]>splitPlane&&v2[dimension]<splitPlane)
		{
		/* Calculate the intersection point: */
		Point s=affCo(v2,v1,(splitPlane-v2[dimension])/(v1[dimension]-v2[dimension]));
		s[dimension]=splitPlane;
		*(fPtr++)=s;
		*(bPtr++)=s;
		}
	
	/* Process the third edge: */
	if(v2[dimension]<=splitPlane)
		*(fPtr++)=v2;
	if(v2[dimension]>=splitPlane)
		*(bPtr++)=v2;
	if(v2[dimension]<splitPlane&&v0[dimension]>splitPlane)
		{
		/* Calculate the intersection point: */
		Point s=affCo(v2,v0,(splitPlane-v2[dimension])/(v0[dimension]-v2[dimension]));
		s[dimension]=splitPlane;
		*(fPtr++)=s;
		*(bPtr++)=s;
		}
	else if(v2[dimension]>splitPlane&&v0[dimension]<splitPlane)
		{
		/* Calculate the intersection point: */
		Point s=affCo(v0,v2,(splitPlane-v0[dimension])/(v2[dimension]-v0[dimension]));
		s[dimension]=splitPlane;
		*(fPtr++)=s;
		*(bPtr++)=s;
		}
	
	/* Extend single points or line segments into zero-area triangles: */
	while(fPtr-front<3)
		{
		fPtr[0]=fPtr[-1];
		++fPtr;
		}
	while(bPtr-back<3)
		{
		bPtr[0]=bPtr[-1];
		++bPtr;
		}
	
	#if 0
	// DEBUGGING
	Box triDomain=Box::empty;
	triDomain.addPoint(v0);
	triDomain.addPoint(v1);
	triDomain.addPoint(v2);
	for(Point* pPtr=front;pPtr!=fPtr;++pPtr)
		assert(triDomain.contains(*pPtr));
	for(Point* pPtr=back;pPtr!=bPtr;++pPtr)
		assert(triDomain.contains(*pPtr));
	#endif
	
	/* Assemble the front result triangle(s): */
	TriangleFragment f0;
	f0.originalIndex=triangleIndex;
	for(int i=0;i<3;++i)
		f0.v[i]=front[i];
	triangleFragments[0].push_back(f0);
	if(fPtr-front>3)
		{
		TriangleFragment f1;
		f1.originalIndex=triangleIndex;
		for(int i=0;i<3;++i)
			f1.v[i]=front[(i+2)%4];
		triangleFragments[0].push_back(f1);
		}
	
	/* Assemble the back result triangle(s): */
	TriangleFragment b0;
	b0.originalIndex=triangleIndex;
	for(int i=0;i<3;++i)
		b0.v[i]=back[i];
	triangleFragments[1].push_back(b0);
	if(bPtr-back>3)
		{
		TriangleFragment b1;
		b1.originalIndex=triangleIndex;
		for(int i=0;i<3;++i)
			b1.v[i]=back[(i+2)%4];
		triangleFragments[1].push_back(b1);
		}
	}

void
TriangleKdTree::initNode(
	TriangleKdTree::Node& node,
	const TriangleKdTree::Box& domain,
	const TriangleKdTree::CardList& triangleIndices,
	const TriangleKdTree::TriangleFragmentList& triangleFragments)
	{
	#if 0
	// DEBUGGING
	for(CardList::const_iterator tiIt=triangleIndices.begin();tiIt!=triangleIndices.end();++tiIt)
		checkTriangle(domain,vertices[*tiIt+0].position,vertices[*tiIt+1].position,vertices[*tiIt+2].position);
	for(TriangleFragmentList::const_iterator tfIt=triangleFragments.begin();tfIt!=triangleFragments.end();++tfIt)
		checkTriangle(domain,tfIt->v[0],tfIt->v[1],tfIt->v[2]);
	#endif
	
	/* Count the total number of distinct original triangle indices in the triangle and fragment lists: */
	Card numDistinctTriangles=triangleIndices.size();
	if(!triangleFragments.empty())
		{
		TriangleFragmentList::const_iterator tfIt=triangleFragments.begin();
		++numDistinctTriangles;
		Card lastIndex=tfIt->originalIndex;
		for(++tfIt;tfIt!=triangleFragments.end();++tfIt)
			if(lastIndex!=tfIt->originalIndex)
				{
				++numDistinctTriangles;
				lastIndex=tfIt->originalIndex;
				}
		}
	
	/* Check the number of distinct triangles against the limit: */
	if(numDistinctTriangles>maxTrianglesPerNode)
		{
		/*******************************************************************
		Create an interior node by distributing the given triangles and
		fragments between the node's two subdomains:  
		*******************************************************************/
		
		// DEBUGGING
		// std::cout<<"Creating interior node with "<<numDistinctTriangles<<" triangles"<<std::endl;
		
		/*******************************************************************
		Try at most three times to find a good split; if it fails, create a
		leaf node instead.
		*******************************************************************/
		
		int numTries;
		for(numTries=0;numTries<3;++numTries)
			{
			/* Find the optimal split plane: */
			node.plane=findBestSplit(domain,node.splitDimension,triangleIndices,triangleFragments);
			
			/* Check if the found split is beneficial: */
			if(node.plane>domain.min[node.splitDimension]&&node.plane<domain.max[node.splitDimension])
				break;
			
			/* Otherwise go to the next split dimension and try again: */
			node.splitDimension=(node.splitDimension+1)%3;
			}
		
		if(numTries<3)
			{
			/*******************************************************************
			Distribute complete triangles and triangle fragments to the two
			subdomains:
			*******************************************************************/
			
			CardList subTriangleIndices[2];
			TriangleFragmentList subTriangleFragments[2];
			
			/* Process all complete triangles: */
			for(CardList::const_iterator tiIt=triangleIndices.begin();tiIt!=triangleIndices.end();++tiIt)
				{
				const Vertex* vPtr=&vertices[*tiIt];
				Scalar s,e;
				s=e=vPtr[0].position[node.splitDimension];
				for(int i=1;i<3;++i)
					{
					Scalar p=vPtr[i].position[node.splitDimension];
					if(s>p)
						s=p;
					else if(e<p)
						e=p;
					}
				
				if(s<=node.plane&&e>=node.plane)
					{
					/* Split the triangle and assign the fragments to the subdomains: */
					splitTriangle(*tiIt,vPtr[0].position,vPtr[1].position,vPtr[2].position,node.splitDimension,node.plane,subTriangleFragments);
					}
				else
					{
					/* Assign the complete triangle to one or both subdomains: */
					if(s<=node.plane)
						subTriangleIndices[0].push_back(*tiIt);
					if(e>=node.plane)
						subTriangleIndices[1].push_back(*tiIt);
					}
				}
			
			/* Process all triangle fragments: */
			for(TriangleFragmentList::const_iterator tfIt=triangleFragments.begin();tfIt!=triangleFragments.end();++tfIt)
				{
				Scalar s,e;
				s=e=tfIt->v[0][node.splitDimension];
				for(int i=1;i<3;++i)
					{
					Scalar p=tfIt->v[i][node.splitDimension];
					if(s>p)
						s=p;
					else if(e<p)
						e=p;
					}
				
				if(s<=node.plane&&e>=node.plane)
					{
					/* Split the triangle fragment and assign the resulting fragments to the subdomains: */
					splitTriangle(tfIt->originalIndex,tfIt->v[0],tfIt->v[1],tfIt->v[2],node.splitDimension,node.plane,subTriangleFragments);
					}
				else
					{
					/* Assign the unchanged triangle fragment to one or both subdomains: */
					if(s<=node.plane)
						subTriangleFragments[0].push_back(*tfIt);
					if(e>=node.plane)
						subTriangleFragments[1].push_back(*tfIt);
					}
				}
			
			// DEBUGGING
			// std::cout<<"Creating subtree node with ";
			// std::cout<<subTriangleIndices[0].size()+subTriangleFragments[0].size()<<" left and ";
			// std::cout<<subTriangleIndices[1].size()+subTriangleFragments[1].size()<<" right triangles"<<std::endl;
			
			/* Create and initialize the node's children: */
			node.children=new Node[2];
			for(int i=0;i<2;++i)
				{
				Box subDomain=domain;
				if(i==0)
					subDomain.max[node.splitDimension]=node.plane;
				else
					subDomain.min[node.splitDimension]=node.plane;
				node.children[i].splitDimension=(node.splitDimension+1)%3;
				initNode(node.children[i],subDomain,subTriangleIndices[i],subTriangleFragments[i]);
				}
			
			return;
			}
		else
			{
			// DEBUGGING
			// std::cout<<"Punting on node with "<<numDistinctTriangles<<" triangles"<<std::endl;
			}
		}
	
	/*******************************************************************
	Create a leaf node containing the given triangles and fragments:
	*******************************************************************/
	
	// DEBUGGING
	// std::cout<<"Creating leaf node with "<<numDistinctTriangles<<" triangles"<<std::endl;
	
	node.triangleIndices.reserve(numDistinctTriangles);
	
	/* Merge the lists of complete and fragmented triangles, removing duplicates: */
	CardList::const_iterator tiIt=triangleIndices.begin();
	TriangleFragmentList::const_iterator tfIt=triangleFragments.begin();
	Card lastIndex=nil;
	while(tiIt!=triangleIndices.end()&&tfIt!=triangleFragments.end())
		{
		/* Get the smaller triangle index from the two lists: */
		Card nextIndex;
		if(*tiIt<=tfIt->originalIndex)
			{
			/* Grab the next complete triangle: */
			nextIndex=*tiIt;
			++tiIt;
			}
		else
			{
			/* Grab the next triangle fragment: */
			nextIndex=tfIt->originalIndex;
			++tfIt;
			}
		
		/* Add the next index to the list unless it's a duplicate: */
		if(nextIndex!=lastIndex)
			node.triangleIndices.push_back(nextIndex);
		lastIndex=nextIndex;
		}
	while(tiIt!=triangleIndices.end())
		{
		/* Add the next complete triangle to the list unless it's a duplicate: */
		if(*tiIt!=lastIndex)
			node.triangleIndices.push_back(*tiIt);
		lastIndex=*tiIt;
		++tiIt;
		}
	while(tfIt!=triangleFragments.end())
		{
		/* Add the next triangle fragment to the list unless it's a duplicate: */
		if(tfIt->originalIndex!=lastIndex)
			node.triangleIndices.push_back(tfIt->originalIndex);
		lastIndex=tfIt->originalIndex;
		++tfIt;
		}
	}

void
TriangleKdTree::intersectNode(
	const TriangleKdTree::Node& node,
	const TriangleKdTree::Point& p0,
	const TriangleKdTree::Point& p1,
	TriangleKdTree::IntersectResult& result) const
	{
	// DEBUGGING
	// ++numTraversedNodes;
	
	/* Check if the node is a leaf: */
	if(node.children==0)
		{
		/* Intersect the ray segment with all triangles in the node: */
		Point firstIntersection=p1;
		Card firstIndex=nil;
		Vector firstNormal;
		for(CardList::const_iterator tiIt=node.triangleIndices.begin();tiIt!=node.triangleIndices.end();++tiIt)
			{
			// DEBUGGING
			// ++numTestedTriangles;
			
			const Vertex* vPtr=&vertices[*tiIt];
			
			/* Intersect the ray with the triangle's plane: */
			Vector normal=Geometry::cross(vPtr[1].position-vPtr[0].position,vPtr[2].position-vPtr[0].position);
			Scalar offset=normal*vPtr[0].position;
			Scalar d0=normal*p0;
			Scalar d1=normal*firstIntersection;
			if((d0<=offset&&d1>offset)||(d0>=offset&&d1<offset))
				{
				/* Check whether the intersection point lies within the triangle: */
				Point intersection=Geometry::affineCombination(p0,firstIntersection,(offset-d0)/(d1-d0));
				bool inside=true;
				for(int i=0;i<3&&inside;++i)
					{
					Vector edgeNormal=Geometry::cross(normal,vPtr[(i+1)%3].position-vPtr[i].position);
					Scalar edgeOffset=edgeNormal*vPtr[i].position;
					inside=intersection*edgeNormal>=edgeOffset;
					}
				if(inside)
					{
					/* Update the first intersection: */
					firstIntersection=intersection;
					firstIndex=*tiIt;
					firstNormal=normal;
					}
				}
			}
		
		if(firstIndex!=nil)
			{
			/* Update the intersection result: */
			result.intersection=firstIntersection;
			result.triangleIndex=firstIndex;
			result.normal=firstNormal;
			}
		}
	else
		{
		/* Check the node's subtrees recursively: */
		if(p0[node.splitDimension]<=node.plane)
			{
			if(p1[node.splitDimension]<=node.plane)
				{
				/* Intersect with the front node only: */
				intersectNode(node.children[0],p0,p1,result);
				}
			else
				{
				/* Intersect the ray with the splitting plane: */
				Point plane=Geometry::affineCombination(p0,p1,(node.plane-p0[node.splitDimension])/(p1[node.splitDimension]-p0[node.splitDimension]));
				plane[node.splitDimension]=node.plane;
				
				/* Intersect with the front node first: */
				intersectNode(node.children[0],p0,plane,result);
				if(result.triangleIndex==nil)
					{
					/* Intersect with the back node: */
					intersectNode(node.children[1],plane,p1,result);
					}
				}
			}
		else
			{
			if(p1[node.splitDimension]>=node.plane)
				{
				/* Intersect with the back node only: */
				intersectNode(node.children[1],p0,p1,result);
				}
			else
				{
				/* Intersect the ray with the splitting plane: */
				Point plane=Geometry::affineCombination(p0,p1,(node.plane-p0[node.splitDimension])/(p1[node.splitDimension]-p0[node.splitDimension]));
				plane[node.splitDimension]=node.plane;
				
				/* Intersect with the back node first: */
				intersectNode(node.children[1],p0,plane,result);
				if(result.triangleIndex==nil)
					{
					/* Intersect with the front node: */
					intersectNode(node.children[0],plane,p1,result);
					}
				}
			}
		}
	}

void
TriangleKdTree::getTrianglesInBox(
	const TriangleKdTree::Node& node,
	const TriangleKdTree::Box& box,
	TriangleKdTree::CardList& triangleIndices) const
	{
	/* Check if the node is a leaf node: */
	if(node.children==0)
		{
		/*******************************************************************
		Use merge sort to test all triangles from this node's list that are
		not already in the given list against the box, and insert those that
		intersect.
		*******************************************************************/
		
		/* Create a new result list and reserve enough space: */
		CardList newTriangleIndices;
		newTriangleIndices.reserve(triangleIndices.size()+node.triangleIndices.size());
		
		/* Merge the two lists: */
		CardList::iterator ti1It=triangleIndices.begin();
		CardList::const_iterator ti2It=node.triangleIndices.begin();
		while(ti1It!=triangleIndices.end()&&ti2It!=node.triangleIndices.end())
			{
			if(*ti1It<=*ti2It)
				{
				/* Copy the triangle index from the given list: */
				newTriangleIndices.push_back(*ti1It);
				
				/* Check if the next node triangle is the same: */
				if(*ti1It==*ti2It)
					{
					/* No need to test the duplicate node triangle; skip it: */
					++ti2It;
					}
				
				/* Go to the next triangle: */
				++ti1It;
				}
			else
				{
				/***************************************************************
				Check if the node triangle intersects the box:
				***************************************************************/
				
				/* Get the triangle's vertices: */
				const Vertex* vPtr=&vertices[*ti2It];
				Point triangle[3];
				for(int i=0;i<3;++i)
					triangle[i]=Point(vPtr[i].position);
				
				/* Add it if it intersects the box: */
				if(Geometry::doesTriangleIntersectBox(box,triangle))
					newTriangleIndices.push_back(*ti2It);
				
				/* Go to the next node triangle: */
				++ti2It;
				}
			}
		
		/* Copy leftover triangles from the given list: */
		while(ti1It!=triangleIndices.end())
			{
			newTriangleIndices.push_back(*ti1It);
			++ti1It;
			}
		
		/* Test leftover triangles from the node's list: */
		while(ti2It!=node.triangleIndices.end())
			{
			/***************************************************************
			Check if the node triangle intersects the box:
			***************************************************************/
			
			/* Get the triangle's vertices: */
			const Vertex* vPtr=&vertices[*ti2It];
			Point triangle[3];
			for(int i=0;i<3;++i)
				triangle[i]=Point(vPtr[i].position);
			
			/* Add it if it intersects the box: */
			if(Geometry::doesTriangleIntersectBox(box,triangle))
				newTriangleIndices.push_back(*ti2It);
			
			/* Go to the next node triangle: */
			++ti2It;
			}
		
		/* Swap the new and old triangle lists: */
		std::swap(triangleIndices,newTriangleIndices);
		}
	else
		{
		/* Recurse into the node's children: */
		if(box.min[node.splitDimension]<node.plane)
			getTrianglesInBox(node.children[0],box,triangleIndices);
		if(box.max[node.splitDimension]>=node.plane)
			getTrianglesInBox(node.children[1],box,triangleIndices);
		}
	}

void
TriangleKdTree::drawIntersectionNode(
	const TriangleKdTree::Node& node,
	const TriangleKdTree::Box& domain,
	const TriangleKdTree::Point& p0,
	const TriangleKdTree::Point& p1,
	bool drawTriangles) const
	{
	if(node.children==0)
		{
		if(drawTriangles)
			{
			/* Draw the leaf node's triangles: */
			glBegin(GL_TRIANGLES);
			for(CardList::const_iterator tIt=node.triangleIndices.begin();tIt!=node.triangleIndices.end();++tIt)
				{
				const Vertex* vPtr=&vertices[*tIt];
				Vector normal=Geometry::cross(vPtr[1].position-vPtr[0].position,vPtr[2].position-vPtr[0].position);
				normal.normalize();
				glNormal(normal);
				glVertex(vPtr[0].position);
				glVertex(vPtr[1].position);
				glVertex(vPtr[2].position);
				}
			glEnd();
			}
		else
			{
			/* Draw the leaf node's domain: */
			glBegin(GL_QUADS);
			
			/* Draw inside faces first: */
			glNormal3f(1.0f,0.0f,0.0f);
			glVertex(domain.getVertex(0));
			glVertex(domain.getVertex(2));
			glVertex(domain.getVertex(6));
			glVertex(domain.getVertex(4));
			glNormal3f(-1.0f,0.0f,0.0f);
			glVertex(domain.getVertex(1));
			glVertex(domain.getVertex(5));
			glVertex(domain.getVertex(7));
			glVertex(domain.getVertex(3));
			glNormal3f(0.0f,1.0f,0.0f);
			glVertex(domain.getVertex(0));
			glVertex(domain.getVertex(4));
			glVertex(domain.getVertex(5));
			glVertex(domain.getVertex(1));
			glNormal3f(0.0f,-1.0f,0.0f);
			glVertex(domain.getVertex(2));
			glVertex(domain.getVertex(3));
			glVertex(domain.getVertex(7));
			glVertex(domain.getVertex(6));
			glNormal3f(0.0f,0.0f,1.0f);
			glVertex(domain.getVertex(0));
			glVertex(domain.getVertex(1));
			glVertex(domain.getVertex(3));
			glVertex(domain.getVertex(2));
			glNormal3f(0.0f,0.0f,-1.0f);
			glVertex(domain.getVertex(4));
			glVertex(domain.getVertex(6));
			glVertex(domain.getVertex(7));
			glVertex(domain.getVertex(5));
			
			/* Draw outside faces second: */
			glNormal3f(-1.0f,0.0f,0.0f);
			glVertex(domain.getVertex(0));
			glVertex(domain.getVertex(4));
			glVertex(domain.getVertex(6));
			glVertex(domain.getVertex(2));
			glNormal3f(1.0f,0.0f,0.0f);
			glVertex(domain.getVertex(1));
			glVertex(domain.getVertex(3));
			glVertex(domain.getVertex(7));
			glVertex(domain.getVertex(5));
			glNormal3f(0.0f,-1.0f,0.0f);
			glVertex(domain.getVertex(0));
			glVertex(domain.getVertex(1));
			glVertex(domain.getVertex(5));
			glVertex(domain.getVertex(4));
			glNormal3f(0.0f,1.0f,0.0f);
			glVertex(domain.getVertex(2));
			glVertex(domain.getVertex(6));
			glVertex(domain.getVertex(7));
			glVertex(domain.getVertex(3));
			glNormal3f(0.0f,0.0f,-1.0f);
			glVertex(domain.getVertex(0));
			glVertex(domain.getVertex(2));
			glVertex(domain.getVertex(3));
			glVertex(domain.getVertex(1));
			glNormal3f(0.0f,0.0f,1.0f);
			glVertex(domain.getVertex(4));
			glVertex(domain.getVertex(5));
			glVertex(domain.getVertex(7));
			glVertex(domain.getVertex(6));
			
			glEnd();
			}
		}
	else
		{
		/* Check the node's subtrees recursively: */
		if(p0[node.splitDimension]<=node.plane)
			{
			if(p1[node.splitDimension]<=node.plane)
				{
				/* Intersect with the front node only: */
				Box subDomain=domain;
				subDomain.max[node.splitDimension]=node.plane;
				drawIntersectionNode(node.children[0],subDomain,p0,p1,drawTriangles);
				}
			else
				{
				/* Intersect the ray with the splitting plane: */
				Point plane=Geometry::affineCombination(p0,p1,(node.plane-p0[node.splitDimension])/(p1[node.splitDimension]-p0[node.splitDimension]));
				plane[node.splitDimension]=node.plane;
				
				/* Intersect with the front node first: */
				Box subDomain=domain;
				subDomain.max[node.splitDimension]=node.plane;
				drawIntersectionNode(node.children[0],subDomain,p0,plane,drawTriangles);
				
				/* Intersect with the back node: */
				subDomain=domain;
				subDomain.min[node.splitDimension]=node.plane;
				drawIntersectionNode(node.children[1],subDomain,plane,p1,drawTriangles);
				}
			}
		else
			{
			if(p1[node.splitDimension]>=node.plane)
				{
				/* Intersect with the back node only: */
				Box subDomain=domain;
				subDomain.min[node.splitDimension]=node.plane;
				drawIntersectionNode(node.children[1],subDomain,p0,p1,drawTriangles);
				}
			else
				{
				/* Intersect the ray with the splitting plane: */
				Point plane=Geometry::affineCombination(p0,p1,(node.plane-p0[node.splitDimension])/(p1[node.splitDimension]-p0[node.splitDimension]));
				plane[node.splitDimension]=node.plane;
				
				/* Intersect with the back node first: */
				Box subDomain=domain;
				subDomain.min[node.splitDimension]=node.plane;
				drawIntersectionNode(node.children[1],subDomain,p0,plane,drawTriangles);
				
				/* Intersect with the front node: */
				subDomain=domain;
				subDomain.max[node.splitDimension]=node.plane;
				drawIntersectionNode(node.children[0],subDomain,plane,p1,drawTriangles);
				}
			}
		}
	}

TriangleKdTree::TriangleKdTree(
	const TriangleKdTree::VertexList& sVertices)
	:vertices(sVertices)
	{
	}

void
TriangleKdTree::createTree(
	const TriangleKdTree::Box& sBoundingBox,
	TriangleKdTree::Card sMaxTrianglesPerNode,
	const TriangleKdTree::CardList& triangleIndices)
	{
	// DEBUGGING
	// std::cout<<"Creating kd-tree for "<<triangleIndices.size()<<" triangles"<<std::endl;
	
	/* Store tree creation parameters: */
	boundingBox=sBoundingBox;
	maxTrianglesPerNode=sMaxTrianglesPerNode;
	
	/* Extend the bounding box slightly outwards: */
	for(int i=0;i<3;++i)
		{
		boundingBox.min[i]=decrement(boundingBox.min[i]);
		boundingBox.max[i]=increment(boundingBox.max[i]);
		}
	
	/* Initialize the kd-tree: */
	TriangleFragmentList triangleFragments;
	root.splitDimension=0;
	initNode(root,boundingBox,triangleIndices,triangleFragments);
	}

TriangleKdTree::IntersectResult
TriangleKdTree::intersect(
	const TriangleKdTree::Point& p0,
	const TriangleKdTree::Point& p1) const
	{
	// DEBUGGING
	// numTestedTriangles=0;
	// numTraversedNodes=0;
	
	/* Initialize the intersection result: */
	IntersectResult result;
	result.triangleIndex=nil;
	
	/* Intersect with the root node: */
	intersectNode(root,p0,p1,result);
	
	// DEBUGGING
	// std::cout<<"Traversed "<<numTraversedNodes<<" nodes and tested "<<numTestedTriangles<<" triangles"<<std::endl;
	
	return result;
	}

TriangleKdTree::Scalar
TriangleKdTree::traceBox(
	const TriangleKdTree::Box& box,
	const TriangleKdTree::Vector& displacement,
	TriangleKdTree::Vector& hitNormal) const
	{
	/* Catch the trivial case of no displacement: */
	if(displacement==Vector::zero)
		return Scalar(1);
	
	/* Create a bounding box around the original and displaced box: */
	Box bound=box;
	for(int i=0;i<3;++i)
		{
		if(displacement[i]>=Scalar(0))
			bound.max[i]+=displacement[i];
		else
			bound.min[i]+=displacement[i];
		}
	
	/* Retrieve the set of triangles intersecting the bounding box: */
	CardList triangles;
	getTrianglesInBox(root,bound,triangles);
	
	/* Process all triangles and find the first intersection: */
	Scalar lambdaMin(1);
	for(CardList::iterator tIt=triangles.begin();tIt!=triangles.end();++tIt)
		{
		/* Retrieve the triangle's vertices: */
		const Vertex* tvPtr=&vertices[*tIt];
		const Point* t[3];
		for(int i=0;i<3;++i)
			t[i]=&tvPtr[i].position;
		
		/* Calculate the triangle's plane equation: */
		Vector tNormal=Geometry::cross((*t[1])-(*t[0]),(*t[2])-(*t[1]));
		Scalar tOffset=tNormal*(*t[1]);
		
		/*******************************************************************
		Find the one box vertex that would hit the triangle's face first,
		by inspecting the relative orientations of the face normal and the
		displacement vector.
		*******************************************************************/
		
		/* Calculate the intercept equation's denominator: */
		Scalar vDenom=displacement*tNormal;
		
		/* Check if the displacement vector and the triangle's face are not coplanar: */
		if(vDenom!=Scalar(0))
			{
			/* Find the box vertex that will hit the triangle face first: */
			Point v=box.min;
			for(int axis=0;axis<3;++axis)
				{
				/* Move along the axis if doing so will move closer to the triangle's plane: */
				if(tNormal[axis]*vDenom>Scalar(0)) // Check if the two factors have the same sign
					{
					/* Move along the axis: */
					v[axis]=box.max[axis];
					}
				}
			
			/* Calculate the vector intercept of the closest box corner to the triangle's face: */
			Scalar vLambda=(tOffset-v*tNormal)/vDenom;
			
			/* Check if the box can reach the triangle within the current reduced displacement: */
			if(vLambda>=lambdaMin)
				{
				/* Trivially reject this triangle: */
				goto triangleDone;
				}
			else if(vLambda>=Scalar(0))
				{
				/* Check if the computed intersection point is inside the triangle: */
				Point ip=v+displacement*vLambda;
				if(Geometry::isPointInsideTriangle(*t[0],*t[1],*t[2],tNormal,ip))
					{
					/* Box vertex hits face; mark the intersection: */
					lambdaMin=vLambda;
					hitNormal=tNormal;
					
					/* Done testing this triangle: */
					goto triangleDone;
					}
				}
			}
		
		/*******************************************************************
		Intersect rays starting at the triangle's vertices with the box:
		*******************************************************************/
		
		for(int vertexIndex=0;vertexIndex<3;++vertexIndex)
			{
			/* Calculate the intersection interval between the triangle's vertex ray and the box: */
			Scalar maxEntry(0);
			Scalar minExit=Math::Constants<Scalar>::max;
			int entryAxis=-1;
			for(int axis=0;axis<3;++axis)
				{
				/* Check the position of the triangle vertex relative to the box: */
				if((*t[vertexIndex])[axis]>=box.max[axis])
					{
					if(displacement[axis]>Scalar(0))
						{
						/* Ray enters box through max face: */
						Scalar entry=((*t[vertexIndex])[axis]-box.max[axis])/displacement[axis];
						if(maxEntry<entry)
							{
							maxEntry=entry;
							entryAxis=axis;
							}
						}
					else
						{
						/* Reject the ray; misses box: */
						minExit=Scalar(-1);
						break;
						}
					}
				if((*t[vertexIndex])[axis]<=box.min[axis])
					{
					if(displacement[axis]<Scalar(0))
						{
						/* Ray enters box through min face: */
						Scalar entry=((*t[vertexIndex])[axis]-box.min[axis])/displacement[axis];
						if(maxEntry<entry)
							{
							maxEntry=entry;
							entryAxis=axis;
							}
						}
					else
						{
						/* Reject the ray; misses box: */
						minExit=Scalar(-1);
						break;
						}
					}
				if(displacement[axis]<Scalar(0))
					{
					/* Ray exits box through max face: */
					Scalar exit=((*t[vertexIndex])[axis]-box.max[axis])/displacement[axis];
					if(minExit>exit)
						minExit=exit;
					}
				if(displacement[axis]>Scalar(0))
					{
					/* Ray exits box through min face: */
					Scalar exit=((*t[vertexIndex])[axis]-box.min[axis])/displacement[axis];
					if(minExit>exit)
						minExit=exit;
					}
				
				/* Bail out if the interval has become invalid: */
				if(maxEntry>minExit)
					break;
				}
			
			if(maxEntry<=minExit&&lambdaMin>maxEntry)
				{
				/* Reject the entire triangle if the vertex is inside the box: */
				if(entryAxis<0)
					goto triangleDone;
				
				/* Triangle vertex hits box; mark the intersection: */
				lambdaMin=maxEntry;
				
				/* Assign the hit normal vector: */
				hitNormal=Vector::zero;
				hitNormal[entryAxis]=Scalar(1);
				}
			}
		
		/*******************************************************************
		Intersect the triangle's edges with the box's edges:
		*******************************************************************/
		
		{
		int tv0=2;
		for(int tv1=0;tv1<3;tv0=tv1,++tv1)
			{
			/* Only consider the edge if its vertices intersect the box on different faces (box faces are convex): */
			// if(vertexEntryFaces[tv0]!=vertexEntryFaces[tv1])
				{
				/* Check against each of the three primary-axis edge bundles of the box: */
				for(int axis=0;axis<3;++axis)
					{
					/* Calculate the bi-orthogonal vector between the box and triangle edge: */
					Vector edge=(*t[tv1])-(*t[tv0]);
					Vector bio;
					switch(axis)
						{
						case 0:
							bio=Vector(0,-edge[2],edge[1]);
							break;
						
						case 1:
							bio=Vector(edge[2],0,-edge[0]);
							break;
						
						case 2:
							bio=Vector(-edge[1],edge[0],0);
							break;
						}
					
					/* Intersect the triangle edge with the box edges parallel to the primary axis: */
					Scalar eDenom=bio*displacement;
					if(eDenom!=Scalar(0))
						{
						/* Find the box edge along the current axis that will hit the triangle edge first: */
						Point e0=box.min;
						for(int i=1;i<3;++i)
							{
							/* Find the index of the secondary axis: */
							int axis1=(axis+i)%3;
							if(bio[axis1]*eDenom>Scalar(0))
								e0[axis1]=box.max[axis1];
							}
						
						/* Calculate the intercept for the edge going through the box's min vertex: */
						Scalar eLambda=(bio*(*t[tv0])-bio*e0)/eDenom;
						
						/* If the intercept is smaller than the current minimum, check if the edges actually hit: */
						if(eLambda>=Scalar(0)&&lambdaMin>eLambda)
							{
							/* Displace the box edge to the intercept position: */
							e0+=displacement*eLambda;
							
							/* Check if the triangle edge's vertices are on different sides of the box edge: */
							Vector beNormal=edge;
							beNormal[axis]=Scalar(0);
							Scalar beOffset=beNormal*e0;
							if((beNormal*(*t[tv0])-beOffset)*(beNormal*(*t[tv1])-beOffset)<=Scalar(0))
								{
								/* Check if the box edge's vertices are on different sides of the triangle edge: */
								Vector teNormal=Geometry::cross(bio,edge);
								Scalar teOffset=teNormal*(*t[tv0]);
								Scalar e0d=teNormal*e0-teOffset;
								Scalar e1d=e0d+teNormal[axis]*(box.max[axis]-box.min[axis]);
								if(e0d*e1d<=Scalar(0))
									{
									/* It's a hit; mark it: */
									lambdaMin=eLambda;
									hitNormal=bio;
									}
								}
							}
						}
					}
				}
			}
		}
		
		/* Done processing this triangle: */
		triangleDone:
		;
		}
	
	/* Return the first intercept: */
	return lambdaMin;
	}

void
TriangleKdTree::drawIntersection(
	const TriangleKdTree::Point& p0,
	const TriangleKdTree::Point& p1,
	bool drawTriangles) const
	{
	drawIntersectionNode(root,boundingBox,p0,p1,drawTriangles);
	}
