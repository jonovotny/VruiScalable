/***********************************************************************
PolygonMesh - Class to represent meshes of planar convex polygons. Meant
as a temporary helper structure to convert polygon soup into an
efficiently rendered representation, and compute a full set of vertex
attributes.
Copyright (c) 2009-2012 Oliver Kreylos
***********************************************************************/

#define POLYGONMESH_IMPLEMENTATION

#include "PolygonMesh.h"

// DEBUGGING
#include <iostream>

#include <utility>
#include <Misc/SelfDestructPointer.h>
#include <Misc/ThrowStdErr.h>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <GL/gl.h>
#include <GL/GLGeometryWrappers.h>

#include "TexCoordCalculator.h"
#include "LineSet.h"
#include "Tesselator.h"

/************************************
Static elements of class PolygonMesh:
************************************/

template <class MeshVertexParam>
const typename PolygonMesh<MeshVertexParam>::Card PolygonMesh<MeshVertexParam>::invalidIndex=~typename PolygonMesh<MeshVertexParam>::Card(0);

/****************************
Methods of class PolygonMesh:
****************************/

template <class MeshVertexParam>
inline
void
PolygonMesh<MeshVertexParam>::connectFace(
	typename PolygonMesh<MeshVertexParam>::Card faceIndex)
	{
	/* Get a handle to the face: */
	Face& face=faces[faceIndex];
	
	#if 0
	
	/* Check if the face already exists: */
	{
	bool allEdgesThere=true;
	bool allEdgesSameFace=true;
	Card allEdgesFaceIndex=invalidIndex;
	Card e0=faceVertexIndices[face.firstVertexIndex+face.numVertices-1];
	for(Card i=0;i<face.numVertices;++i)
		{
		Card e1=faceVertexIndices[face.firstVertexIndex+i];
		typename FaceEdgeHasher::Iterator feIt=faceEdges.findEntry(DirectedEdge(e0,e1));
		if(feIt.isFinished())
			{
			allEdgesThere=false;
			break;
			}
		else if(allEdgesFaceIndex==invalidIndex)
			allEdgesFaceIndex=feIt->getDest().faceIndex;
		else if(allEdgesFaceIndex!=feIt->getDest().faceIndex)
			{
			allEdgesSameFace=false;
			break;
			}
		
		e0=e1;
		}
	if(allEdgesThere&&allEdgesSameFace)
		{
		/* Discard the face: */
		for(Card i=0;i<face.numVertices;++i)
			faceVertexIndices.pop_back();
		faces.pop_back();
		return;
		}
	}
	
	#endif
	
	// DEBUGGING start
	#if 0
	
	/* Find if it's a watched face: */
	{
	bool isWatched=false;
	for(Card i=0;i<face.numVertices;++i)
		if(faceVertexIndices[face.firstVertexIndex+i]==13022)
			{
			isWatched=true;
			break;
			}
	if(isWatched)
		{
		std::cout<<"Face "<<faceIndex<<":";
		for(Card i=0;i<face.numVertices;++i)
			std::cout<<" "<<faceVertexIndices[face.firstVertexIndex+i];
		std::cout<<std::endl;
		}
	}
	
	#endif
	// DEBUGGING end
	
	/*********************************************************************
	Calculate the face's normal vector, and check for non-convex faces at
	the same time.
	*********************************************************************/
	
	face.convex=true;
	
	/* Get the last corner's normal; it's convex by agreement: */
	const MeshVertex* v0=&vertices[faceVertexIndices[face.firstVertexIndex+face.numVertices-2]];
	const MeshVertex* v1=&vertices[faceVertexIndices[face.firstVertexIndex+face.numVertices-1]];
	Vector d0=v1->position-v0->position;
	const MeshVertex* v2=&vertices[faceVertexIndices[face.firstVertexIndex+0]];
	Vector d1=v2->position-v1->position;
	face.normal=Geometry::cross(d0,d1);
	
	if(face.numVertices>3)
		{
		/* Calculate corner normal vectors around the face, and check for "concave" corners: */
		Vector baseNormal=face.normal;
		v1=v2;
		d0=d1;
		for(Card i=1;i<face.numVertices;++i)
			{
			v2=&vertices[faceVertexIndices[face.firstVertexIndex+i]];
			d1=v2->position-v1->position;
			
			/* Calculate the corner normal, and if it goes against the current accumulated normal, there must be a concave corner somewhere: */
			Vector cornerNormal=Geometry::cross(d0,d1);
			if(cornerNormal*baseNormal<Scalar(0))
				{
				cornerNormal=-cornerNormal;
				face.convex=false;
				}
			
			/* Accumulate the face normal: */
			face.normal+=cornerNormal;
			
			v1=v2;
			d0=d1;
			}
		}
	
	/* Normalize the final face normal vector if it is non-zero: */
	if(Geometry::sqr(face.normal)!=Scalar(0))
		face.normal.normalize();
	
	/* Check the face for any problematic winged or T-edges: */
	{
	Card i0=face.numVertices-1;
	Card e0=faceVertexIndices[face.firstVertexIndex+i0];
	for(Card i1=0;i1<face.numVertices;++i1)
		{
		Card e1=faceVertexIndices[face.firstVertexIndex+i1];
		
		/* Look for the directed edge in the face edge hash table: */
		if(faceEdges.isEntry(DirectedEdge(e0,e1)))
			{
			/*****************************************************************
			We have a winged or T-edge; this is bad and needs to be handled
			properly. For now, we punt by creating two new vertices, and
			changing the new face's vertex list. The mesh might get ugly, but
			at least it will be consistent.
			*****************************************************************/
			
			// DEBUGGING
			// std::cout<<"Detected T-edge; cloning vertices "<<e0<<" and "<<e1<<std::endl;
			
			/* Clone the vertices e0 and e1: */
			faceVertexIndices[face.firstVertexIndex+i0]=vertices.size();
			vertices.push_back(vertices[e0]);
			vertexEdges.push_back(invalidIndex);
			vertexCreaseFlags.push_back(false);
			faceVertexIndices[face.firstVertexIndex+i1]=vertices.size();
			vertices.push_back(vertices[e1]);
			vertexEdges.push_back(invalidIndex);
			vertexCreaseFlags.push_back(false);
			
			e1=faceVertexIndices[face.firstVertexIndex+i1];
			}
		
		i0=i1;
		e0=e1;
		}
	}
	
	/* Connect the face to all existing faces in the mesh: */
	FaceEdge faceEdge;
	faceEdge.faceIndex=faceIndex;
	faceEdge.previousVertexIndex=faceVertexIndices[face.firstVertexIndex+face.numVertices-2];
	Card e0=faceVertexIndices[face.firstVertexIndex+face.numVertices-1];
	for(Card i=0;i<face.numVertices;++i)
		{
		/* Create a directed edge: */
		Card e1=faceVertexIndices[face.firstVertexIndex+i];
		DirectedEdge edge(e0,e1);
		
		/* Add the directed edge to the face edge hash table: */
		faceEdges.setEntry(typename FaceEdgeHasher::Entry(edge,faceEdge));
		
		/* Check if the outgoing edge of the edge's start vertex needs to be updated: */
		if(vertexEdges[e0]==invalidIndex)
			{
			/* Vertex has no edges yet; just store this one: */
			vertexEdges[e0]=e1;
			}
		else if(vertexEdges[e0]==faceEdge.previousVertexIndex)
			{
			/*****************************************************************
			We just violated the invariant that e0's start edge is a boundary
			edge, so we need to search through e0's platelet clockwise until
			we find one, or complete a full circle.
			*****************************************************************/
			
			/* Start looking at the other side of the new face: */
			Card firstEdge=e1;
			typename FaceEdgeHasher::Iterator feIt=faceEdges.findEntry(DirectedEdge(firstEdge,e0));
			
			/* Loop until the current start edge candidate is a boundary edge: */
			while(!feIt.isFinished())
				{
				/* Find the face on the other side of the start edge candidate: */
				Face& face=faces[feIt->getDest().faceIndex];
				
				/* Search for firstedge followed by e0 in the face's vertices: */
				Card fe0=faceVertexIndices[face.firstVertexIndex+face.numVertices-2];
				Card fe1=faceVertexIndices[face.firstVertexIndex+face.numVertices-1];
				for(Card i=0;i<face.numVertices;++i)
					{
					Card fe2=faceVertexIndices[face.firstVertexIndex+i];
					if(fe0==firstEdge&&fe1==e0)
						{
						firstEdge=fe2;
						break;
						}
					fe0=fe1;
					fe1=fe2;
					}
				
				/* Bail out if a full circle was completed: */
				if(firstEdge==faceEdge.previousVertexIndex)
					break;
				
				/* Get the next face edge: */
				feIt=faceEdges.findEntry(DirectedEdge(firstEdge,e0));
				}
			
			/* Store the current start edge candidate: */
			vertexEdges[e0]=firstEdge;
			}
		
		/* Go to the next face edge: */
		faceEdge.previousVertexIndex=e0;
		e0=e1;
		}
	}

