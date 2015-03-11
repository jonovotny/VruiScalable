/***********************************************************************
TexCoordCalculator - Abstract base class to calculate texture
coordinates for mesh vertices
Copyright (c) 2009 Oliver Kreylos
***********************************************************************/

#ifndef TEXCOORDCALCULATOR_INCLUDED
#define TEXCOORDCALCULATOR_INCLUDED

template <class MeshVertexParam>
class TexCoordCalculator
	{
	/* Embedded classes: */
	public:
	typedef MeshVertexParam MeshVertex; // Type for mesh vertices
	typedef typename MeshVertex::Scalar Scalar; // Scalar type for points
	typedef typename MeshVertex::Point Point; // Type for points
	typedef typename MeshVertex::TPoint TPoint; // Type for points in texture space
	
	/* Constructors and destructors: */
	public:
	virtual ~TexCoordCalculator(void) // Destroys the material
		{
		}
	
	/* Methods: */
	virtual TPoint calcTexCoord(const Point& position) const =0; // Returns texture coordinates for the given vertex position
	};

#endif
