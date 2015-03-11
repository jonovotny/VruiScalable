/***********************************************************************
PhongMaterial - Class to represent standard OpenGL Phong lighting
material properties.
Copyright (c) 2009-2012 Oliver Kreylos
***********************************************************************/

#ifndef PHONGMATERIAL_INCLUDED
#define PHONGMATERIAL_INCLUDED

#include <GL/GLMaterial.h>

#include "Material.h"

class PhongMaterial:public Material
	{
	/* Elements: */
	private:
	GLMaterial frontMaterial; // The OpenGL material properties for polygon front faces
	GLMaterial backMaterial; // The OpenGL material properties for polygon back faces
	bool twoSided; // Flag whether the surface is two-sided
	
	/* Constructors and destructors: */
	public:
	PhongMaterial(const GLMaterial& sFrontMaterial,const GLMaterial& sBackMaterial); // Creates two-sided material
	PhongMaterial(const GLMaterial& sMaterial); // Creates single-sided material
	
	/* Methods from Material: */
	virtual bool needsNormals(void) const
		{
		return true;
		}
	virtual void set(GLContextData& contextData) const;
	virtual void reset(GLContextData& contextData) const;
	
	/* New methods: */
	void setTwoSided(bool newTwoSided); // Sets the material's two-sided flag
	};

#endif