template <class MeshVertexParam>
inline
void
PolygonMesh<MeshVertexParam>::triangulateFace(
	typename PolygonMesh<MeshVertexParam>::Card faceIndex,
	std::vector<typename PolygonMesh<MeshVertexParam>::Card>& triangleVertexIndices) const
	{
	/* Get a handle to the face: */
	const Face& face=faces[faceIndex];
	
	if(face.convex)
		{
		/******************************
		Trivially triangulate the face:
		******************************/
		
		/* Get the first two triangle vertices: */
		Card v0=faceVertexIndices[face.firstVertexIndex+0];
		Card v1=faceVertexIndices[face.firstVertexIndex+1];
		
		/* Generate numVertices-2 triangles: */
		for(Card i=2;i<face.numVertices;++i)
			{
			/* Get the third triangle vertex: */
			Card v2=faceVertexIndices[face.firstVertexIndex+i];
			
			/* Store the triangle: */
			triangleVertexIndices.push_back(v0);
			triangleVertexIndices.push_back(v1);
			triangleVertexIndices.push_back(v2);
			
			/* Go to the next triangle: */
			v1=v2;
			}
		}
	else
		{
		/*****************************************************
		Triangulate the face using a Tesselator helper object:
		*****************************************************/
		
		
		
		
		/*******************************************************
		Triangulate the face using Kong's ear-cutting algorithm:
		*******************************************************/
		
		#if 0
		
		/* Find the face's concave corners: */
		std::vector<Card> vertexIndices;
		std::vector<Vector> vertexNormals;
		std::vector<bool> concaveFlags;
		Card numConcaveCorners=0;
		{
		Card v0=faceVertexIndices[face.firstVertexIndex+face.numVertices-2];
		Card v1=faceVertexIndices[face.firstVertexIndex+face.numVertices-1];
		Vector d0=vertices[v1].position-vertices[v0].position;
		for(Card i=0;i<face.numVertices;++i)
			{
			vertexIndices.push_back(v1);
			Card v2=faceVertexIndices[face.firstVertexIndex+i];
			
			/* Check if the corner is concave: */
			Vector d1=vertices[v2].position-vertices[v1].position;
			Vector vertexNormal=Geometry::cross(d0,d1);
			vertexNormals.push_back(vertexNormal);
			bool concave=vertexNormal*face.normal<Scalar(0);
			if(concave)
				++numConcaveCorners;
			concaveFlags.push_back(concave);
			
			v1=v2;
			d0=d1;
			}
		}
		
		// DEBUGGING
		//std::cout<<"Triangulating non-convex face with "<<face.numVertices<<" vertices and "<<numConcaveCorners<<" concave corners";
		//for(Card i=0;i<face.numVertices;++i)
		//	if(concaveFlags[i])
		//		std::cout<<" "<<i;
		//std::cout<<std::endl;
		
		/* Cut off and render "ear" triangles that do not contain concave corners until only a single triangle remains: */
		{
		Card numVertices=face.numVertices;
		Card i0=numVertices-2;
		Card v0=vertexIndices[i0];
		Card i1=numVertices-1;
		Card v1=vertexIndices[i1];
		Card i2=0;
		Card v2=vertexIndices[0];
		while(numVertices>3)
			{
			/* Find an ear triangle: */
			for(Card i=0;i<numVertices;++i)
				{
				/* Check if the ear candidate is convex: */
				if(!concaveFlags[i1])
					{
					// DEBUGGING
					//std::cout<<"Checking candidate ear "<<i0<<", "<<i1<<", "<<i2<<std::endl;
					
					/* Check if the ear contains any concave corners: */
					Vector d0=vertices[v1].position-vertices[v0].position;
					Vector plane0Normal=Geometry::cross(vertexNormals[i1],d0);
					Scalar plane0Offset=Math::div2(plane0Normal*vertices[v0].position+plane0Normal*vertices[v1].position);
					Vector d1=vertices[v2].position-vertices[v1].position;
					Vector plane1Normal=Geometry::cross(vertexNormals[i1],d1);
					Scalar plane1Offset=Math::div2(plane1Normal*vertices[v1].position+plane1Normal*vertices[v2].position);
					Vector ear=vertices[v2].position-vertices[v0].position;
					Vector plane2Normal=Geometry::cross(ear,vertexNormals[i1]);
					Scalar plane2Offset=Math::div2(plane2Normal*vertices[v0].position+plane2Normal*vertices[v2].position);
					bool earClear=true;
					for(Card j=0;j<numVertices;++j)
						if(concaveFlags[j]&&j!=i0&&j!=i2)
							{
							const Point& p=vertices[vertexIndices[j]].position;
							if(plane0Normal*p>=plane0Offset&&plane1Normal*p>=plane1Offset&&plane2Normal*p>=plane2Offset)
								{
								// DEBUGGING
								//std::cout<<"Alas, contains concave corner "<<j<<std::endl;
								earClear=false;
								break;
								}
							}
					
					if(earClear)
						{
						/* Store the ear triangle: */
						triangleVertexIndices.push_back(v0);
						triangleVertexIndices.push_back(v1);
						triangleVertexIndices.push_back(v2);
						
						/* Cut off the ear: */
						// DEBUGGING
						//std::cout<<"Cutting off ear "<<i0<<", "<<i1<<", "<<i2<<std::endl;
						vertexIndices.erase(vertexIndices.begin()+i1);
						vertexNormals.erase(vertexNormals.begin()+i1);
						concaveFlags.erase(concaveFlags.begin()+i1);
						--numVertices;
						if(i0>i1)
							--i0;
						if(i2>i1)
							--i2;
						i1=i0;
						v1=v0;
						i0=(i1+numVertices-1)%numVertices;
						v0=vertexIndices[i0];
						
						/* Check if the ear's adjacent corners are no longer concave: */
						if(concaveFlags[i1])
							{
							Vector d0=vertices[v1].position-vertices[v0].position;
							Vector d1=vertices[v2].position-vertices[v1].position;
							Vector vertexNormal=Geometry::cross(d0,d1);
							if(vertexNormal*face.normal>=Scalar(0))
								{
								// DEBUGGING
								//std::cout<<"Corner "<<i1<<" no longer concave"<<std::endl;
								vertexNormals[i1]=vertexNormal;
								concaveFlags[i1]=false;
								--numConcaveCorners;
								}
							}
						if(concaveFlags[i2])
							{
							Card i3=(i2+1)%numVertices;
							Vector d0=vertices[v2].position-vertices[v1].position;
							Vector d1=vertices[vertexIndices[i3]].position-vertices[v2].position;
							Vector vertexNormal=Geometry::cross(d0,d1);
							if(vertexNormal*face.normal>=Scalar(0))
								{
								// DEBUGGING
								//std::cout<<"Corner "<<i2<<" no longer concave"<<std::endl;
								vertexNormals[i2]=vertexNormal;
								concaveFlags[i2]=false;
								--numConcaveCorners;
								}
							}
						goto cutOffEar; // Yay!
						}
					}
				
				/* Go to the next candidate ear: */
				i0=i1;
				v0=v1;
				i1=i2;
				v1=v2;
				i2=(i2+1)%numVertices;
				v2=vertexIndices[i2];
				}
			
			/* Just punt for now... */
			// DEBUGGING
			std::cout<<"No ear found in face "<<faceIndex<<", "<<numVertices<<" vertices and "<<numConcaveCorners<<" concave corners left"<<std::endl;
			break;
			// Misc::throwStdErr("No ear found! Inconceivable!");
			
			cutOffEar:
			;
			}
		
		/* Store the remaining triangle: */
		triangleVertexIndices.push_back(vertexIndices[0]);
		triangleVertexIndices.push_back(vertexIndices[1]);
		triangleVertexIndices.push_back(vertexIndices[2]);
		}
		
		#else
		
		/************************************************************
		Reduce the problem's dimensionality by projecting the polygon
		into the most closely aligned primary plane:
		************************************************************/
		
		/* Use the primary 2D plane that is orthogonal to the largest normal vector component: */
		int primaryAxis=Geometry::findParallelAxis(face.normal);
		int a0=(primaryAxis+1)%3;
		int a1=(primaryAxis+2)%3;
		if(face.normal[primaryAxis]<Scalar(0))
			{
			/* Flip the secondary axes to preserve the face's winding order: */
			std::swap(a0,a1);
			}
		
		/* Create lists of vertex indices and concavity flags: */
		std::vector<Card> vertexIndices;
		vertexIndices.reserve(face.numVertices);
		std::vector<bool> concaveFlags;
		concaveFlags.reserve(face.numVertices);
		std::vector<Card> concaveCorners;
		{
		Card v0=faceVertexIndices[face.firstVertexIndex+face.numVertices-2];
		Card v1=faceVertexIndices[face.firstVertexIndex+face.numVertices-1];
		Scalar d0x=vertices[v1].position[a0]-vertices[v0].position[a0];
		Scalar d0y=vertices[v1].position[a1]-vertices[v0].position[a1];
		for(unsigned int i=0;i<face.numVertices;++i)
			{
			/* Store the corner's vertex index in the temporary list: */
			vertexIndices.push_back(v1);
			
			/* Check if the corner is concave: */
			Card v2=faceVertexIndices[face.firstVertexIndex+i];
			Scalar d1x=vertices[v2].position[a0]-vertices[v1].position[a0];
			Scalar d1y=vertices[v2].position[a1]-vertices[v1].position[a1];
			bool concave=d0x*d1y-d0y*d1x<Scalar(0);
			if(concave)
				concaveCorners.push_back(v1);
			concaveFlags.push_back(concave);
			
			/* Go to next corner: */
			d0x=d1x;
			d0y=d1y;
			v1=v2;
			}
		}
		
		/* Cut off "ears" until there are no more concave corners: */
		Card nv=vertexIndices.size();
		Card i0=nv-1;
		Card v0=vertexIndices[i0];
		Card i1=0;
		Card v1=vertexIndices[i1];
		Card i2=1;
		Card v2=vertexIndices[i2];
		while(!concaveCorners.empty())
			{
			/* Find the next ear: */
			unsigned int numEarTrials=0;
			while(true)
				{
				/* Check if the current corner is an ear candidate: */
				if(!concaveFlags[i1]&&(concaveFlags[i0]||concaveFlags[i2])) // Corner is convex and next to at least one concave corner
					{
					/* Check if the current corner is an ear: */
					Scalar c0x=vertices[v0].position[a0];
					Scalar c0y=vertices[v0].position[a1];
					Scalar cx=vertices[v1].position[a0];
					Scalar cy=vertices[v1].position[a1];
					Scalar d0x=cx-c0x;
					Scalar d0y=cy-c0y;
					Scalar c2x=vertices[v2].position[a0];
					Scalar c2y=vertices[v2].position[a1];
					Scalar d1x=c2x-cx;
					Scalar d1y=c2y-cy;
					Scalar d2x=c0x-c2x;
					Scalar d2y=c0y-c2y;
					Scalar ex=Math::mid(c0x,c2x);
					Scalar ey=Math::mid(c0y,c2y);
					
					/* Test every concave corner that is not adjacent to the ear candidate: */
					bool earClear=true;
					for(std::vector<Card>::iterator ccIt=concaveCorners.begin();earClear&&ccIt!=concaveCorners.end();++ccIt)
						if(*ccIt!=v0&&*ccIt!=v2)
							{
							/* Get the concave corner's position: */
							Scalar ccx=vertices[*ccIt].position[a0];
							Scalar ccy=vertices[*ccIt].position[a1];
							
							/* Check if the concave corner is inside the ear candidate: */
							earClear=(ccx-cx)*-d0y+(ccy-cy)*d0x<Scalar(0)||(ccx-cx)*-d1y+(ccy-cy)*d1x<Scalar(0)||(ccx-ex)*-d2y+(ccy-ey)*d2x<Scalar(0);
							}
					
					/* Bail out if the ear is clear: */
					if(earClear)
						break;
					}
				
				/* Go to the next corner around the remaining vertex loop: */
				i0=i1;
				v0=v1;
				i1=i2;
				v1=v2;
				++i2;
				if(i2==nv)
					i2=0;
				v2=vertexIndices[i2];
				
				++numEarTrials;
				if(numEarTrials>nv)
					{
					std::cerr<<concaveCorners.size()<<" concave corners left, but no more ears found"<<std::endl;
					goto doneWithEars;
					}
				}
			
			/* Generate a triangle for the current ear corner: */
			triangleVertexIndices.push_back(v0);
			triangleVertexIndices.push_back(v1);
			triangleVertexIndices.push_back(v2);
			
			/* Cut off the ear: */
			vertexIndices.erase(vertexIndices.begin()+i1);
			concaveFlags.erase(concaveFlags.begin()+i1);
			--nv;
			if(i0>i1)
				--i0;
			if(i2>i1)
				--i2;
			
			/* Check if concave corners adjacent to the ear just became convex: */
			Scalar d1x=vertices[v2].position[a0]-vertices[v0].position[a0];
			Scalar d1y=vertices[v2].position[a1]-vertices[v0].position[a1];
			if(concaveFlags[i0])
				{
				Card il=(i0+nv-1)%nv;
				Card vl=vertexIndices[il];
				Scalar d0x=vertices[v0].position[a0]-vertices[vl].position[a0];
				Scalar d0y=vertices[v0].position[a1]-vertices[vl].position[a1];
				concaveFlags[i0]=d0x*d1y-d0y*d1x<Scalar(0);
				if(!concaveFlags[i0])
					{
					/* Remove the concave corner from the list: */
					for(std::vector<Card>::iterator ccIt=concaveCorners.begin();ccIt!=concaveCorners.end();++ccIt)
						if(*ccIt==v0)
							{
							concaveCorners.erase(ccIt);
							break;
							}
					}
				}
			if(concaveFlags[i2]);
				{
				Card ir=(i2+1)%nv;
				Card vr=vertexIndices[ir];
				Scalar d2x=vertices[vr].position[a0]-vertices[v2].position[a0];
				Scalar d2y=vertices[vr].position[a1]-vertices[v2].position[a1];
				concaveFlags[i2]=d1x*d2y-d1y*d2x<Scalar(0);
				if(!concaveFlags[i2])
					{
					/* Remove the concave corner from the list: */
					for(std::vector<Card>::iterator ccIt=concaveCorners.begin();ccIt!=concaveCorners.end();++ccIt)
						if(*ccIt==v2)
							{
							concaveCorners.erase(ccIt);
							break;
							}
					}
				}
			
			/* Set up the next corner to try: */
			i1=i0;
			v1=v0;
			i0=(i0+nv-1)%nv;
			v0=vertexIndices[i0];
			}
		
		doneWithEars:
		
		/* Tesselate the rest of the polygon trivially: */
		std::vector<Card>::iterator vi0=vertexIndices.begin();
		std::vector<Card>::iterator vi1=vi0+1;
		for(std::vector<Card>::iterator vi2=vi1+1;vi2!=vertexIndices.end();vi1=vi2,++vi2)
			{
			triangleVertexIndices.push_back(*vi0);
			triangleVertexIndices.push_back(*vi1);
			triangleVertexIndices.push_back(*vi2);
			}
		
		#endif
		}
	}

