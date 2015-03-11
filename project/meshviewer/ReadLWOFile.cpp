/***********************************************************************
ReadLWOFile - Function to read polygonal models from 3D model files in
Lightwave object file format.
Copyright (c) 2008-2011 Oliver Kreylos
***********************************************************************/

#include "ReadLWOFile.h"

// DEBUGGING
#include <iostream>

#include <string>
#include <vector>
#include <stdexcept>
#include <Misc/SelfDestructPointer.h>
#include <Misc/ThrowStdErr.h>
#include <Misc/Timer.h>
#include <Misc/HashTable.h>
#include <IO/File.h>
#include <Cluster/OpenFile.h>
#include <GL/gl.h>
#include <GL/GLColor.h>
#include <GL/GLColorOperations.h>

#include "MaterialManager.h"
#include "PhongMaterial.h"
#include "Texture.h"
#include "PhongTextureMaterial.h"
#include "MeshVertex.h"
#include "PolygonMesh.h"
#include "TexCoordCalculator.h"
#include "IFFChunk.h"
#include "TriangleSet.h"

namespace {

/**************
Helper classes:
**************/

typedef MeshVertex<float> MyMeshVertex;
typedef TriangleSet<MyMeshVertex> MyTriangleSet;
typedef PolygonMesh<MyMeshVertex> MyPolygonMesh;
typedef MyPolygonMesh::Scalar Scalar;
typedef MyPolygonMesh::Point Point;
typedef MyPolygonMesh::Vector Vector;
typedef MyPolygonMesh::TPoint TPoint;
typedef MyPolygonMesh::Card Card;
typedef GLColor<GLfloat,3> Color; // Type for RGB colors

class TextureMap:public TexCoordCalculator<MyMeshVertex> // Structure describing a texture map
	{
	/* Embedded classes: */
	public:
	enum ProjectionMode // Enumerated type for texture projection modes
		{
		UNKNOWN,
		PLANAR,
		CYLINDRICAL,
		SPHERICAL,
		CUBIC,
		UVMAP
		};
	
	enum TextureFlags // Enumerated type for texture flags
		{
		NONE=0x0,
		X_AXIS=0x1,
		Y_AXIS=0x2,
		Z_AXIS=0x4,
		AXIS_MODE=0x7,
		WORLD_COORDS=0x8,
		NEGATIVE_IMAGE=0x10,
		PIXEL_BLENDING=0x20,
		ANTIALIASING=0x40
		};
	
	enum WrapMode // Enumerated type for texture wrapping modes
		{
		BLACK=0,
		CLAMP=1,
		REPEAT=2,
		REPEAT_MIRROR=3
		};
	
	/* Elements: */
	std::string imageName; // Name of the texture image
	ProjectionMode projectionMode; // Texture projection mode
	unsigned int flags; // Texture flags bit field
	unsigned int wrapModes[2]; // Texture wrapping modes for width and height, respectively
	Vector size; // Texture size
	Point center; // Texture center
	Vector falloff; // ??
	Vector velocity; // ??
	Color color; // Texture color for color textures
	float value; // Texture value
	
	/* Constructors and destructors: */
	TextureMap(void) // Creates texture with default settings
		:projectionMode(UNKNOWN),
		 flags(NONE),
		 color(0.0f,0.0f,0.0f),
		 value(0.0f)
		{
		for(int i=0;i<2;++i)
			wrapModes[i]=REPEAT;
		}
	
