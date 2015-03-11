/***********************************************************************
TextureMaterial - Class to represent non-illuminated texture maps.
Copyright (c) 2011 Oliver Kreylos
***********************************************************************/

#ifndef TEXTUREMATERIAL_INCLUDED
#define TEXTUREMATERIAL_INCLUDED

#include <GL/gl.h>
#include <GL/GLMaterial.h>
#include <GL/GLObject.h>

#include "Texture.h"

#include "Material.h"

class TextureMaterial:public Material,public GLObject
	{
	/* Embedded classes: */
	private:
	struct DataItem:public GLObject::DataItem
		{
		/* Elements: */
		public:
		GLuint textureObjectId; // Object ID of material's texture
		
		/* Constructors and destructors: */
		DataItem(void);
		virtual ~DataItem(void);
		};
	
	/* Elements: */
	private:
	Texture map; // Material texture
	GLenum wrapS,wrapT; // Texture wrapping modes in both texture coordinate directions
	
	/* Constructors and destructors: */
	public:
	TextureMaterial(const Texture& sMap); // Creates texture material
	
	/* Methods from Material: */
	virtual unsigned int needsTextureCoordinates(void) const
		{
		return 1;
		}
	virtual bool needsNormals(void) const
		{
		return false;
		}
	virtual void set(GLContextData& contextData) const;
	virtual void reset(GLContextData& contextData) const;
	
	/* Methods from GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	
	/* New methods: */
	void setWrap(GLenum newWrapS,GLenum newWrapT); // Sets texture wrapping modes
	};

#endif
