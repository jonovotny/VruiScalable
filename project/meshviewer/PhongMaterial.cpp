/***********************************************************************
PhongMaterial - Class to represent standard OpenGL Phong lighting
material properties.
Copyright (c) 2009-2012 Oliver Kreylos
***********************************************************************/

#include "PhongMaterial.h"

#include <GL/gl.h>

/******************************
Methods of class PhongMaterial:
******************************/

PhongMaterial::PhongMaterial(const GLMaterial& sFrontMaterial,const GLMaterial& sBackMaterial)
	:frontMaterial(sFrontMaterial),backMaterial(sBackMaterial),
	 twoSided(false)
	{
	}

PhongMaterial::PhongMaterial(const GLMaterial& sMaterial)
	:frontMaterial(sMaterial),backMaterial(sMaterial),
	 twoSided(false)
	{
	}

void PhongMaterial::set(GLContextData& contextData) const
	{
	/* Save OpenGL state: */
	glPushAttrib(GL_ENABLE_BIT|GL_LIGHTING_BIT);
	
	/* Set up OpenGL state: */
	glEnable(GL_LIGHTING);
	glDisable(GL_COLOR_MATERIAL);
	
	/* Set the front and back materials: */
	glMaterial(GLMaterialEnums::FRONT,frontMaterial);
	glMaterial(GLMaterialEnums::BACK,backMaterial);
	
	#if 0
	if(twoSided)
		{
		/* Disable culling: */
		glDisable(GL_CULL_FACE);
		
		/* Enable two-sided lighting: */
		glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_TRUE);
		}
	else
		{
		/* Enable culling: */
		glEnable(GL_CULL_FACE);
		
		/* Disable two-sided lighting: */
		glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_FALSE);
		}
	#endif
	}

void PhongMaterial::reset(GLContextData& contextData) const
	{
	/* Restore OpenGL state: */
	glPopAttrib();
	}

void PhongMaterial::setTwoSided(bool newTwoSided)
	{
	twoSided=newTwoSided;
	}
