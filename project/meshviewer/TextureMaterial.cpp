/***********************************************************************
TextureMaterial - Class to represent non-illuminated texture maps.
Copyright (c) 2011 Oliver Kreylos
***********************************************************************/

#include "TextureMaterial.h"

#include <Math/Random.h>
#include <GL/GLContextData.h>

/******************************************
Methods of class TextureMaterial::DataItem:
******************************************/

TextureMaterial::DataItem::DataItem(void)
	:textureObjectId(0)
	{
	/* Create a texture object: */
	glGenTextures(1,&textureObjectId);
	}

TextureMaterial::DataItem::~DataItem(void)
	{
	/* Destroy the texture object: */
	glDeleteTextures(1,&textureObjectId);
	}

/********************************
Methods of class TextureMaterial:
********************************/

TextureMaterial::TextureMaterial(const Texture& sMap)
	:map(sMap),
	 wrapS(GL_REPEAT),wrapT(GL_REPEAT)
	{
	}

void TextureMaterial::set(GLContextData& contextData) const
	{
	/* Retrieve the data item: */
	DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
	
	/* Save OpenGL state: */
	glPushAttrib(GL_COLOR_BUFFER_BIT|GL_TEXTURE_BIT);
	
	/* Set up OpenGL state: */
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GEQUAL,0.5f);
	
	/* Enable texture mapping: */
	GLenum textureTarget=map.glGetTextureTarget();
	glEnable(textureTarget);
	
	/* Bind the map texture: */
	glBindTexture(textureTarget,dataItem->textureObjectId);
	
	/* Enable texture mapping: */
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
	}

void TextureMaterial::reset(GLContextData& contextData) const
	{
	/* Protect the diffuse map texture: */
	glBindTexture(map.glGetTextureTarget(),0);
	
	/* Restore OpenGL state: */
	glPopAttrib();
	}

void TextureMaterial::initContext(GLContextData& contextData) const
	{
	/* Create a new data item and store it in the OpenGL context: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	
	/* Upload the texture image: */
	GLenum textureTarget=map.glGetTextureTarget();
	glBindTexture(textureTarget,dataItem->textureObjectId);
	glTexParameteri(textureTarget,GL_TEXTURE_WRAP_S,wrapS);
	glTexParameteri(textureTarget,GL_TEXTURE_WRAP_T,wrapT);
	glTexParameteri(textureTarget,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(textureTarget,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	
	#if 0
	int width=314;
	int height=233;
	GLubyte* data=new GLubyte[width*height*4];
	for(int i=0;i<width*height*4;++i)
		data[i]=GLubyte(Math::randUniformCO(0,256));
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,data);
	delete[] data;
	#else
	map.glTexImage();
	#endif
	
	/* Protect the texture object: */
	glBindTexture(textureTarget,0);
	}

void TextureMaterial::setWrap(GLenum newWrapS,GLenum newWrapT)
	{
	wrapS=newWrapS;
	wrapT=newWrapT;
	}
