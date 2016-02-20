/***********************************************************************
PhongTextureMaterial - Class to represent standard OpenGL Phong lighting
material properties with a diffuse texture map.
Copyright (c) 2009-2012 Oliver Kreylos
***********************************************************************/

#ifndef PHONGTEXTUREMATERIAL_INCLUDED
#define PHONGTEXTUREMATERIAL_INCLUDED

#include <GL/gl.h>
#include <GL/GLMaterial.h>
#include <GL/GLObject.h>

#include "Texture.h"

#include "Material.h"

class PhongTextureMaterial:public Material,public GLObject
	{
	/* Embedded classes: */
	private:
	struct DataItem:public GLObject::DataItem
		{
		/* Elements: */
		public:
		GLuint textureObjectId; // Object ID of material's diffuse texture
		
		/* Constructors and destructors: */
		DataItem(void);
		virtual ~DataItem(void);
		};
	
	/* Elements: */
	private:
	GLMaterial frontMaterial; // The OpenGL material properties for polygon front faces
	GLMaterial backMaterial; // The OpenGL material properties for polygon back faces
	bool twoSided; // Flag whether the surface is two-sided
	Texture diffuseMap; // Texture to replace the materials' diffuse component
	GLenum wrapS,wrapT; // Texture wrapping modes in both texture coordinate directions
	
	/* Constructors and destructors: */
	public:
	PhongTextureMaterial(const GLMaterial& sFrontMaterial,const GLMaterial& sBackMaterial,const Texture& sDiffuseMap); // Creates two-sided material
	PhongTextureMaterial(const GLMaterial& sMaterial,const Texture& sDiffuseMap); // Creates single-sided material
	
	/* Methods from Material: */
	virtual unsigned int needsTextureCoordinates(void) const
		{
		return 1;
		}
	virtual bool needsNormals(void) const
		{
		return true;
		}
	virtual void set(GLContextData& contextData) const;
	virtual void reset(GLContextData& contextData) const;
	
	/* Methods from GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	
	/* New methods: */
	void setTwoSided(bool newTwoSided); // Sets the material's two-sided flag
	void setWrap(GLenum newWrapS,GLenum newWrapT); // Sets texture wrapping modes
	};

#endif
