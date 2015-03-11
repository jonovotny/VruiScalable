/***********************************************************************
Texture - Class to represent textures in uncompressed and compressed
formats.
Copyright (c) 2007-2009 Oliver Kreylos
***********************************************************************/

#include "Texture.h"

#include <string.h>
#include <Misc/ThrowStdErr.h>
#include <GL/gl.h>
#include <GL/GLExtensionManager.h>
#include <GL/Extensions/GLARBTextureCompression.h>
#include <GL/Extensions/GLEXTTexture3D.h>
#include <GL/Extensions/GLEXTTextureCompressionS3TC.h>
#include <GL/Extensions/GLEXTTextureCubeMap.h>

/************************
Methods of class Texture:
************************/

bool Texture::initExtensions(void)
	{
	/* Check if all required extensions are supported: */
	if(!GLARBTextureCompression::isSupported()||!GLEXTTexture3D::isSupported()||!GLEXTTextureCompressionS3TC::isSupported()||!GLEXTTextureCubeMap::isSupported())
		return false;
	
	/* Initialize all required extensions: */
	GLARBTextureCompression::initExtension();
	GLEXTTexture3D::initExtension();
	GLEXTTextureCompressionS3TC::initExtension();
	GLEXTTextureCubeMap::initExtension();
	}

size_t Texture::calcImageSize(const Texture::Size& imageSize) const
	{
	/* Calculate the required size for this mipmap level: */
	size_t result;
	switch(storageFormat)
		{
		case RGB:
			{
			// size_t rowSize=(imageSize[0]*3U+3U)&~0x03U; // Row size is aligned to 4 bytes
			size_t rowSize=imageSize[0]*3U; // No alignment for now
			result=rowSize*imageSize[1];
			break;
			}
		
		case RGBA:
			{
			size_t rowSize=imageSize[0]*4U; // No need to align row size; always multiple of 4 bytes
			result=rowSize*imageSize[1];
			break;
			}
		
		case DXT1:
			{
			result=((imageSize[0]+3U)/4U)*((imageSize[1]+3U)/4U)*8U; // DXT1 is 8 byte per 4x4 tile
			break;
			}
		
		case DXT2:
		case DXT3:
		case DXT4:
		case DXT5:
		case RXGB:
			{
			result=((imageSize[0]+3U)/4U)*((imageSize[1]+3U)/4U)*16U; // DXT2-5 and RXGB are 16 byte per 4x4 tile
			break;
			}
		}
	
	return result;
	}

Texture::Texture(const Texture::Size& sSize,unsigned int sCubeMapFaces,Texture::StorageFormat sStorageFormat,unsigned int sMaxMipMapLevel)
	:size(sSize),
	 cubeMapFaces(sCubeMapFaces),
	 maxMipMapLevel(sMaxMipMapLevel),
	 storageFormat(sStorageFormat),
	 data(0)
	{
	/* Fix the size to match the cube map face mask: */
	if(cubeMapFaces!=NO_CUBEMAP)
		{
		/* Count the number of cube map faces: */
		GLsizei numFaces=0U;
		for(int i=0;i<6;++i)
			if(cubeMapFaces&(0x1U<<i))
				++numFaces;
		
		/* Set the texture's depth: */
		size[2]=numFaces;
		}
	
	/* Calculate the size requirements for the texture image data: */
	size_t totalSize=0;
	Size levelSize=size;
	for(int level=0;level<=maxMipMapLevel;++level)
		{
		/* Accumulate the byte size for the current level: */
		totalSize+=calcImageSize(levelSize)*levelSize[2];
		
		/* Bail out early if the smallest mipmap level is reached: */
		if(levelSize[0]==1U&&levelSize[1]==1U&&(cubeMapFaces!=NO_CUBEMAP||levelSize[2]==1U))
			{
			maxMipMapLevel=level; // Remember the number of the smallest valid mipmap level
			break;
			}
		
		/* Go to the next mipmap level: */
		if(cubeMapFaces!=NO_CUBEMAP)
			{
			for(int i=0;i<2;++i)
				levelSize[i]=(levelSize[i]+1U)>>1;
			}
		else
			{
			for(int i=0;i<3;++i)
				levelSize[i]=(levelSize[i]+1U)>>1;
			}
		}
	
	/* Create the texture image data: */
	ref(allocData(totalSize));
	}