template <class MeshVertexParam>
inline
const typename PolygonMesh<MeshVertexParam>::TPoint&
PolygonMesh<MeshVertexParam>::getVertexTexCoord(
	typename PolygonMesh<MeshVertexParam>::Card faceIndex,
	typename PolygonMesh<MeshVertexParam>::Card vertexIndex) const
	{
	/* Return per-face texture coordinates for multi-surface vertices: */
	if(vertexMultiSurfaceFlags[vertexIndex])
		{
		/* Get the vertex' per-face texture coordinates: */
		typename FaceVertexTexCoordHasher::ConstIterator fvtcIt=vertexTexCoords.findEntry(FaceVertex(faceIndex,vertexIndex));
		if(fvtcIt.isFinished())
			{
			// DEBUGGING
			//std::cout<<"Missing per-face texture coordinates for face "<<faceIndex<<", vertex "<<vertexIndex<<std::endl;
			return vertices[vertexIndex].texCoord;
			}
		else
			return fvtcIt->getDest();
		}
	else
		return vertices[vertexIndex].texCoord;
	}

template <class MeshVertexParam>
inline
const typename PolygonMesh<MeshVertexParam>::Vector&
PolygonMesh<MeshVertexParam>::getVertexNormal(
	typename PolygonMesh<MeshVertexParam>::Card faceIndex,
	typename PolygonMesh<MeshVertexParam>::Card vertexIndex) const
	{
	/* Return a per-face normal vector for crease vertices: */
	if(vertexCreaseFlags[vertexIndex])
		{
		/* Get the vertex' per-face normal vector: */
		typename FaceVertexNormalHasher::ConstIterator fvnIt=vertexNormals.findEntry(FaceVertex(faceIndex,vertexIndex));
		if(fvnIt.isFinished())
			{
			// DEBUGGING
			//std::cout<<"Missing per-face vertex normal for face "<<faceIndex<<", vertex "<<vertexIndex<<std::endl;
			return vertices[vertexIndex].normal;
			}
		else
			return fvnIt->getDest();
		}
	else
		return vertices[vertexIndex].normal;
	}