	/* Methods from TexCoordCalculator: */
	virtual TPoint calcTexCoord(const Point& position) const
		{
		/* Scale the point position: */
		Point sp;
		for(int i=0;i<3;++i)
			sp[i]=(position[i]-center[i])/size[i];
		
		TPoint result;
		switch(projectionMode)
			{
			case PLANAR:
				switch(flags&AXIS_MODE)
					{
					case X_AXIS:
						result[0]=sp[1]+Scalar(0.5);
						result[1]=sp[2]+Scalar(0.5);
						break;
					
					case Y_AXIS:
						result[0]=sp[0]+Scalar(0.5);
						result[1]=sp[1]+Scalar(0.5);
						break;
					
					case Z_AXIS:
						result[0]=sp[0]+Scalar(0.5);
						result[1]=sp[2]+Scalar(0.5);
						break;
					}
				break;
			
			case CYLINDRICAL:
				switch(flags&AXIS_MODE)
					{
					case X_AXIS:
						result[0]=Math::atan2(sp[1],sp[2])/(Scalar(2)*Math::Constants<Scalar>::pi)+Scalar(0.5);
						result[1]=sp[0]+Scalar(0.5);
						break;
					
					case Y_AXIS:
						result[0]=Math::atan2(sp[0],sp[1])/(Scalar(2)*Math::Constants<Scalar>::pi)+Scalar(0.5);
						result[1]=sp[2]+Scalar(0.5);
						break;
					
					case Z_AXIS:
						result[0]=Math::atan2(sp[0],sp[2])/(Scalar(2)*Math::Constants<Scalar>::pi)+Scalar(0.5);
						result[1]=sp[1]+Scalar(0.5);
						break;
					}
				break;
			
			case SPHERICAL:
				switch(flags&AXIS_MODE)
					{
					case X_AXIS:
						result[0]=Math::atan2(sp[1],sp[2])/(Scalar(2)*Math::Constants<Scalar>::pi)+Scalar(0.5);
						result[1]=Math::atan2(sp[0],Math::sqrt(sp[1]*sp[1]+sp[2]*sp[2]))/Math::Constants<Scalar>::pi+Scalar(0.5);
						break;
					
					case Y_AXIS:
						result[0]=Math::atan2(sp[0],sp[1])/(Scalar(2)*Math::Constants<Scalar>::pi)+Scalar(0.5);
						result[1]=Math::atan2(sp[2],Math::sqrt(sp[0]*sp[0]+sp[1]*sp[1]))/Math::Constants<Scalar>::pi+Scalar(0.5);
						break;
					
					case Z_AXIS:
						result[0]=Math::atan2(sp[0],sp[2])/(Scalar(2)*Math::Constants<Scalar>::pi)+Scalar(0.5);
						result[1]=Math::atan2(sp[1],Math::sqrt(sp[0]*sp[0]+sp[2]*sp[2]))/Math::Constants<Scalar>::pi+Scalar(0.5);
						break;
					}
				break;
			
			case CUBIC:
				{
				if(Math::abs(sp[0])>=Math::abs(sp[1])&&Math::abs(sp[0])>=Math::abs(sp[2]))
					{
					/* sp[0] is biggest: */
					result[0]=sp[1]/(sp[0]*Scalar(2))+Scalar(0.5);
					result[1]=sp[2]/(sp[0]*Scalar(2))+Scalar(0.5);
					}
				else if(Math::abs(sp[1])>=Math::abs(sp[2]))
					{
					/* sp[1] is biggest: */
					result[0]=sp[0]/(sp[1]*Scalar(2))+Scalar(0.5);
					result[1]=sp[2]/(sp[1]*Scalar(2))+Scalar(0.5);
					}
				else
					{
					/* sp[2] is biggest: */
					result[0]=sp[0]/(sp[2]*Scalar(2))+Scalar(0.5);
					result[1]=sp[1]/(sp[2]*Scalar(2))+Scalar(0.5);
					}
				break;
				}
			}
		
		return result;
		}
	};

struct Surface // Structure describing a surface
	{
	/* Embedded classes: */
	public:
	enum SurfaceFlags // Enumerated type for surface flags
		{
		NONE=0x0,
		LUMINOUS=0x1,
		OUTLINE=0x2,
		SMOOTHING=0x4,
		COLOR_HIGHLIGHTS=0x8,
		COLOR_FILTER=0x10,
		OPAQUE_EDGE=0x20,
		TRANSPARENT_EDGE=0x40,
		SHARP_TERMINATOR=0x80,
		DOUBLE_SIDED=0x100,
		ADDITIVE=0x200,
		SHADOW_ALPHA=0x400
		};
	
	/* Elements: */
	std::string name; // Surface's name
	Card index; // Surface's index as referenced by mesh faces
	Color color;
	unsigned int flags;
	float lumi;
	float diff;
	float spec;
	float refl;
	float tran;
	float trnl;
	float glos;
	Scalar creaseAngle;
	TextureMap colorMap;
	TextureMap diffuseMap;
	TextureMap specularMap;
	TextureMap reflectionMap;
	TextureMap transparentMap;
	TextureMap luminosityMap;
	TextureMap bumpMap;
	Scalar bumpMapAmplitude;
	
