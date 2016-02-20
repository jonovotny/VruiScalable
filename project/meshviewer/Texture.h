/***********************************************************************
Texture - Class to represent textures in uncompressed and compressed
formats.
Copyright (c) 2007-2009 Oliver Kreylos
***********************************************************************/

#ifndef TEXTURE_INCLUDED
#define TEXTURE_INCLUDED

#include <GL/gl.h>

class Texture
	{
	/* Embedded classes: */
	public:
	struct Size // Class for texture sizes
		{
		/* Elements: */
		private:
		GLsizei size[3]; // Width, height, and depth
		
		/* Constructors and destructors: */
		public:
		Size(void)
			{
			for(int i=0;i<3;++i)
				size[i]=1;
			}
		Size(GLsizei sWidth,GLsizei sHeight,GLsizei sDepth =1)
			{
			size[0]=sWidth;
			size[1]=sHeight;
			size[2]=sDepth;
			}
		Size(const GLsizei sSize[3])
			{
			for(int i=0;i<3;++i)
				size[i]=sSize[i];
			}
		Size(const Size& source)
			{
			for(int i=0;i<3;++i)
				size[i]=source.size[i];
			}
		
		/* Methods: */
		Size& operator=(const Size& source)
			{
			for(int i=0;i<3;++i)
				size[i]=source.size[i];
			return *this;
			}
		friend bool operator==(const Size& size1,const Size& size2) // Equality operator
			{
			return size1.size[0]==size2.size[0]&&size1.size[0]==size2.size[0]&&size1.size[2]==size2.size[2];
			}
		friend bool operator!=(const Size& size1,const Size& size2) // Inequality operator
			{
			return size1.size[0]!=size2.size[0]||size1.size[0]!=size2.size[0]||size1.size[2]!=size2.size[2];
			}
		GLsizei operator[](int dimension) const // Returns on of the size's components
			{
			return size[dimension];
			}
		GLsizei& operator[](int dimension) // Ditto
			{
			return size[dimension];
			}
		};
	
	enum CubeMapFaces // Enumerated type for cube map faces
		{
		NO_CUBEMAP=0x00,
		POSITIVE_X=0x01,
		NEGATIVE_X=0x02,
		POSITIVE_Y=0x04,
		NEGATIVE_Y=0x08,
		POSITIVE_Z=0x10,
		NEGATIVE_Z=0x20,
		};
	
	enum StorageFormat // Enumerated type for texture storage formats
		{
		RGB, // Uncompressed texture with RGB values
		RGBA, // Uncompressed texture with RGB and alpha values
		DXT1, // Compressed texture in DXT1 format
		DXT2, // Compressed texture in DXT2 format
		DXT3, // Compressed texture in DXT3 format
		DXT4, // Compressed texture in DXT4 format
		DXT5, // Compressed texture in DXT5 format
		RXGB // Compressed texture in RXGB format (same as DXT5 with swizzled components)
		};
	
	/* Elements: */
	private:
	Size size; // Size of base texture image
	unsigned int cubeMapFaces; // Bitmask of cube map faces stored in the texture, or 0x0 if not a cube map
	StorageFormat storageFormat; // Storage format of texture images
	unsigned int maxMipMapLevel; // Number of smallest mipmap level, automatically cut off at full pyramid
	unsigned char* data; // Pointer to shared raw texture image data (pointer is unsigned int-aligned and reference-counted)
	
	/* Private methods: */
	static unsigned char* allocData(size_t size) // Allocates texture image data to hold the given number of bytes
		{
		/* Align the size to unsigned int: */
		size_t uintSize=(size+sizeof(unsigned int)-1)/sizeof(unsigned int);
		
		/* Allocate the storage, including the reference counter: */
		unsigned int* uintData=new unsigned int[uintSize+1];
		
		/* Initialize the reference counter to 0: */
		uintData[0]=0;
		
		/* Return the aligned storage: */
		return reinterpret_cast<unsigned char*>(uintData+1);
		}
	void ref(unsigned char* newData) // Changes object's texture image data pointer to the given properly aligned pointer
		{
		if(data!=0)
			{
			/* Unreference the current texture image data: */
			unsigned int* oldUintData=reinterpret_cast<unsigned int*>(data)-1;
			if(--*oldUintData==0)
				{
				/* Delete the old texture image data: */
				delete[] oldUintData;
				}
			}
		if(newData!=0)
			{
			/* Reference the new texture image data: */
			unsigned int* newUintData=reinterpret_cast<unsigned int*>(newData)-1;
			++*newUintData;
			}
		data=newData;
		}
	size_t calcImageSize(const Size& imageSize) const; // Calculates byte size of a slice or cube map face of the given image using the current storage format
	
	/* Constructors and destructors: */
	public:
	Texture(void) // Creates an uninitialized texture
		:data(0)
		{
		}
	Texture(const Size& sSize,unsigned int sCubeMapFaces,StorageFormat sStorageFormat,unsigned int sMaxMipMapLevel =~0x0); // Creates uninitialized texture that can hold the given texture image(s); if no maximum mipmap level is given, creates full pyramid
	Texture(const Texture& source) // Copy constructor
		:size(source.size),
		 cubeMapFaces(source.cubeMapFaces),
		 storageFormat(source.storageFormat),
		 maxMipMapLevel(source.maxMipMapLevel),
		 data(0)
		{
		/* Reference source texture image data: */
		ref(source.data);
		}
	~Texture(void) // Destroys a texture
		{
		/* Unreference current texture image data: */
		ref(0);
		}
	
	/* Methods: */
	static bool initExtensions(void); // Initializes all OpenGL extensions required by textures; returns false if insufficient OpenGL support
	Texture& operator=(const Texture& source) // Assignment operator
		{
		if(this!=&source)
			{
			/* Copy source texture size and format: */
			size=source.size;
			cubeMapFaces=source.cubeMapFaces;
			storageFormat=source.storageFormat;
			maxMipMapLevel=source.maxMipMapLevel;
			
			/* Reference source texture image data: */
			ref(source.data);
			}
		return *this;
		}
	const Size& getSize(void) const // Returns base image size
		{
		return size;
		}
	GLsizei getWidth(void) const // Returns base image width
		{
		return size[0];
		}
	GLsizei getHeight(void) const // Returns base image height
		{
		return size[1];
		}
	GLsizei getDepth(void) const // Returns base image depth
		{
		return size[2];
		}
	bool isCubeMap(void) const // Returns true if the texture holds a (partial) cube map
		{
		return cubeMapFaces!=NO_CUBEMAP;
		}
	unsigned int getCubeMapFaces(void) const // Returns bit mask of cube map faces stored in texture
		{
		return cubeMapFaces;
		}
	StorageFormat getStorageFormat(void) const // Returns storage format
		{
		return storageFormat;
		}
	unsigned int getMaxMipMapLevel(void) const // Returns number of smallest mipmap level
		{
		return maxMipMapLevel;
		}
	void setLevelData(unsigned int level,const void* levelData,size_t levelDataSize); // Uploads raw image data for the given mipmap level into the texture
	GLenum glGetTextureTarget(void) const; // Returns the OpenGL texture target used by this texture
	void glTexImage(void) const; // Uploads the texture to OpenGL, using an appropriate texture target and internal format
	};

#endif