template <class MeshVertexParam>
inline
typename PolygonMesh<MeshVertexParam>::Card
PolygonMesh<MeshVertexParam>::getTriangleVertexIndex(
	typename PolygonMesh<MeshVertexParam>::Card faceIndex,
	typename PolygonMesh<MeshVertexParam>::Card vertexIndex) const
	{
	/* Not implemented yet! */
	return invalidIndex;
	}

template <class MeshVertexParam>
inline
PolygonMesh<MeshVertexParam>::PolygonMesh(void)
	:numSurfaces(0),
	 faceEdges(101),
	 vertexTexCoords(101),
	 creaseEdges(101),
	 vertexNormals(101),
	 addingFace(false)
	{
	}

template <class MeshVertexParam>
inline
PolygonMesh<MeshVertexParam>::~PolygonMesh(void)
	{
	}

template <class MeshVertexParam>
inline
typename PolygonMesh<MeshVertexParam>::Card
PolygonMesh<MeshVertexParam>::addVertex(
	const typename PolygonMesh<MeshVertexParam>::MeshVertex& newVertex)
	{
	/* Store the new mesh vertex: */
	Card result=vertices.size();
	vertices.push_back(newVertex);
	vertexEdges.push_back(invalidIndex); // Vertex doesn't have any edges yet
	vertexMultiSurfaceFlags.push_back(false); // Not a multi-surface vertex, either
	vertexCreaseFlags.push_back(false); // Not a crease vertex, either
	
	return result;
	}

template <class MeshVertexParam>
inline
void
PolygonMesh<MeshVertexParam>::startFace(
	void)
	{
	if(addingFace)
		Misc::throwStdErr("PolygonMesh::startFace: Already adding face");
	
	/* Prepare adding a new face: */
	addingFace=true;
	newNumVertices=0;
	newFirstVertexIndex=faceVertexIndices.size();
	}

