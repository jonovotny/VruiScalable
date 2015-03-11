/***********************************************************************
MaterialManager - Class to manage texture images and materials to render
polygonal models.
Copyright (c) 2009 Oliver Kreylos
***********************************************************************/

#include "MaterialManager.h"

#include <string.h>
#include <Misc/ThrowStdErr.h>
#include <Misc/FileNameExtensions.h>
#include <Misc/File.h>
#include <Images/RGBImage.h>
#include <Images/RGBAImage.h>
#include <Images/IFFImageFileReader.h>
#include <Images/PNMImageFileReader.h>
#include <Images/TargaImageFileReader.h>
#include <Images/ReadImageFile.h>

/********************************
Methods of class MaterialManager:
********************************/

MaterialManager::MaterialManager(std::string sImageNamePrefix,std::string sReplaceName)
	:imageNamePrefix(sImageNamePrefix),
	 replaceName(sReplaceName)
	{
	}

MaterialManager::~MaterialManager(void)
	{
	}

Texture MaterialManager::loadTexture(std::string imageFileName) const
	{
	/* Remove the image name prefix from the image file name: */
	std::string::const_iterator ifnIt=imageFileName.begin();
	std::string::const_iterator inpIt;
	for(inpIt=imageNamePrefix.begin();inpIt!=imageNamePrefix.end();++ifnIt,++inpIt)
		if(ifnIt==imageFileName.end()||*ifnIt!=*inpIt)
			break;
	
	std::string imageName;
	if(inpIt==imageNamePrefix.end())
		{
		/* Concatenate the replace name and the image file name suffix: */
		imageName=replaceName;
		for(;ifnIt!=imageFileName.end();++ifnIt)
			{
			/* Replace backslash path separators with forward slashes: */
			if(*ifnIt=='\\')
				imageName.push_back('/');
			else
				imageName.push_back(*ifnIt);
			}
		}
	else
		{
		/* Use the given image name directly: */
		imageName=imageFileName;
		}
	
	/* Load the image file: */
	Images::RGBImage rgbImage;
	Images::RGBAImage rgbaImage;
	bool haveAlpha=false;
	const char* extPtr=Misc::getExtension(imageName.c_str());
	if(*extPtr=='\0')
		Misc::throwStdErr("MaterialManager::loadTexture: Image file name %s has no extension",imageFileName.c_str());
	else if(strcasecmp(extPtr,".iff")==0||strcasecmp(extPtr,".col")==0||strcasecmp(extPtr,".map")==0)
		{
		Misc::File imageFile(imageName.c_str(),"rb");
		imageFile.setEndianness(Misc::File::BigEndian);
		Images::IFFImageFileReader<Misc::File> reader(imageFile);
		rgbImage=reader.readImage<Images::RGBImage>();
		}
	else if(tolower(extPtr[1])=='p'&&(tolower(extPtr[2])=='g'||tolower(extPtr[2])=='n'||tolower(extPtr[2])=='p')&&tolower(extPtr[3])=='m'&&extPtr[4]=='\0')
		{
		Misc::File imageFile(imageName.c_str(),"rb");
		Images::PNMImageFileReader<Misc::File> reader(imageFile);
		rgbImage=reader.readImage<Images::RGBImage>();
		}
	else if(strcasecmp(extPtr,".tga")==0)
		{
		Misc::File imageFile(imageName.c_str(),"rb");
		Images::TargaImageFileReader<Misc::File> reader(imageFile);
		rgbaImage=reader.readImage<Images::RGBAImage>();
		haveAlpha=true;
		}
	else
		{
		rgbaImage=Images::readTransparentImageFile(imageName.c_str());
		haveAlpha=true;
		}
	
	/* Create a texture from the read image: */
	if(haveAlpha)
		{
		Texture result(Texture::Size(rgbaImage.getWidth(),rgbaImage.getHeight()),0x0,Texture::RGBA,0);
		result.setLevelData(0,rgbaImage.getPixels(),rgbaImage.getWidth()*rgbaImage.getHeight()*sizeof(Images::RGBAImage::Color));
		return result;
		}
	else
		{
		Texture result(Texture::Size(rgbImage.getWidth(),rgbImage.getHeight()),0x0,Texture::RGB,0);
		result.setLevelData(0,rgbImage.getPixels(),rgbImage.getWidth()*rgbImage.getHeight()*sizeof(Images::RGBImage::Color));
		return result;
		}
	}
