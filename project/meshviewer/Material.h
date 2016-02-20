/***********************************************************************
Material - Abstract base class for material properties applied to
surfaces during rendering.
Copyright (c) 2009 Oliver Kreylos
***********************************************************************/

#ifndef MATERIAL_INCLUDED
#define MATERIAL_INCLUDED

#include <Misc/RefCounted.h>
#include <Misc/Autopointer.h>

/* Forward declarations: */
class GLContextData;

class Material:public Misc::RefCounted
	{
	/* Constructors and destructors: */
	public:
	virtual ~Material(void) // Destroys the material
		{
		}
	
	/* Methods: */
	virtual unsigned int needsTextureCoordinates(void) const // Returns the number of per-vertex texture coordinates required by this material
		{
		return 0;
		}
	virtual bool needsColors(void) const // Returns true if this material requires per-vertex colors
		{
		return false;
		}
	virtual bool needsNormals(void) const // Returns true if this material requires per-vertex normal vectors
		{
		return false;
		}
	virtual bool needsTangents(void) const // Returns true if this material requires per-vertex tangent vectors
		{
		return false;
		}
	virtual void set(GLContextData& contextData) const =0; // Sets the material properties in the current OpenGL context
	virtual void reset(GLContextData& contextData) const =0; // Unsets the material properties in the current OpenGL context
	};

typedef Misc::Autopointer<Material> MaterialPointer;

#endif