template <class MeshVertexParam>
inline
void
PolygonMesh<MeshVertexParam>::addFaceVertex(
	typename PolygonMesh<MeshVertexParam>::Card vertexIndex)
	{
	if(!addingFace)
		Misc::throwStdErr("PolygonMesh::addFaceVertex: Not adding face");
	
	/* Store the new vertex: */
	faceVertexIndices.push_back(vertexIndex);
	++newNumVertices;
	}

template <class MeshVertexParam>
inline
typename PolygonMesh<MeshVertexParam>::Card
PolygonMesh<MeshVertexParam>::finishFace(void)
	{
	if(!addingFace)
		Misc::throwStdErr("PolygonMesh::finishFace: Not adding face");
	addingFace=false;
	
	if(newNumVertices<3)
		{
		/* Roll back the face vertex index array: */
		for(Card i=0;i<newNumVertices;++i)
			faceVertexIndices.pop_back();
		addingFace=false;
		
		/* Signal an error: */
		return invalidIndex;
		// Misc::throwStdErr("PolygonMesh::finishFace: Need at least three vertices");
		}
	
	/* Prepare the new face: */
	Face newFace;
	newFace.numVertices=newNumVertices;
	newFace.firstVertexIndex=newFirstVertexIndex;
	newFace.surfaceIndex=0;
	newFace.smoothingGroupMask=0x0U;
	
	/* Store the new face: */
	Card result=faces.size();
	faces.push_back(newFace);
	connectFace(result);
	return result;
	}

template <class MeshVertexParam>
inline
typename PolygonMesh<MeshVertexParam>::Card
PolygonMesh<MeshVertexParam>::addFace(
	typename PolygonMesh<MeshVertexParam>::Card newNumVertices,
	const typename PolygonMesh<MeshVertexParam>::Card newVertexIndices[])
	{
	if(newNumVertices<3)
		return invalidIndex;
		// Misc::throwStdErr("PolygonMesh::addFace: Need at least three vertices");
	
	/* Prepare the new face: */
	Face newFace;
	newFace.numVertices=newNumVertices;
	newFace.firstVertexIndex=faceVertexIndices.size();
	newFace.surfaceIndex=0;
	newFace.smoothingGroupMask=0x0U;
	
	/* Store the new vertex indices: */
	for(Card i=0;i<newNumVertices;++i)
		faceVertexIndices.push_back(newVertexIndices[i]);
	
	/* Store the new face: */
	Card result=faces.size();
	faces.push_back(newFace);
	connectFace(result);
	return result;
	}

template <class MeshVertexParam>
inline
typename PolygonMesh<MeshVertexParam>::Card
PolygonMesh<MeshVertexParam>::addFace(
	const std::vector<typename PolygonMesh<MeshVertexParam>::Card>& newVertexIndices)
	{
	if(newVertexIndices.size()<3)
		return invalidIndex;
		// Misc::throwStdErr("PolygonMesh::addFace: Need at least three vertices");
	
	/* Prepare the new face: */
	Face newFace;
	newFace.numVertices=newVertexIndices.size();
	newFace.firstVertexIndex=faceVertexIndices.size();
	newFace.surfaceIndex=0;
	newFace.smoothingGroupMask=0x0U;
	
	/* Store the new vertex indices: */
	for(Card i=0;i<newVertexIndices.size();++i)
		faceVertexIndices.push_back(newVertexIndices[i]);
	
	/* Store the new face: */
	Card result=faces.size();
	faces.push_back(newFace);
	connectFace(result);
	return result;
	}

template <class MeshVertexParam>
inline
void
PolygonMesh<MeshVertexParam>::setFaceSmoothingGroupMask(
	typename PolygonMesh<MeshVertexParam>::Card faceIndex,
	unsigned int newSmoothingGroupMask)
	{
	/* Set the face's smoothing group mask: */
	faces[faceIndex].smoothingGroupMask=newSmoothingGroupMask;
	}

template <class MeshVertexParam>
inline
void
PolygonMesh<MeshVertexParam>::setFaceNormal(
	typename PolygonMesh<MeshVertexParam>::Card faceIndex,
	const typename PolygonMesh<MeshVertexParam>::Vector& newNormal)
	{
	/* Set the face's normal vector: */
	faces[faceIndex].normal=newNormal;
	}

template <class MeshVertexParam>
inline
void
PolygonMesh<MeshVertexParam>::setFaceVertexNormal(
	typename PolygonMesh<MeshVertexParam>::Card faceIndex,
	typename PolygonMesh<MeshVertexParam>::Card vertexIndex,
	const typename PolygonMesh<MeshVertexParam>::Vector& newNormal)
	{
	/* Mark the vertex as a crease vertex: */
	vertexCreaseFlags[vertexIndex]=true;
	
	/* Store the per-face vertex normal: */
	vertexNormals.setEntry(typename FaceVertexNormalHasher::Entry(FaceVertex(faceIndex,vertexIndex),newNormal));
	}

template <class MeshVertexParam>
inline
void
PolygonMesh<MeshVertexParam>::setFaceSurface(
	typename PolygonMesh<MeshVertexParam>::Card faceIndex,
	typename PolygonMesh<MeshVertexParam>::Card surfaceIndex)
	{
	/* Override the face's surface index: */
	faces[faceIndex].surfaceIndex=surfaceIndex;
	
	/* Keep track of the largest used surface index: */
	if(numSurfaces<surfaceIndex+1)
		numSurfaces=surfaceIndex+1;
	}