	/* Constructors and destructors: */
	Surface(const std::string& sName,Card sIndex)
		:name(sName),index(sIndex),
		 color(0.0f,0.0f,0.0f),flags(0x0),
		 lumi(0.0f),diff(1.0f),spec(0.0f),refl(0.0f),tran(0.0f),trnl(0.0f),
		 glos(0.0f),
		 creaseAngle(0),
		 bumpMapAmplitude(0)
		{
		}
	};

/****************
Helper functions:
****************/

template <class DataSourceParam>
inline
Point
readPoint(IFFChunk<DataSourceParam>& chunk)
	{
	Point result;
	result[0]=Scalar(chunk.template read<float>());
	result[2]=Scalar(chunk.template read<float>()); // Swap y and z coordinates to flip
	result[1]=Scalar(chunk.template read<float>()); // coordinate system's handedness
	return result;
	}

template <class DataSourceParam>
inline
Vector
readVector(IFFChunk<DataSourceParam>& chunk)
	{
	Vector result;
	result[0]=Scalar(chunk.template read<float>());
	result[2]=Scalar(chunk.template read<float>()); // Swap y and z coordinates to flip
	result[1]=Scalar(chunk.template read<float>()); // coordinate system's handedness
	return result;
	}

template <class DataSourceParam>
inline
Color
readColor3ub(IFFChunk<DataSourceParam>& chunk)
	{
	GLColor<GLubyte,3> result;
	chunk.template read<unsigned char>(result.getRgba(),3);
	return Color(result);
	}

template <class DataSourceParam>
inline
Color
readColor3f(IFFChunk<DataSourceParam>& chunk)
	{
	Color result;
	chunk.template read<float>(result.getRgba(),3);
	return result;
	}

/*************************************************************
Function to parse Lightwave object files prior to version 6.0:
*************************************************************/

template <class DataSourceParam>
inline
void
readLWOBFile(
	IFFChunk<DataSourceParam>& formChunk,
	MaterialManager& materialManager,
	MyTriangleSet& triangleSet)
	{
	/* Embedded classes: */
	typedef IFFChunk<DataSourceParam> Chunk;
	
	/* Create a polygon mesh: */
	MyPolygonMesh mesh;
	
	/* Store a list of materials: */
	std::vector<MaterialPointer> surfaceMaterials;
	
	/* Read the form chunk's body: */
	std::vector<Surface> surfaces; // List of surfaces
	while(formChunk.getRestSize()>0)
		{
		/* Read the next chunk: */
		Chunk chunk(formChunk);
		
		/* Process the chunk: */
		if(chunk.isChunk("SRFS"))
			{
			/* Read all surface names from the chunk: */
			while(chunk.getRestSize()>0)
				surfaces.push_back(Surface(chunk.readString(),Card(surfaces.size())));
			}
		else if(chunk.isChunk("PNTS"))
			{
			/* Read all vertices from the chunk: */
			while(chunk.getRestSize()>0)
				{
				/* Store the next point: */
				mesh.addVertex(readPoint(chunk));
				}
			}
		else if(chunk.isChunk("POLS"))
			{
			/* Create a polygon vertex index buffer: */
			Card maxNumVertices=10;
			Card* vertexIndices=new Card[maxNumVertices];
			
			/* Read all polygons from the chunk: */
			while(chunk.getRestSize()>0)
				{
				/* Read the polygon's vertex indices: */
				Card numVertices=Card(chunk.template read<unsigned short>());
				if(maxNumVertices<numVertices)
					{
					/* Re-allocate the vertex index buffer: */
					delete[] vertexIndices;
					maxNumVertices=numVertices+2;
					vertexIndices=new Card[maxNumVertices];
					}
				for(Card i=0;i<numVertices;++i)
					{
					// vertexIndices[i]=Card(chunk.template read<unsigned short>());
					vertexIndices[numVertices-1-i]=Card(chunk.template read<unsigned short>()); // Flip vertex order to go from clockwise to counter-clockwise face orientation
					}
				
				/* Read the surface index: */
				int surfaceNumber=chunk.template read<signed short>();
				
				/* Check if the polygon has detail polygons: */
				if(surfaceNumber<0)
					{
					/* Read (and ignore) all detail polygons: */
					unsigned int numDetailPolygons=chunk.template read<unsigned short>();
					for(unsigned int i=0;i<numDetailPolygons;++i)
						{
						/* Read the number of vertices in the detail polygon: */
						unsigned int numVertices=chunk.template read<unsigned short>();
						
						/* Skip the detail polygon's vertices: */
						for(unsigned int i=0;i<numVertices;++i)
							chunk.template read<unsigned short>();
						
						/* Skip the detail polygon's surface number: */
						chunk.template read<signed short>();
						}
					
					surfaceNumber=-surfaceNumber;
					}
				
				/* Add the polygon and set its zero-based surface index: */
				Card faceIndex=mesh.addFace(numVertices,vertexIndices);
				if(faceIndex!=MyPolygonMesh::invalidIndex)
					mesh.setFaceSurface(faceIndex,Card(surfaceNumber-1));
				}
			
			/* Clean up: */
			delete[] vertexIndices;
			}
		else if(chunk.isChunk("SURF"))
			{
			/* Find the surface's index as referenced by polygons: */
			std::string surfaceName=chunk.readString();
			Card surfaceIndex=surfaces.size();
			for(Card i=0;i<surfaces.size();++i)
				if(surfaces[i].name==surfaceName)
					{
					surfaceIndex=i;
					break;
					}
			
			/*****************************
			Read the surface's parameters:
			*****************************/
			
			Surface& surface=surfaces[surfaceIndex];
			TextureMap* tmap=0;
			bool haveDiffuseMap=false;
			
			/* Process all subchunks of the chunk: */
			while(chunk.getRestSize()>0)
				{
				/* Read the next subchunk: */
				Chunk surfChunk(chunk,true);
				
				/* Process the subchunk: */
				if(surfChunk.isChunk("COLR"))
					{
					/* Read the surface color: */
					surface.color=readColor3ub(surfChunk);
					}
				else if(surfChunk.isChunk("FLAG"))
					{
					/* Read the surface flags: */
					surface.flags=surfChunk.template read<unsigned short>();
					}
				else if(surfChunk.isChunk("LUMI"))
					surface.lumi=float(surfChunk.template read<unsigned short>())/256.0f;
				else if(surfChunk.isChunk("VLUM"))
					surface.lumi=surfChunk.template read<float>();
				else if(surfChunk.isChunk("DIFF"))
					surface.diff=float(surfChunk.template read<unsigned short>())/256.0f;
				else if(surfChunk.isChunk("VDIF"))
					surface.diff=surfChunk.template read<float>();
				else if(surfChunk.isChunk("SPEC"))
					surface.spec=float(surfChunk.template read<unsigned short>())/256.0f;
				else if(surfChunk.isChunk("VSPC"))
					surface.spec=surfChunk.template read<float>();
				else if(surfChunk.isChunk("REFL"))
					surface.refl=float(surfChunk.template read<unsigned short>())/256.0f;
				else if(surfChunk.isChunk("VRFL"))
					surface.refl=surfChunk.template read<float>();
				else if(surfChunk.isChunk("TRAN"))
					surface.tran=float(surfChunk.template read<unsigned short>())/256.0f;
				else if(surfChunk.isChunk("VTRN"))
					surface.tran=surfChunk.template read<float>();
				else if(surfChunk.isChunk("GLOS"))
					{
					/* Read the surface glossiness: */
					surface.glos=float(surfChunk.template read<unsigned short>());
					}
				else if(surfChunk.isChunk("SMAN"))
					{
					/* Read the surface crease angle: */
					surface.creaseAngle=Scalar(surfChunk.template read<float>());
					}
				else if(surfChunk.isChunk("CTEX"))
					{
					/* Activate the color texture: */
					tmap=&surface.colorMap;
					}
				else if(surfChunk.isChunk("DTEX"))
					{
					/* Activate the diffuse texture: */
					tmap=&surface.diffuseMap;
					std::string textureType=surfChunk.readString();
					if(textureType=="Planar Image Map")
						tmap->projectionMode=TextureMap::PLANAR;
					else if(textureType=="Cylindrical Image Map")
						tmap->projectionMode=TextureMap::CYLINDRICAL;
					else if(textureType=="Spherical Image Map")
						tmap->projectionMode=TextureMap::SPHERICAL;
					else if(textureType=="Cubic Image Map")
						tmap->projectionMode=TextureMap::CUBIC;
					else
						std::cerr<<"Unrecognized texture type "<<textureType<<std::endl;
					haveDiffuseMap=true;
					}
				else if(surfChunk.isChunk("STEX"))
					{
					/* Activate the specular texture: */
					tmap=&surface.specularMap;
					}
				else if(surfChunk.isChunk("RTEX"))
					{
					/* Activate the reflection texture: */
					tmap=&surface.reflectionMap;
					}
				else if(surfChunk.isChunk("TTEX"))
					{
					/* Activate the transparent texture: */
					tmap=&surface.transparentMap;
					}
				else if(surfChunk.isChunk("LTEX"))
					{
					/* Activate the luminosity texture: */
					tmap=&surface.luminosityMap;
					}
				else if(surfChunk.isChunk("BTEX"))
					{
					/* Activate the bump texture: */
					tmap=&surface.bumpMap;
					}
				else if(surfChunk.isChunk("TIMG")&&tmap!=0)
					{
					/* Read the texture image name: */
					tmap->imageName=surfChunk.readString();
					}
				else if(surfChunk.isChunk("TFLG")&&tmap!=0)
					{
					/* Read the texture flags: */
					tmap->flags=surfChunk.template read<unsigned short>();
					}
				else if(surfChunk.isChunk("TWRP")&&tmap!=0)
					{
					/* Read the texture wrapping modes: */
					for(int i=0;i<2;++i)
						tmap->wrapModes[i]=surfChunk.template read<unsigned short>();
					}
				else if(surfChunk.isChunk("TSIZ")&&tmap!=0)
					{
					/* Read the texture size: */
					tmap->size=readVector(surfChunk);
					}
				else if(surfChunk.isChunk("TCTR")&&tmap!=0)
					{
					/* Read the texture center: */
					tmap->center=readPoint(surfChunk);
					}
				else if(surfChunk.isChunk("TFAL")&&tmap!=0)
					{
					/* Read the texture fall-off: */
					tmap->falloff=readVector(surfChunk);
					}
				else if(surfChunk.isChunk("TVEL")&&tmap!=0)
					{
					/* Read the texture velocity: */
					tmap->velocity=readVector(surfChunk);
					}
				else if(surfChunk.isChunk("TCLR")&&tmap!=0)
					{
					/* Read the texture color: */
					tmap->color=readColor3ub(surfChunk);
					}
				else if(surfChunk.isChunk("TVAL")&&tmap!=0)
					{
					/* Read the texture value: */
					tmap->value=float(surfChunk.template read<unsigned short>())/256.0f;
					}
				else if(surfChunk.isChunk("TAMP"))
					{
					/* Read the bump map amplitude value: */
					surface.bumpMapAmplitude=surfChunk.template read<float>();
					}
				
				/* Finish reading the subchunk: */
				surfChunk.closeChunk();
				}
			
			/* Set the triangle set's surface material parameters: */
			GLMaterial material;
			material.ambient=GLMaterial::Color(surface.color);
			material.diffuse=GLMaterial::Color(surface.color);
			material.specular=GLMaterial::Color(surface.color*surface.spec);
			material.shininess=surface.glos/8.0f;
			material.emission=GLMaterial::Color(surface.color*surface.lumi);
			MaterialPointer surfaceMaterial;
			if(haveDiffuseMap)
				{
				try
					{
					/* Load the diffuse texture and create a Phong texture material: */
					Texture diffuseMap=materialManager.loadTexture(surface.diffuseMap.imageName);
					surfaceMaterial=new PhongTextureMaterial(material,diffuseMap);
					}
				catch(std::runtime_error err)
					{
					/* Fall back to using a standard Phong material: */
					std::cerr<<"Ignoring texture due to exception "<<err.what()<<std::endl;
					surfaceMaterial=new PhongMaterial(material);
					}
				}
			else
				surfaceMaterial=new PhongMaterial(material);
			
			/* Store the material: */
			while(surfaceMaterials.size()<=surfaceIndex)
				surfaceMaterials.push_back(0);
			surfaceMaterials[surfaceIndex]=surfaceMaterial;
			}
		
		/* Finish reading the chunk: */
		chunk.closeChunk();
		}
	
	/*************************
	Finalize the polygon mesh:
	*************************/
	
	/* Calculate all vertex texture coordinates: */
	std::vector<const TexCoordCalculator<MyMeshVertex>*> texCoordCalculators;
	for(std::vector<Surface>::const_iterator sIt=surfaces.begin();sIt!=surfaces.end();++sIt)
		texCoordCalculators.push_back(&sIt->diffuseMap);
	mesh.calcVertexTexCoords(texCoordCalculators);
	
	/* Find all crease edges: */
	std::cout<<"Generating crease edges..."<<std::flush;
	Misc::Timer smoothingTimer;
	std::vector<Scalar> creaseAngles;
	creaseAngles.reserve(surfaces.size());
	for(std::vector<Surface>::const_iterator sIt=surfaces.begin();sIt!=surfaces.end();++sIt)
		creaseAngles.push_back(sIt->creaseAngle);
	mesh.findCreaseEdges(creaseAngles);
	smoothingTimer.elapse();
	std::cout<<" done in "<<smoothingTimer.getTime()*1000.0<<" ms"<<std::endl;
	
	/* Calculate all vertex normal vectors: */
	std::cout<<"Generating normal vectors..."<<std::flush;
	Misc::Timer normalTimer;
	mesh.calcVertexNormals();
	normalTimer.elapse();
	std::cout<<" done in "<<normalTimer.getTime()*1000.0<<" ms"<<std::endl;
	
	/* Triangulate the polygon mesh independently for each surface: */
	for(Card surfaceIndex=0;surfaceIndex<surfaces.size();++surfaceIndex)
		{
		/* Add the surface's material to the triangle set: */
		triangleSet.addMaterial(surfaceMaterials[surfaceIndex]);
		
		/* Triangulate the specific surface: */
		mesh.triangulateSurface(triangleSet,surfaceIndex);
		
		/* Finish the submesh: */
		triangleSet.setSubMeshMaterial(surfaceIndex);
		triangleSet.finishSubMesh();
		}
	}

/**************************************************************
Function to parse Lightwave object files of version 6.0 and up:
**************************************************************/

template <class DataSourceParam>
inline
void
readLWO2File(
	IFFChunk<DataSourceParam>& formChunk,
	MaterialManager& materialManager,
	MyTriangleSet& triangleSet)
	{
	/* Embedded classes: */
	typedef IFFChunk<DataSourceParam> Chunk;
	
	/* Create a polygon mesh: */
	MyPolygonMesh mesh;
	
	/* Create a hash table of texture images: */
	Misc::HashTable<unsigned int,std::string> clipNames(17);
	
	/* Store a list of materials: */
	std::vector<MaterialPointer> surfaceMaterials;
	
	/* Read the form chunk's body: */
	std::vector<Card> faceIndices; // List of face indices created from the most recent POLS chunk
	std::vector<std::string> tags; // List of tags from the most recent TAGS chunk
	std::vector<Surface> surfaces; // List of surfaces
	Card layerIndex=0;
	Point layerPivot=Point::origin;
	Card layerFirstVertexIndex=0;
	Card layerFirstFaceIndex=0;
	Card pointsFirstVertexIndex=0;
	Card polygonsFirstFaceIndex=0;
	while(formChunk.getRestSize()>0)
		{
		/* Read the next chunk: */
		Chunk chunk(formChunk);
		
		/* Process the chunk: */
		if(chunk.isChunk("LAYR"))
			{
			/* Read the definition of the current layer: */
			layerIndex=chunk.template read<unsigned short int>();
			unsigned int layerFlags=chunk.template read<unsigned short int>();
			layerPivot=readPoint(chunk);
			std::string layerName=chunk.readString();
			Card layerParent=chunk.getRestSize()>=2?chunk.template read<unsigned short int>():MyPolygonMesh::invalidIndex;
			std::cout<<"LAYR of name "<<layerName<<std::endl;
			
			/* Remember the start indices for this layer's points and polygons: */
			layerFirstVertexIndex=mesh.getNumVertices();
			layerFirstFaceIndex=mesh.getNumFaces();
			}
		else if(chunk.isChunk("PNTS"))
			{
			/* Remember the point start indices for this chunk: */
			pointsFirstVertexIndex=mesh.getNumVertices();
			
			/* Read all vertices from the chunk: */
			Card numPoints=0;
			while(chunk.getRestSize()>0)
				{
				/* Store the next point: */
				mesh.addVertex(readPoint(chunk));
				++numPoints;
				}
			std::cout<<"PNTS, read "<<numPoints<<" points"<<std::endl;
			}
		else if(chunk.isChunk("VMAP"))
			{
			/* Read the mapping type, dimension, and name: */
			char type[4];
			chunk.template read<char>(type,4);
			unsigned int dimension=chunk.template read<short unsigned int>();
			std::string name=chunk.readString();
			
			std::cout<<"VMAP of type "<<std::string(type,type+4)<<" and name "<<name<<std::endl;
			if(memcmp(type,"NORM",4)==0&&dimension==3)
				{
				/* Read all vertex mappings: */
				std::cout<<"Reading vertex normal vectors"<<std::endl;
				while(chunk.getRestSize()>0)
					{
					/* Read the next vertex normal vector: */
					Card vertexIndex=chunk.readVarInt();
					MyMeshVertex& v=mesh.getVertex(pointsFirstVertexIndex+vertexIndex);
					for(unsigned int i=0;i<3;++i)
						v.normal[i]=Scalar(chunk.template read<float>());
					
					/* Normalize the normal just in case: */
					v.normal.normalize();
					}
				}
			else if(memcmp(type,"TXUV",4)==0&&dimension==2)
				{
				/* Read all vertex mappings: */
				std::cout<<"Reading vertex texture coordinates"<<std::endl;
				while(chunk.getRestSize()>0)
					{
					/* Read the next vertex texture coordinate: */
					Card vertexIndex=chunk.readVarInt();
					MyMeshVertex& v=mesh.getVertex(pointsFirstVertexIndex+vertexIndex);
					for(unsigned int i=0;i<2;++i)
						v.texCoord[i]=Scalar(chunk.template read<float>());
					}
				}
			}
		else if(chunk.isChunk("POLS"))
			{
			/* Read the polygon type: */
			char type[4];
			chunk.template read<char>(type,4);
			std::cout<<"POLS of type "<<std::string(type,type+4)<<std::endl;
			
			if(memcmp(type,"FACE",4)==0||memcmp(type,"PTCH",4)==0)
				{
				/* Remember the first polygon index for this chunk: */
				polygonsFirstFaceIndex=mesh.getNumFaces();
				
				/* Create a polygon vertex index buffer: */
				Card maxNumVertices=10;
				Card* vertexIndices=new Card[maxNumVertices];
				
				/* Read all polygons from the chunk: */
				faceIndices.clear();
				while(chunk.getRestSize()>0)
					{
					/* Read the polygon's vertex indices: */
					Card numVertices=Card(chunk.template read<unsigned short>())&0x3ffU; // Mask out flag bits
					if(maxNumVertices<numVertices)
						{
						/* Re-allocate the vertex index buffer: */
						delete[] vertexIndices;
						maxNumVertices=numVertices+2;
						vertexIndices=new Card[maxNumVertices];
						}
					for(Card i=0;i<numVertices;++i)
						vertexIndices[numVertices-1-i]=Card(chunk.readVarInt())+pointsFirstVertexIndex; // Flip vertex order to go from clockwise to counter-clockwise face orientation
					
					/* Add the polygon and store its face index: */
					faceIndices.push_back(mesh.addFace(numVertices,vertexIndices));
					}
				
				/* Clean up: */
				delete[] vertexIndices;
				
				std::cout<<"Read "<<faceIndices.size()<<" polygons"<<std::endl;
				}
			}
		else if(chunk.isChunk("TAGS"))
			{
			/* Read all tags: */
			tags.clear();
			while(chunk.getRestSize()>0)
				tags.push_back(chunk.readString());
			}
		else if(chunk.isChunk("PTAG"))
			{
			/* Read the polygon tag type: */
			char type[4];
			chunk.template read<char>(type,4);
			std::cout<<"PTAG of type "<<std::string(type,type+4)<<std::endl;
			
			if(memcmp(type,"SURF",4)==0)
				{
				/* Read the polygon surface indices: */
				while(chunk.getRestSize()>0)
					{
					/* Read the polygon and surface index: */
					Card polygonIndex=chunk.readVarInt();
					Card surfaceIndex=chunk.template read<short unsigned int>();
					
					/* Set the polygon's surface in the mesh: */
					if(polygonIndex<faceIndices.size()&&faceIndices[polygonIndex]!=MyPolygonMesh::invalidIndex)
						mesh.setFaceSurface(faceIndices[polygonIndex],surfaceIndex);
					}
				}
			}
		else if(chunk.isChunk("VMAD"))
			{
			/* Read the face vertex mapping type: */
			char type[4];
			chunk.template read<char>(type,4);
			std::cout<<"VMAD of type "<<std::string(type,type+4)<<std::endl;
			}
		else if(chunk.isChunk("CLIP"))
			{
			/* Read the clip index: */
			unsigned int clipIndex=chunk.template read<unsigned int>();
			
			/* Process all subchunks of the chunk: */
			while(chunk.getRestSize()>0)
				{
				/* Read the next subchunk: */
				Chunk clipChunk(chunk,true);
				
				/* Process the subchunk: */
				if(clipChunk.isChunk("STIL"))
					{
					/* Read the texture image name and store it in the clip name map: */
					clipNames[clipIndex]=clipChunk.readString();
					}
				
				/* Finish reading the subchunk: */
				clipChunk.closeChunk();
				}
			}
		else if(chunk.isChunk("SURF"))
			{
			/* Find the surface's index as referenced by polygons: */
			std::string surfaceName=chunk.readString();
			Card surfaceTagIndex=tags.size();
			for(Card i=0;i<tags.size();++i)
				if(tags[i]==surfaceName)
					{
					surfaceTagIndex=i;
					break;
					}
			
			/*****************************
			Read the surface's parameters:
			*****************************/
			
			surfaces.push_back(Surface(surfaceName,surfaceTagIndex));
			Surface& surface=surfaces.back();
			std::string sourceSurface=chunk.readString();
			TextureMap* tmap=0;
			
			/* Process all subchunks of the chunk: */
			while(chunk.getRestSize()>0)
				{
				/* Read the next subchunk: */
				Chunk surfChunk(chunk,true);
				
				/* Process the subchunk: */
				if(surfChunk.isChunk("COLR"))
					{
					/* Read the surface color: */
					surface.color=readColor3f(surfChunk);
					}
				else if(surfChunk.isChunk("LUMI"))
					surface.lumi=surfChunk.template read<float>();
				else if(surfChunk.isChunk("DIFF"))
					surface.diff=surfChunk.template read<float>();
				else if(surfChunk.isChunk("SPEC"))
					surface.spec=surfChunk.template read<float>();
				else if(surfChunk.isChunk("REFL"))
					surface.refl=surfChunk.template read<float>();
				else if(surfChunk.isChunk("TRAN"))
					surface.tran=surfChunk.template read<float>();
				else if(surfChunk.isChunk("TRNL"))
					surface.trnl=surfChunk.template read<float>();
				else if(surfChunk.isChunk("GLOS"))
					{
					/* Read the surface glossiness: */
					surface.glos=Math::pow(2.0f,surfChunk.template read<float>()*10.0f+2.0f);
					}
				else if(surfChunk.isChunk("SIDE"))
					{
					/* Read the surface sidedness: */
					unsigned int sidedness=surfChunk.template read<short unsigned int>();
					if(sidedness==3U)
						surface.flags|=Surface::DOUBLE_SIDED;
					std::cout<<"Surface is double-sided"<<std::endl;
					}
				else if(surfChunk.isChunk("SMAN"))
					{
					/* Read the surface crease angle: */
					surface.creaseAngle=Scalar(surfChunk.template read<float>());
					std::cout<<"Got crease angle "<<surface.creaseAngle<<std::endl;
					}
				else if(surfChunk.isChunk("BLOK"))
					{
					/* Read the blok's header chunk: */
					Chunk headerChunk(surfChunk,true);
					if(headerChunk.isChunk("IMAP"))
						{
						/* Skip the ordinal string: */
						headerChunk.readString();
						
						/* Process header subchunks: */
						char mapChannel[4];
						while(headerChunk.getRestSize()>0)
							{
							/* Read the next subchunk: */
							Chunk attrChunk(headerChunk,true);
							
							/* Process the subchunk: */
							if(attrChunk.isChunk("CHAN"))
								{
								/* Read the channel ID: */
								attrChunk.template read<char>(mapChannel,4);
								}
							
							/* Finish reading the subchunk: */
							attrChunk.closeChunk();
							}
						
						/* Finish reading the header chunk: */
						headerChunk.closeChunk();
						
						/* Process all subchunks of the blok chunk: */
						std::string imageName;
						unsigned int projMode=0;
						while(surfChunk.getRestSize()>0)
							{
							/* Read the next subchunk: */
							Chunk blokChunk(surfChunk,true);
							
							/* Process the subchunk: */
							if(blokChunk.isChunk("IMAG"))
								{
								/* Read the clip index: */
								unsigned int clipIndex=blokChunk.readVarInt();
								
								/* Set the texture image name: */
								imageName=clipNames[clipIndex].getDest();
								}
							else if(blokChunk.isChunk("PROJ"))
								{
								/* Read the projection mode: */
								projMode=blokChunk.template read<unsigned short int>();
								}
							
							/* Finish reading the subchunk: */
							blokChunk.closeChunk();
							}
						
						/* Check if the texture map is a color map: */
						if(memcmp(mapChannel,"COLR",4)==0)
							{
							/* Create a texture: */
							tmap=new TextureMap;
							tmap->imageName=imageName;
							switch(projMode)
								{
								case 0U:
									tmap->projectionMode=TextureMap::PLANAR;
									break;
								
								case 1U:
									tmap->projectionMode=TextureMap::CYLINDRICAL;
									break;
								
								case 2U:
									tmap->projectionMode=TextureMap::SPHERICAL;
									break;
								
								case 3U:
									tmap->projectionMode=TextureMap::CUBIC;
									break;
								
								case 5U:
									tmap->projectionMode=TextureMap::UVMAP;
									break;
								
								default:
									tmap->projectionMode=TextureMap::UNKNOWN;
								}
							}
						}
					}
				
				/* Finish reading the subchunk: */
				surfChunk.closeChunk();
				}
			
			/* Set the polygon mesh's surface material parameters: */
			GLMaterial material;
			material.ambient=GLMaterial::Color(surface.color);
			material.diffuse=GLMaterial::Color(surface.color);
			material.specular=GLMaterial::Color(surface.spec,surface.spec,surface.spec);
			material.shininess=surface.glos/8.0f;
			material.emission=GLMaterial::Color(surface.lumi,surface.lumi,surface.lumi);
			MaterialPointer surfaceMaterial;
			if(tmap!=0)
				{
				try
					{
					/* Create a Phong-illuminated surface with textured color channel: */
					PhongTextureMaterial* phongTexMat=new PhongTextureMaterial(material,materialManager.loadTexture(tmap->imageName));
					if(surface.flags&Surface::DOUBLE_SIDED)
						phongTexMat->setTwoSided(true);
					surfaceMaterial=phongTexMat;
					}
				catch(std::runtime_error err)
					{
					/* Print an error message: */
					std::cerr<<"Ignoring texture map "<<tmap->imageName<<" due to exception "<<err.what()<<std::endl;
					
					/* Create an untextured Phong material instead: */
					PhongMaterial* phongMat=new PhongMaterial(material);
					if(surface.flags&Surface::DOUBLE_SIDED)
						phongMat->setTwoSided(true);
					surfaceMaterial=phongMat;
					}
				delete tmap;
				}
			else
				{
				/* Create a Phong-illuminated surface: */
				PhongMaterial* phongMat=new PhongMaterial(material);
				if(surface.flags&Surface::DOUBLE_SIDED)
					phongMat->setTwoSided(true);
				surfaceMaterial=phongMat;
				}
			
			/* Store the material: */
			while(surfaceMaterials.size()<=surface.index)
				surfaceMaterials.push_back(0);
			surfaceMaterials[surface.index]=surfaceMaterial;
			}
		
		/* Finish reading the chunk: */
		chunk.closeChunk();
		}
	
	/*************************
	Finalize the polygon mesh:
	*************************/
	
	/* Find all crease edges: */
	std::cout<<"Generating crease edges..."<<std::flush;
	Misc::Timer smoothingTimer;
	Card maxSurfaceIndex=0;
	for(std::vector<Surface>::const_iterator sIt=surfaces.begin();sIt!=surfaces.end();++sIt)
		if(maxSurfaceIndex<sIt->index)
			maxSurfaceIndex=sIt->index;
	std::vector<Scalar> creaseAngles;
	creaseAngles.reserve(maxSurfaceIndex+1);
	for(Card i=0;i<=maxSurfaceIndex;++i)
		creaseAngles.push_back(Scalar(0));
	for(std::vector<Surface>::const_iterator sIt=surfaces.begin();sIt!=surfaces.end();++sIt)
		creaseAngles[sIt->index]=sIt->creaseAngle;
	mesh.findCreaseEdges(creaseAngles);
	smoothingTimer.elapse();
	std::cout<<" done in "<<smoothingTimer.getTime()*1000.0<<" ms"<<std::endl;
	
	/* Calculate all vertex normal vectors: */
	std::cout<<"Generating normal vectors..."<<std::flush;
	Misc::Timer normalTimer;
	mesh.calcVertexNormals();
	normalTimer.elapse();
	std::cout<<" done in "<<normalTimer.getTime()*1000.0<<" ms"<<std::endl;
	
	/* Triangulate the polygon mesh independently for each surface: */
	for(Card surfaceIndex=0;surfaceIndex<surfaces.size();++surfaceIndex)
		{
		/* Add the surface's material to the triangle set: */
		triangleSet.addMaterial(surfaceMaterials[surfaceIndex]);
		
		/* Triangulate the specific surface: */
		mesh.triangulateSurface(triangleSet,surfaceIndex);
		
		/* Finish the submesh: */
		triangleSet.setSubMeshMaterial(surfaceIndex);
		triangleSet.finishSubMesh();
		}
	}

template <class DataSourceParam>
inline
void
readLWOFile(
	DataSourceParam& dataSource,
	MaterialManager& materialManager,
	MyTriangleSet& triangleSet)
	{
	/* Read the FORM chunk: */
	IFFChunk<DataSourceParam> formChunk(dataSource);
	if(!formChunk.isChunk("FORM"))
		Misc::throwStdErr("is not a valid Lightwave object file");
	
	/* Check the FORM chunk type: */
	char formChunkType[4];
	formChunk.template read<char>(formChunkType,4);
	if(memcmp(formChunkType,"LWOB",4)==0)
		{
		try
			{
			readLWOBFile(formChunk,materialManager,triangleSet);
			}
		catch(std::runtime_error err)
			{
			Misc::throwStdErr("caused exception %s",err.what());
			}
		}
	else if(memcmp(formChunkType,"LWO2",4)==0)
		{
		try
			{
			readLWO2File(formChunk,materialManager,triangleSet);
			}
		catch(std::runtime_error err)
			{
			Misc::throwStdErr("caused exception %s",err.what());
			}
		}
	else
		Misc::throwStdErr("is not of a supported Lightwave object file format");
	
	/* Finish reading the data source: */
	formChunk.closeChunk();
	}

}

PolygonModel* readLWOFile(const char* fileName,MaterialManager& materialManager,Cluster::Multiplexer* multiplexer)
	{
	/* Create the result triangle set: */
	Misc::SelfDestructPointer<MyTriangleSet> result(new MyTriangleSet);
	
	/* Open the input file: */
	IO::FilePtr lwoFile(Cluster::openFile(multiplexer,fileName));
	lwoFile->setEndianness(Misc::BigEndian);
	
	/* Read the file: */
	try
		{
		readLWOFile(*lwoFile,materialManager,*result);
		}
	catch(std::runtime_error err)
		{
		Misc::throwStdErr("readLWOFile: %s %s",fileName,err.what());
		}
	
	/* Return the result triangle set: */
	return result.releaseTarget();
	}
