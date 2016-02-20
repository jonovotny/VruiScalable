/***********************************************************************
PhongTextureMaterial - Class to represent standard OpenGL Phong lighting
material properties with a diffuse texture map.
Copyright (c) 2009-2012 Oliver Kreylos
***********************************************************************/

#include "PhongTextureMaterial.h"

#include <Math/Random.h>
#include <GL/GLContextData.h>

/***********************************************
Methods of class PhongTextureMaterial::DataItem:
***********************************************/

PhongTextureMaterial::DataItem::DataItem(void)
	:textureObjectId(0)
	{
	/* Create a texture object: */
	glGenTextures(1,&textureObjectId);
	}

PhongTextureMaterial::DataItem::~DataItem(void)
	{
	/* Destroy the texture object: */
	glDeleteTextures(1,&textureObjectId);
	}

/*************************************
Methods of class PhongTextureMaterial:
*************************************/

PhongTextureMaterial::PhongTextureMaterial(const GLMaterial& sFrontMaterial,const GLMaterial& sBackMaterial,const Texture& sDiffuseMap)
	:frontMaterial(sFrontMaterial),backMaterial(sBackMaterial),
	 twoSided(false),
	 diffuseMap(sDiffuseMap),
	 wrapS(GL_REPEAT),wrapT(GL_REPEAT)
	{
	}

PhongTextureMaterial::PhongTextureMaterial(const GLMaterial& sMaterial,const Texture& sDiffuseMap)
	:frontMaterial(sMaterial),backMaterial(sMaterial),
	 twoSided(false),
	 diffuseMap(sDiffuseMap),
	 wrapS(GL_REPEAT),wrapT(GL_REPEAT)
	{
	}

void PhongTextureMaterial::set(GLContextData& contextData) const
	{
	/* Retrieve the data item: */
	DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
	
	/* Save OpenGL state: */
	glPushAttrib(GL_COLOR_BUFFER_BIT|GL_ENABLE_BIT|GL_LIGHTING_BIT|GL_TEXTURE_BIT);
	
	/* Set up OpenGL state: */
	glEnable(GL_LIGHTING);
	glDisable(GL_COLOR_MATERIAL);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GEQUAL,0.5f);
	
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
	
	/* Enable texture mapping: */
	GLenum textureTarget=diffuseMap.glGetTextureTarget();
	glEnable(textureTarget);
	
	/* Bind the diffuse map texture: */
	glBindTexture(textureTarget,dataItem->textureObjectId);
	
	glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL,GL_SEPARATE_SPECULAR_COLOR);
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
	}

void PhongTextureMaterial::reset(GLContextData& contextData) const
	{
	/* Protect the diffuse map texture: */
	glBindTexture(diffuseMap.glGetTextureTarget(),0);
	
	/* Restore OpenGL state: */
	glPopAttrib();
	}

void PhongTextureMaterial::initContext(GLContextData& contextData) const
	{
	/* Create a new data item and store it in the OpenGL context: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	
	/* Upload the texture image: */
	GLenum textureTarget=diffuseMap.glGetTextureTarget();
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
	diffuseMap.glTexImage();
	#endif
	
	/* Protect the texture object: */
	glBindTexture(textureTarget,0);
	}

void PhongTextureMaterial::setTwoSided(bool newTwoSided)
	{
	twoSided=newTwoSided;
	}

void PhongTextureMaterial::setWrap(GLenum newWrapS,GLenum newWrapT)
	{
	wrapS=newWrapS;
	wrapT=newWrapT;
	}