template <class MeshVertexParam>
inline
void
PolygonMesh<MeshVertexParam>::calcVertexTexCoords(
	const std::vector<const TexCoordCalculator<typename PolygonMesh<MeshVertexParam>::MeshVertex>*>& texCoordCalculators)
	{
	if(texCoordCalculators.size()<numSurfaces)
		Misc::throwStdErr("PolygonMesh::calcVertexTexCoords: Not enough texture coordinate calculators supplied");
	
	/* Iterate through all vertices: */
	Card numVertices=vertices.size();
	for(Card vertexIndex=0;vertexIndex<numVertices;++vertexIndex)
		{
		MeshVertex& vertex=vertices[vertexIndex];
		
		/* Find the first outgoing edge for this vertex (might not exist): */
		DirectedEdge firstEdge(vertexIndex,vertexEdges[vertexIndex]);
		
		/* Loop around the vertex in counter-clockwise order: */
		DirectedEdge edge=firstEdge;
		Card currentSurfaceIndex=invalidIndex;
		TPoint currentTexCoord;
		bool isMultiSurfaceVertex=false;
		DirectedEdge firstWedgeEdge;
		do
			{
			/* Find the face defined by this edge: */
			typename FaceEdgeHasher::Iterator feIt=faceEdges.findEntry(edge);
			if(feIt.isFinished()) // Bail out if boundary edge is reached
				break;
			Card faceIndex=feIt->getDest().faceIndex;
			const Face& face=faces[faceIndex];
			
			if(face.surfaceIndex!=currentSurfaceIndex)
				{
				if(currentSurfaceIndex!=invalidIndex&&!isMultiSurfaceVertex)
					{
					/* This is a multi-surface vertex: */
					firstWedgeEdge=edge;
					isMultiSurfaceVertex=true;
					}
				
				/* Calculate new texture coordinates: */
				currentSurfaceIndex=face.surfaceIndex;
				currentTexCoord=texCoordCalculators[currentSurfaceIndex]->calcTexCoord(vertex.position);
				}
			
			if(isMultiSurfaceVertex)
				{
				/* Assign the current texture coordinates to the current face vertex: */
				vertexTexCoords.setEntry(typename FaceVertexTexCoordHasher::Entry(FaceVertex(faceIndex,vertexIndex),currentTexCoord));
				}
			
			/* Find the next edge around the vertex: */
			edge.set(1,feIt->getDest().previousVertexIndex);
			}
		while(edge!=firstEdge);
		
		if(isMultiSurfaceVertex)
			{
			/* Store the per-face texture coordinates for all faces before the first wedge boundary: */
			DirectedEdge wedgeEdge=firstEdge;
			do
				{
				/* Get a handle to the current face: */
				const FaceEdge& fe=faceEdges.getEntry(wedgeEdge).getDest();
				const Face& face=faces[fe.faceIndex];
				
				if(currentSurfaceIndex!=face.surfaceIndex)
					{
					/* Calculate new texture coordinates: */
					currentSurfaceIndex=face.surfaceIndex;
					currentTexCoord=texCoordCalculators[currentSurfaceIndex]->calcTexCoord(vertex.position);
					}
				
				/* Assign the current texture coordinates to the current face vertex: */
				vertexTexCoords.setEntry(typename FaceVertexTexCoordHasher::Entry(FaceVertex(fe.faceIndex,vertexIndex),currentTexCoord));
				
				/* Go to the next edge: */
				wedgeEdge.set(1,fe.previousVertexIndex);
				}
			while(wedgeEdge!=firstWedgeEdge);
			
			/* Mark the vertex permanently: */
			vertexMultiSurfaceFlags[vertexIndex]=true;
			}
		else
			{
			/* Store the texture coordinates: */
			vertex.texCoord=currentTexCoord;
			}
		}
	}

template <class MeshVertexParam>
inline
void
PolygonMesh<MeshVertexParam>::addCreaseEdge(
	typename PolygonMesh<MeshVertexParam>::Card vertexIndex0,
	typename PolygonMesh<MeshVertexParam>::Card vertexIndex1)
	{
	/*********************************************************************
	No out-of-bounds checking, because having nonexistent crease edges
	doesn't hurt much, and because users might be adding crease edges and
	faces out-of order. And even if the indices are inside bounds, the
	edge they define might still not exist. So there.
	*********************************************************************/
	
	/* Add the crease edge: */
	creaseEdges.setEntry(CreaseEdgeHasher::Entry(UndirectedEdge(vertexIndex0,vertexIndex1)));
	
	/*********************************************************************
	Check if the current outgoing edge of either edge vertex is an
	interior edge. If so, set the outgoing edge to the crease edge to
	simplify normal vector calculation later.
	*********************************************************************/
	
	if(faceEdges.isEntry(DirectedEdge(vertexEdges[vertexIndex0],vertexIndex0)))
		{
		/* Outgoing edge of vertex 0 is an interior edge: */
		vertexEdges[vertexIndex0]=vertexIndex1;
		}
	if(faceEdges.isEntry(DirectedEdge(vertexEdges[vertexIndex1],vertexIndex1)))
		{
		/* Outgoing edge of vertex 1 is an interior edge: */
		vertexEdges[vertexIndex1]=vertexIndex0;
		}
	}

template <class MeshVertexParam>
inline
void
PolygonMesh<MeshVertexParam>::findSmoothingGroupCreaseEdges(
	void)
	{
	/* Iterate through all faces: */
	for(typename std::vector<Face>::iterator fIt=faces.begin();fIt!=faces.end();++fIt)
		{
		/* Iterate through all edges of this face: */
		Card e0=faceVertexIndices[fIt->firstVertexIndex+fIt->numVertices-1];
		for(Card i=0;i<fIt->numVertices;++i)
			{
			Card e1=faceVertexIndices[fIt->firstVertexIndex+i];
			
			/* Check if the edge has an opposite: */
			typename FaceEdgeHasher::Iterator feIt=faceEdges.findEntry(DirectedEdge(e1,e0));
			if(!feIt.isFinished())
				{
				/* Check if the two faces sharing this edge have a smoothing group in common: */
				if((fIt->smoothingGroupMask&faces[feIt->getDest().faceIndex].smoothingGroupMask)==0x0)
					{
					/* Mark the edge as a crease: */
					addCreaseEdge(e0,e1);
					}
				}
			
			/* Go to the next edge: */
			e0=e1;
			}
		}
	}

template <class MeshVertexParam>
inline
void
PolygonMesh<MeshVertexParam>::findCreaseEdges(
	typename PolygonMesh<MeshVertexParam>::Scalar creaseAngle)
	{
	/* Calculate the cosine of the crease angle: */
	Scalar cosCreaseAngle=Math::cos(creaseAngle);
	
	/* Iterate through all faces: */
	for(typename std::vector<Face>::iterator fIt=faces.begin();fIt!=faces.end();++fIt)
		{
		/* Iterate through all edges of this face: */
		Card e0=faceVertexIndices[fIt->firstVertexIndex+fIt->numVertices-1];
		for(Card i=0;i<fIt->numVertices;++i)
			{
			Card e1=faceVertexIndices[fIt->firstVertexIndex+i];
			
			/* Check if the edge has an opposite: */
			typename FaceEdgeHasher::Iterator feIt=faceEdges.findEntry(DirectedEdge(e1,e0));
			if(!feIt.isFinished())
				{
				/* Calculate the angle between this face and the neighboring face: */
				Scalar cosAngle=faces[feIt->getDest().faceIndex].normal*fIt->normal;
				if(cosAngle<cosCreaseAngle)
					{
					/* Mark the edge as a crease: */
					addCreaseEdge(e0,e1);
					}
				}
			
			/* Go to the next edge: */
			e0=e1;
			}
		}
	}

template <class MeshVertexParam>
inline
void
PolygonMesh<MeshVertexParam>::findCreaseEdges(
	typename PolygonMesh<MeshVertexParam>::Card surfaceIndex,
	typename PolygonMesh<MeshVertexParam>::Scalar creaseAngle)
	{
	/* Calculate the cosine of the crease angle: */
	Scalar cosCreaseAngle=Math::cos(creaseAngle);
	
	/* Iterate through all faces: */
	for(typename std::vector<Face>::iterator fIt=faces.begin();fIt!=faces.end();++fIt)
		if(fIt->surfaceIndex==surfaceIndex)
			{
			/* Iterate through all edges of this face: */
			Card e0=faceVertexIndices[fIt->firstVertexIndex+fIt->numVertices-1];
			for(Card i=0;i<fIt->numVertices;++i)
				{
				Card e1=faceVertexIndices[fIt->firstVertexIndex+i];
				
				/* Check if the edge has an opposite whose face belongs to the same surface: */
				typename FaceEdgeHasher::Iterator feIt=faceEdges.findEntry(DirectedEdge(e1,e0));
				if(!feIt.isFinished())
					{
					Face& face=faces[feIt->getDest().faceIndex];
					if(face.surfaceIndex==surfaceIndex)
						{
						/* Calculate the angle between this face and the neighboring face: */
						Scalar cosAngle=face.normal*fIt->normal;
						if(cosAngle<cosCreaseAngle)
							{
							/* Mark the edge as a crease: */
							addCreaseEdge(e0,e1);
							}
						}
					}
				
				/* Go to the next edge: */
				e0=e1;
				}
			}
	}

