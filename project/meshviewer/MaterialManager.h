/***********************************************************************
MaterialManager - Class to manage texture images and materials to render
polygonal models.
Copyright (c) 2009 Oliver Kreylos
***********************************************************************/

#ifndef MATERIALMANAGER_INCLUDED
#define MATERIALMANAGER_INCLUDED

#include <string>

#include "Texture.h"

class MaterialManager
	{
	/* Elements: */
	private:
	std::string imageNamePrefix; // The prefix of image names that will be replaced with the replace name
	std::string replaceName; // The name with which to replace the image name prefix
	
	/* Constructors and destructors: */
	public:
	MaterialManager(std::string sImageNamePrefix,std::string sReplaceName); // Creates an empty material manager with the given file name translation
	~MaterialManager(void); // Destroys the material manager
	
	/* Methods: */
	Texture loadTexture(std::string imageFileName) const; // Attempts to load a texture from the image file of the given name; applies name transformations etc.
	};

#endif