void Texture::setLevelData(unsigned int level,const void* levelData,size_t levelDataSize)
	{
	/* Find the base pointer for the given mipmap level: */
	Size lSize=size;
	size_t lDataSize=calcImageSize(lSize);
	unsigned char* lData=data;
	for(unsigned int l=0;l<level;++l)
		{
		/* Go to the next mipmap level: */
		lData+=lDataSize;
		for(int i=0;i<3;++i)
			lSize[i]=(lSize[i]+1U)>>1;
		lDataSize=calcImageSize(lSize);
		}
	
	/* Check the given level data size: */
	if(levelDataSize!=lDataSize)
		Misc::throwStdErr("Texture::setLevelData: Level data size does not match mipmap level size");
	
	/* Copy the level data: */
	memcpy(lData,levelData,lDataSize);
	}

GLenum Texture::glGetTextureTarget(void) const
	{
	/* Determine the proper texture target: */
	GLenum target;
	if(size[2]==1U) // Regular 2D texture
		target=GL_TEXTURE_2D;
	else if(cubeMapFaces==NO_CUBEMAP) // 3D texture
		target=GL_TEXTURE_3D_EXT;
	else // Cube map texture
		target=GL_TEXTURE_CUBE_MAP_EXT;
	
	return target;
	}

void Texture::glTexImage(void) const
	{
	/* Determine the proper texture target: */
	GLenum target;
	if(size[2]==1U) // Regular 2D texture
		target=GL_TEXTURE_2D;
	else if(cubeMapFaces==NO_CUBEMAP) // 3D texture
		target=GL_TEXTURE_3D_EXT;
	else // Cube map texture
		target=GL_TEXTURE_CUBE_MAP_EXT;
	
	/* Determine the proper texture data formats: */
	GLenum internalFormat;
	bool compressed;
	GLenum externalFormat;
	GLenum dataType;
	switch(storageFormat)
		{
		case RGB:
			internalFormat=GL_RGB8;
			compressed=false;
			externalFormat=GL_RGB;
			dataType=GL_UNSIGNED_BYTE;
			break;
		
		case RGBA:
			internalFormat=GL_RGBA8;
			compressed=false;
			externalFormat=GL_RGBA;
			dataType=GL_UNSIGNED_BYTE;
			break;
		
		case DXT1:
		case DXT2:
		case DXT3:
		case DXT4:
		case DXT5:
		case RXGB:
			internalFormat=GL_RGB8;
			compressed=true;
			externalFormat=GL_RGB;
			dataType=GL_UNSIGNED_BYTE;
		}
	
	/* Prepare the current texture object for the correct number of mipmap levels: */
	glTexParameteri(target,GL_TEXTURE_BASE_LEVEL,0);
	glTexParameteri(target,GL_TEXTURE_MAX_LEVEL,maxMipMapLevel);
	
	/* Define texture data's memory layout: */
	glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
	glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
	glPixelStorei(GL_UNPACK_IMAGE_HEIGHT,0);
	glPixelStorei(GL_UNPACK_SKIP_IMAGES,0);
	// glPixelStorei(GL_UNPACK_ALIGNMENT,4);
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	
	/* Upload all mipmap levels: */
	Size levelSize=size;
	const unsigned char* levelData=data;
	for(unsigned int level=0;level<=maxMipMapLevel;++level)
		{
		/* Upload this mipmap level: */
		size_t sliceSize=calcImageSize(levelSize);
		if(size[2]==1U)
			{
			glTexImage2D(GL_TEXTURE_2D,level,internalFormat,levelSize[0],levelSize[1],0,externalFormat,dataType,levelData);
			levelData+=sliceSize;
			}
		else if(cubeMapFaces==NO_CUBEMAP)
			{
			glTexImage3DEXT(GL_TEXTURE_3D_EXT,level,internalFormat,levelSize[0],levelSize[1],levelSize[2],0,externalFormat,dataType,levelData);
			levelData+=sliceSize*levelSize[2];
			}
		else
			{
			for(int i=0;i<6;++i)
				{
				if(cubeMapFaces&(0x1U<<i))
					{
					glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT+i,level,internalFormat,levelSize[0],levelSize[1],0,externalFormat,dataType,levelData);
					levelData+=sliceSize;
					}
				}
			}
		
		/* Go to the next mipmap level: */
		for(int i=0;i<3;++i)
			levelSize[i]=(levelSize[i]+1U)>>1;
		}
	}