template <class MeshVertexParam>
inline
void
PolygonMesh<MeshVertexParam>::findCreaseEdges(
	const std::vector<typename PolygonMesh<MeshVertexParam>::Scalar>& creaseAngles)
	{
	if(creaseAngles.size()<numSurfaces)
		Misc::throwStdErr("PolygonMesh::findCreaseEdges: Not enough crease angles supplied");
	
	/* Calculate the cosines of all crease angles: */
	std::vector<Scalar> cosCreaseAngles;
	cosCreaseAngles.reserve(creaseAngles.size());
	for(typename std::vector<Scalar>::const_iterator caIt=creaseAngles.begin();caIt!=creaseAngles.end();++caIt)
		cosCreaseAngles.push_back(Math::cos(*caIt));
	
	/* Iterate through all faces: */
	for(typename std::vector<Face>::iterator fIt=faces.begin();fIt!=faces.end();++fIt)
		{
		/* Iterate through all edges of this face: */
		Card e0=faceVertexIndices[fIt->firstVertexIndex+fIt->numVertices-1];
		for(Card i=0;i<fIt->numVertices;++i)
			{
			Card e1=faceVertexIndices[fIt->firstVertexIndex+i];
			
			/* Check if the edge has an opposite whose face belongs to the same surface: */
			typename FaceEdgeHasher::Iterator feIt=faceEdges.findEntry(DirectedEdge(e1,e0));
			if(!feIt.isFinished())
				{
				Face& face=faces[feIt->getDest().faceIndex];
				
				/* Calculate the angle between this face and the neighboring face: */
				Scalar cosAngle=face.normal*fIt->normal;
				
				/* Compare the angle against the crease angles on both sides of the edge: */
				if(cosAngle<cosCreaseAngles[face.surfaceIndex]&&cosAngle<cosCreaseAngles[fIt->surfaceIndex])
					{
					/* Mark the edge as a crease: */
					addCreaseEdge(e0,e1);
					}
				}

			/* Go to the next edge: */
			e0=e1;
			}
		}
	}

template <class MeshVertexParam>
inline
void
PolygonMesh<MeshVertexParam>::findSurfaceCreaseEdges(
	void)
	{
	/* Iterate through all faces: */
	for(typename std::vector<Face>::iterator fIt=faces.begin();fIt!=faces.end();++fIt)
		{
		/* Iterate through all edges of this face: */
		Card e0=faceVertexIndices[fIt->firstVertexIndex+fIt->numVertices-1];
		for(Card i=0;i<fIt->numVertices;++i)
			{
			Card e1=faceVertexIndices[fIt->firstVertexIndex+i];
			
			/* Check if the edge has an opposite whose face belongs to a different surface: */
			typename FaceEdgeHasher::Iterator feIt=faceEdges.findEntry(DirectedEdge(e1,e0));
			if(!feIt.isFinished()&&faces[feIt->getDest().faceIndex].surfaceIndex!=fIt->surfaceIndex)
				{
				/* Mark the edge as a crease: */
				addCreaseEdge(e0,e1);
				}
			
			/* Go to the next edge: */
			e0=e1;
			}
		}
	}

template <class MeshVertexParam>
inline
void
PolygonMesh<MeshVertexParam>::calcVertexNormals(
	void)
	{
	/* Iterate through all vertices (have to do it by index): */
	Card numVertices=vertices.size();
	for(Card vertexIndex=0;vertexIndex<numVertices;++vertexIndex)
		{
		/* Initialize the vertex normal: */
		Vector normal=Vector::zero;
		
		/* Find the first outgoing edge for this vertex (might not exist): */
		DirectedEdge firstEdge(vertexIndex,vertexEdges[vertexIndex]);
		
		/* Loop around the vertex in counter-clockwise order: */
		DirectedEdge edge=firstEdge;
		bool isCreaseVertex=false;
		DirectedEdge wedgeStartEdge=firstEdge;
		do
			{
			/* Find the face defined by this edge: */
			typename FaceEdgeHasher::Iterator feIt=faceEdges.findEntry(edge);
			if(feIt.isFinished()) // Bail out if boundary edge is reached
				break;
			Face& face=faces[feIt->getDest().faceIndex];
			
			#if 1
			/* Calculate the corner angle: */
			Vector d0=vertices[feIt->getDest().previousVertexIndex].position-vertices[edge[0]].position;
			Vector d1=vertices[edge[1]].position-vertices[edge[0]].position;
			Scalar angleCos=(d0*d1)/(Geometry::mag(d0)*Geometry::mag(d1));
			Scalar angle;
			if(angleCos>Scalar(1))
				angle=Scalar(0);
			else if(angleCos<Scalar(-1))
				angle=Math::Constants<Scalar>::pi;
			else
				angle=Math::acos(angleCos);
			
			/* Accumulate this face's normal vector weighted by corner angle: */
			normal+=face.normal*angle;
			#else
			/* Accumulate this face's normal vector: */
			normal+=face.normal;
			#endif
			
			/* Find the next edge around the vertex: */
			edge.set(1,feIt->getDest().previousVertexIndex);
			
			/* Check if the new edge is a crease edge: */
			if(edge!=firstEdge&&creaseEdges.isEntry(UndirectedEdge(edge[0],edge[1])))
				{
				/* Store the current accumulated normal in all faces belonging to the current wedge: */
				normal.normalize();
				DirectedEdge wedgeEdge=wedgeStartEdge;
				while(wedgeEdge!=edge)
					{
					/* Store the per-face vertex normal: */
					const FaceEdge& fe=faceEdges.getEntry(wedgeEdge).getDest();
					vertexNormals.setEntry(typename FaceVertexNormalHasher::Entry(FaceVertex(fe.faceIndex,vertexIndex),normal));
					
					/* Go to the next edge: */
					wedgeEdge.set(1,fe.previousVertexIndex);
					}
				
				/* Start accumulating a new wedge: */
				normal=Vector::zero;
				wedgeStartEdge=edge;
				
				/* Mark the vertex as a crease vertex: */
				isCreaseVertex=true;
				}
			}
		while(edge!=firstEdge);
		
		if(isCreaseVertex)
			{
			/* Store the current accumulated normal in all faces belonging to the current wedge: */
			normal.normalize();
			DirectedEdge wedgeEdge=wedgeStartEdge;
			while(wedgeEdge!=edge)
				{
				/* Store the per-face vertex normal: */
				const FaceEdge& fe=faceEdges.getEntry(wedgeEdge).getDest();
				vertexNormals.setEntry(typename FaceVertexNormalHasher::Entry(FaceVertex(fe.faceIndex,vertexIndex),normal));
				
				/* Go to the next edge: */
				wedgeEdge.set(1,fe.previousVertexIndex);
				}
			
			/* Mark the vertex permanently: */
			vertexCreaseFlags[vertexIndex]=true;
			}
		else
			{
			/* Store the final normal vector: */
			vertices[vertexIndex].normal=normal.normalize();
			}
		}
	}

template <class MeshVertexParam>
inline
LineSet<typename PolygonMesh<MeshVertexParam>::MeshVertex>*
PolygonMesh<MeshVertexParam>::createLineSet(
	bool getBoundaryEdges,
	bool getCreaseEdges) const
	{
	/* Create the result line set: */
	Misc::SelfDestructPointer<LineSet<MeshVertex> > result(new LineSet<MeshVertex>());
	
	if(getBoundaryEdges)
		{
		/* Add all boundary edges to the line set: */
		result->setSubMeshColor(typename LineSet<MeshVertex>::Color(1.0f,0.0f,0.0f));
		
		/* Iterate through all faces: */
		Card numFaces=faces.size();
		for(Card faceIndex=0;faceIndex<numFaces;++faceIndex)
			{
			const Face& face=faces[faceIndex];
			
			/* Iterate through all edges of this face: */
			Card e0=faceVertexIndices[face.firstVertexIndex+face.numVertices-1];
			for(Card i=0;i<face.numVertices;++i)
				{
				Card e1=faceVertexIndices[face.firstVertexIndex+i];
				
				/* Check if the edge has no opposite, i.e., is a boundary edge: */
				typename FaceEdgeHasher::ConstIterator feIt=faceEdges.findEntry(DirectedEdge(e1,e0));
				if(feIt.isFinished())
					{
					/* Add the edge to the line set: */
					result->addVertex(vertices[e0]);
					result->addVertex(vertices[e1]);
					}
				
				/* Go to the next edge: */
				e0=e1;
				}
			}
		
		result->finishSubMesh();
		}
	
	if(getCreaseEdges)
		{
		/* Add all crease edges to the line set: */
		result->setSubMeshColor(typename LineSet<MeshVertex>::Color(1.0f,1.0f,0.0f));
		
		/* Iterate through all faces: */
		Card numFaces=faces.size();
		for(Card faceIndex=0;faceIndex<numFaces;++faceIndex)
			{
			const Face& face=faces[faceIndex];
			
			/* Iterate through all edges of this face: */
			Card e0=faceVertexIndices[face.firstVertexIndex+face.numVertices-1];
			for(Card i=0;i<face.numVertices;++i)
				{
				Card e1=faceVertexIndices[face.firstVertexIndex+i];
				
				/* Check if the edge has an opposite, i.e., is an interior edge: */
				typename FaceEdgeHasher::ConstIterator feIt=faceEdges.findEntry(DirectedEdge(e1,e0));
				if(!feIt.isFinished())
					{
					/* Add each crease edge only once: */
					if(e0<e1&&creaseEdges.isEntry(UndirectedEdge(e0,e1)))
						{
						/* Add the edge to the line set: */
						result->addVertex(vertices[e0]);
						result->addVertex(vertices[e1]);
						}
					}
				
				/* Go to the next edge: */
				e0=e1;
				}
			}
		
		result->finishSubMesh();
		}
	
	/* Return the result line set: */
	return result.releaseTarget();
	}

template <class MeshVertexParam>
template <class TriangleSetParam>
inline
void
PolygonMesh<MeshVertexParam>::triangulate(
	TriangleSetParam& triangleSet) const
	{
	/* Create a tesselator object to help tesselating non-convex faces: */
	typedef Tesselator<MeshVertex> TS;
	TS tesselator(32); // 32 is just a guess; the object auto-adjusts
	tesselator.setVertices(&vertices[0]);
	
	/* Add all faces to the triangle set: */
	Card numFaces=faces.size();
	for(Card faceIndex=0;faceIndex<numFaces;++faceIndex)
		{
		const Face& face=faces[faceIndex];
		
		/* Add the face's vertex loop to the tesselator: */
		tesselator.reset();
		for(Card i=0;i<face.numVertices;++i)
			tesselator.addVertex(faceVertexIndices[face.firstVertexIndex+i]);
		
		/* Tesselate the face: */
		tesselator.tesselate(face.normal);
		
		/* Store each triangle in the triangle set: */
		const typename TS::Index* tviPtr=tesselator.getTriangleVertexIndices();
		for(typename TS::Card triangleIndex=0;triangleIndex<tesselator.getNumTriangles();++triangleIndex,tviPtr+=3)
			{
			/* Add the triangle's vertices to the result triangle set: */
			for(unsigned int i=0;i<3;++i)
				{
				MeshVertex vertex;
				vertex.texCoord=getVertexTexCoord(faceIndex,tviPtr[i]);
				vertex.normal=getVertexNormal(faceIndex,tviPtr[i]);
				vertex.position=vertices[tviPtr[i]].position;
				triangleSet.addVertex(vertex);
				}
			}
		}
	}

template <class MeshVertexParam>
template <class TriangleSetParam>
inline
void
PolygonMesh<MeshVertexParam>::triangulateSurface(
	TriangleSetParam& triangleSet,
	typename PolygonMesh<MeshVertexParam>::Card surfaceIndex) const
	{
	/* Create a tesselator object to help tesselating non-convex faces: */
	typedef Tesselator<MeshVertex> TS;
	TS tesselator(32); // 32 is just a guess; the object auto-adjusts
	tesselator.setVertices(&vertices[0]);
	
	/* Add all faces belonging to the given surface to the triangle set: */
	Card numFaces=faces.size();
	for(Card faceIndex=0;faceIndex<numFaces;++faceIndex)
		{
		const Face& face=faces[faceIndex];
		if(face.surfaceIndex==surfaceIndex)
			{
			/* Add the face's vertex loop to the tesselator: */
			tesselator.reset();
			for(Card i=0;i<face.numVertices;++i)
				tesselator.addVertex(faceVertexIndices[face.firstVertexIndex+i]);
			
			/* Tesselate the face: */
			tesselator.tesselate(face.normal);
			
			/* Store each triangle in the triangle set: */
			const typename TS::Index* tviPtr=tesselator.getTriangleVertexIndices();
			for(typename TS::Card triangleIndex=0;triangleIndex<tesselator.getNumTriangles();++triangleIndex,tviPtr+=3)
				{
				/* Add the triangle's vertices to the result triangle set: */
				for(unsigned int i=0;i<3;++i)
					{
					MeshVertex vertex;
					vertex.texCoord=getVertexTexCoord(faceIndex,tviPtr[i]);
					vertex.normal=getVertexNormal(faceIndex,tviPtr[i]);
					vertex.position=vertices[tviPtr[i]].position;
					triangleSet.addVertex(vertex);
					}
				}
			}
		}
	}
