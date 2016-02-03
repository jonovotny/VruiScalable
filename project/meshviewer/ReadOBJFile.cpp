/***********************************************************************
ReadOBJFile - Function to read polygonal models from 3D model files in
Alias Wavefront OBJ file format.
Copyright (c) 2010-2012 Oliver Kreylos
***********************************************************************/

#include "ReadOBJFile.h"

#include <stdexcept>
#include <vector>
#include <iostream>
#include <Misc/SelfDestructPointer.h>
#include <Misc/StringPrintf.h>
#include <Misc/ThrowStdErr.h>
#include <Misc/StringHashFunctions.h>
#include <Misc/HashTable.h>
#include <IO/ValueSource.h>
#include <Cluster/OpenFile.h>
#include <Geometry/SplineCurve.h>

#include "Material.h"
#include "PhongMaterial.h"
#include "TextureMaterial.h"
#include "PhongTextureMaterial.h"
#include "MaterialManager.h"
#include "MeshVertex.h"
#include "HierarchicalTriangleSet.h"
#include "CurveSet.h"
#include "MultiModel.h"
#include "Tesselator.h"

namespace {

/**************
Helper classes:
**************/

typedef float Scalar;
typedef MeshVertex<Scalar> MyMeshVertex;
typedef HierarchicalTriangleSet<MyMeshVertex> MyTriangleSet;
typedef CurveSet<Scalar> MyCurveSet;
typedef Tesselator<MyMeshVertex> MyTesselator;
typedef Misc::HashTable<std::string,MaterialPointer> MaterialMap;

class OBJValueSource:public IO::ValueSource // Derived ValueSource class to handle line continuations
	{
	/* Embedded classes: */
	private:
	typedef IO::ValueSource Base; // Base class type
	
	/* Elements: */
	private:
	std::string fileName; // Name of source file
	unsigned int lineNumber; // Current line number
	
	/* Private methods: */
	void skipContinuations(void)
		{
		while(!Base::eof()&&Base::peekc()=='\\')
			{
			/* Skip the rest of the line: */
			Base::skipLine();
			++lineNumber;
			Base::skipWs();
			}
		}
	
	/* Constructors and destructors: */
	public:
	OBJValueSource(IO::FilePtr sSource,std::string sFileName)
		:Base(sSource),fileName(sFileName),lineNumber(1)
		{
		/* Set default punctuation characters: */
		Base::setPunctuation("#\\\n");
		skipWs();
		skipComments();
		}
	
	/* Overloaded methods from IO::ValueSource: */
	void skipWs(void)
		{
		Base::skipWs();
		skipContinuations();
		}
	void skipLine(void)
		{
		while(!Base::eof()&&Base::peekc()!='\n')
			{
			/* Check for line continuation characters: */
			if(Base::peekc()=='\\')
				{
				/* Skip the continued line end: */
				Base::skipLine();
				++lineNumber;
				}
			else
				{
				/* Skip the next character: */
				Base::getChar();
				}
			}
		}
	int readChar(void)
		{
		int result=Base::readChar();
		if(result=='\n')
			++lineNumber;
		skipContinuations();
		return result;
		}
	std::string readString(void)
		{
		std::string result=Base::readString();
		skipContinuations();
		return result;
		}
	std::string readLine(void)
		{
		std::string result;
		while(!Base::eof()&&Base::peekc()!='\n')
			{
			/* Check for line continuation characters: */
			if(Base::peekc()=='\\')
				{
				/* Skip the continued line end: */
				Base::skipLine();
				++lineNumber;
				}
			else
				{
				/* Read and store the next character: */
				result.push_back(Base::getChar());
				}
			}
		return result;
		}
	int readInteger(void)
		{
		int result=0;
		try
			{
			result=Base::readInteger();
			skipContinuations();
			}
		catch(Base::NumberError err)
			{
			Misc::throwStdErr("OBJValueSource: Number format error at %s:%s",fileName.c_str(),lineNumber);
			}
		return result;
		}
	unsigned int readUnsignedInteger(void)
		{
		unsigned int result=0;
		try
			{
			result=Base::readUnsignedInteger();
			skipContinuations();
			}
		catch(Base::NumberError err)
			{
			Misc::throwStdErr("OBJValueSource: Number format error at %s:%u",fileName.c_str(),lineNumber);
			}
		return result;
		}
	double readNumber(void)
		{
		double result=0.0;
		try
			{
			result=Base::readNumber();
			skipContinuations();
			}
		catch(Base::NumberError err)
			{
			Misc::throwStdErr("OBJValueSource: Number format error at %s:%u",fileName.c_str(),lineNumber);
			}
		return result;
		}
	
	/* New methods: */
	bool eol(void) const
		{
		return Base::eof()||Base::peekc()=='\n';
		}
	void skipComments(void)
		{
		while(!Base::eof()&&(Base::peekc()=='\n'||Base::peekc()=='#'))
			{
			skipLine();
			readChar();
			}
		}
	void finishLine(void)
		{
		skipLine();
		readChar();
		skipComments();
		}
	std::string where(void) const // Returns a string with the current file:line location
		{
		return Misc::stringPrintf("%s:%u",fileName.c_str(),lineNumber);
		}
	GLMaterial::Color readColor(void)
		{
		GLMaterial::Color result;
		for(int i=0;i<3;++i)
			result[i]=GLfloat(readNumber());
		result[3]=1.0f;
		return result;
		}
	};

/****************
Helper functions:
****************/

std::string trim(std::string s)
	{
	std::string result;
	
	/* Skip whitespace at the beginning of the string: */
	std::string::const_iterator sIt=s.begin();
	while(sIt!=s.end()&&isspace(*sIt))
		++sIt;
	
	/* Copy the rest of the string: */
	while(sIt!=s.end())
		{
		/* Copy a stretch of non-whitespace characters: */
		while(sIt!=s.end()&&!isspace(*sIt))
			{
			result.push_back(*sIt);
			++sIt;
			}
		
		/* Tentatively skip the next stretch of whitespace characters: */
		std::string::const_iterator wsStart=sIt;
		while(sIt!=s.end()&&isspace(*sIt))
			++sIt;
		
		/* Check if there is more non-whitespace: */
		if(sIt!=s.end())
			{
			/* Copy the stretch of whitespace: */
			result.append(wsStart,sIt);
			}
		}
	
	return result;
	}

void readMaterialFile(const char* fileName,std::string baseDirectory,MaterialManager& materialManager,MaterialMap& materialMap,Cluster::Multiplexer* multiplexer)
	{
	/* Open the input file: */
	OBJValueSource mtlFile(Cluster::openFile(multiplexer,fileName),fileName);
	
	/* Read the file: */
	bool inMaterial=false; // Flag whether parser is currently parsing a material definition
	std::string materialName; // Name of currently parsed material
	GLMaterial phong; // Phong material properties of current material
	GLMaterial::Color black(0.0f,0.0f,0.0f,1.0f);
	std::string diffuseTextureName; // Name of diffuse texture image
	while(!mtlFile.eof())
		{
		/* Read the tag: */
		std::string tag=mtlFile.readString();
		if(tag=="newmtl")
			{
			if(inMaterial&&!materialMap.isEntry(materialName))
				{
				/* Add the current material to the material map: */
				if(!diffuseTextureName.empty())
					{
					/* Load the texture map: */
					Texture diffuseTexture=materialManager.loadTexture(diffuseTextureName);
					
					if(phong.ambient!=black||phong.diffuse!=black||phong.specular!=black||phong.emission!=black)
						{
						/* Create a Phong texture material: */
						MaterialPointer mat=new PhongTextureMaterial(phong,diffuseTexture);
						materialMap.setEntry(MaterialMap::Entry(materialName,mat));
						}
					else
						{
						/* Create a texture material: */
						MaterialPointer mat=new TextureMaterial(diffuseTexture);
						materialMap.setEntry(MaterialMap::Entry(materialName,mat));
						}
					}
				else
					{
					/* Create a Phong material: */
					MaterialPointer mat=new PhongMaterial(phong);
					materialMap.setEntry(MaterialMap::Entry(materialName,mat));
					}
				}
			
			/* Start a new material: */
			inMaterial=true;
			materialName=trim(mtlFile.readLine());
			phong.ambient=GLMaterial::Color(0.0f,0.0f,0.0f);
			phong.diffuse=GLMaterial::Color(0.8f,0.8f,0.8f);
			phong.specular=GLMaterial::Color(0.4f,0.4f,0.4f);
			phong.shininess=25.0f;
			phong.emission=GLMaterial::Color(0.0f,0.0f,0.0f);
			diffuseTextureName.clear();
			}
		else if(tag=="Ka")
			phong.ambient=mtlFile.readColor();
		else if(tag=="Kd")
			phong.diffuse=mtlFile.readColor();
		else if(tag=="Ks")
			phong.specular=mtlFile.readColor();
		else if(tag=="Ns")
			{
			phong.shininess=float(mtlFile.readNumber());
			if(phong.shininess>128.0f)
				phong.shininess=128.0f;
			}
		else if(tag=="Ke")
			phong.emission=mtlFile.readColor();
		else if(tag=="Tr")
			mtlFile.readNumber();
		else if(tag=="Tf")
			mtlFile.readColor();
		else if(tag=="Ni")
			mtlFile.readNumber();
		else if(tag=="d")
			{
			double d=mtlFile.readNumber();
			if(d!=1.0)
				std::cout<<"Unsupported dissolve value "<<d<<" at "<<mtlFile.where()<<std::endl;
			}
		else if(tag=="map_Kd")
			{
			/* Read the texture name: */
			diffuseTextureName=baseDirectory;
			diffuseTextureName.append(trim(mtlFile.readLine()));
			}
		else if(tag=="illum")
			{
			unsigned int illum=mtlFile.readUnsignedInteger();
			switch(illum)
				{
				case 0:
					phong.ambient=GLMaterial::Color(0.0f,0.0f,0.0f);
					phong.specular=GLMaterial::Color(0.0f,0.0f,0.0f);
					break;
				
				case 1:
					phong.specular=GLMaterial::Color(0.0f,0.0f,0.0f);
					break;
				
				case 2:
					break;
				
				default:
					std::cout<<"Unsupported illumination value "<<illum<<" at "<<mtlFile.where()<<std::endl;
				}
			}
		else
			{
			/* Skip unknown tag: */
			std::cout<<"Unknown tag "<<tag<<" at "<<mtlFile.where()<<std::endl;
			}
		
		/* Go to the next line: */
		mtlFile.finishLine();
		}
	
	/* Terminate any dangling materials: */
	if(inMaterial&&!materialMap.isEntry(materialName))
		{
		/* Add the current material to the material map: */
		if(!diffuseTextureName.empty())
			{
			/* Load the texture map: */
			Texture diffuseTexture=materialManager.loadTexture(diffuseTextureName);
			
			/* Create a Phong texture material: */
			MaterialPointer mat=new PhongTextureMaterial(phong,diffuseTexture);
			materialMap.setEntry(MaterialMap::Entry(materialName,mat));
			}
		else
			{
			/* Create a Phong material: */
			MaterialPointer mat=new PhongMaterial(phong);
			materialMap.setEntry(MaterialMap::Entry(materialName,mat));
			}
		}
	}

}

PolygonModel* readOBJFiles(const std::vector<const char*>& fileNames,MaterialManager& materialManager,Cluster::Multiplexer* multiplexer)
	{
	/* Create the result models: */
	Misc::SelfDestructPointer<MyTriangleSet> triangles(new MyTriangleSet);
	Misc::SelfDestructPointer<MyCurveSet> curves(new MyCurveSet);
	
	/* Create the material map: */
	MaterialMap materialMap(17);
	
	for(std::vector<const char*>::const_iterator fnIt=fileNames.begin();fnIt!=fileNames.end();++fnIt)
		{
		/* Get the file name's base directory: */
		const char* slashPtr=*fnIt;
		for(const char* fnPtr=*fnIt;*fnPtr!='\0';++fnPtr)
			if(*fnPtr=='/')
				slashPtr=fnPtr+1;
		std::string baseDirectory(*fnIt,slashPtr);
		
		/* Open the input file: */
		OBJValueSource objFile(Cluster::openFile(0,*fnIt),*fnIt);
		
		/* Read the file: */
		MyTesselator tesselator;
		typedef Geometry::HVector<MyMeshVertex::Scalar,3> HPoint;
		std::vector<MyMeshVertex::TPoint> vertexTexCoords;
		std::vector<MyMeshVertex::Vector> vertexNormals;
		std::vector<HPoint> vertexPositions;
		bool inSubMesh=false;
		std::string subMeshName;
		MaterialPointer currentMaterial;
		while(!objFile.eof())
			{
			/* Read the tag: */
			std::string tag=objFile.readString();
			if(tag=="vt")
				{
				/* Read a texture vertex: */
				MyMeshVertex::TPoint texCoord=MyMeshVertex::TPoint::origin;
				int i;
				for(i=0;i<2&&!objFile.eol();++i)
					texCoord[i]=objFile.readNumber();
				if(i<1)
					std::cout<<"Truncated texture vertex at "<<objFile.where()<<std::endl;
				vertexTexCoords.push_back(texCoord);
				}
			else if(tag=="vn")
				{
				/* Read a normal vertex: */
				MyMeshVertex::Vector normal=MyMeshVertex::Vector::zero;
				int i;
				for(i=0;i<3&&!objFile.eol();++i)
					normal[i]=objFile.readNumber();
				if(i<3)
					std::cout<<"Truncated normal vertex at "<<objFile.where()<<std::endl;
				vertexNormals.push_back(normal);
				}
			else if(tag=="vp")
				{
				/* Ignore parameter space vertices */
				}
			else if(tag=="v")
				{
				/* Read a vertex: */
				HPoint position=HPoint::origin;
				int i;
				for(i=0;i<4&&!objFile.eol();++i)
					position[i]=objFile.readNumber();
				if(i<3)
					std::cout<<"Truncated vertex at "<<objFile.where()<<std::endl;
				if(position[3]!=MyMeshVertex::Scalar(1))
					{
					/* Turn affine point + weight into a proper projective point: */
					for(int i=0;i<3;++i)
						position[i]*=position[3];
					}
				vertexPositions.push_back(position);
				}
			else if(tag=="p")
				{
				/* Ignore point primitives: */
				}
			else if(tag=="l")
				{
				/* Ignore line primitives: */
				}
			else if(tag=="f")
				{
				/* Read a face primitive: */
				std::vector<MyMeshVertex> faceVertices;
				while(!objFile.eol())
					{
					MyMeshVertex v;
					v.texCoord=MyMeshVertex::TPoint::origin;
					v.tangentS=v.tangentT=MyMeshVertex::Vector::zero;
					v.normal=MyMeshVertex::Vector::zero;
					int positionIndex=objFile.readInteger();
					if(positionIndex<0)
						v.position=vertexPositions.end()[positionIndex].toPoint();
					else
						v.position=vertexPositions[positionIndex-1].toPoint();
					if(objFile.peekc()=='/')
						{
						objFile.getChar();
						if(objFile.peekc()!='/')
							{
							int textureIndex=objFile.readInteger();
							if(textureIndex<0)
								v.texCoord=vertexTexCoords.end()[textureIndex];
							else
								v.texCoord=vertexTexCoords[textureIndex-1];
							}
						if(objFile.peekc()=='/')
							{
							objFile.getChar();
							if(objFile.peekc()=='-'||(objFile.peekc()>='0'&&objFile.peekc()<='9'))
								{
								int normalIndex=objFile.readInteger();
								if(normalIndex<0)
									v.normal=vertexNormals.end()[normalIndex];
								else
									v.normal=vertexNormals[normalIndex-1];
								}
							else
								objFile.skipWs();
							}
						}
					faceVertices.push_back(v);
					}
				
				/* Tesselate the face: */
				tesselator.setVertices(&faceVertices[0]);
				MyTesselator::Card numVertices=faceVertices.size();
				tesselator.reset(numVertices);
				for(MyTesselator::Card i=0;i<numVertices;++i)
					tesselator.addVertex(i);
				
				#if 0
				
				/* Calculate the face normal: */
				MyTesselator::Vector d0=faceVertices[0].position-faceVertices[numVertices-1].position;
				MyTesselator::Vector normal;
				unsigned int i=1;
				do
					{
					MyTesselator::Vector d1=faceVertices[i].position-faceVertices[0].position;
					normal=Geometry::cross(d0,d1);
					++i;
					}
				while(Geometry::sqr(normal)==0.0f);
				
				#else
				
				/* Use a dummy face normal (tesselator will re-calculate anyways): */
				MyTesselator::Vector normal=MyTesselator::Vector::zero;
				
				#endif
				
				tesselator.tesselate(normal);
				const MyTesselator::Index* tvi=tesselator.getTriangleVertexIndices();
				for(MyTesselator::Card i=0;i<tesselator.getNumTriangles();++i)
					{
					triangles->addVertex(faceVertices[tvi[i*3+0]]);
					triangles->addVertex(faceVertices[tvi[i*3+1]]);
					triangles->addVertex(faceVertices[tvi[i*3+2]]);
					}
				
				inSubMesh=true;
				}
			else if(tag=="cstype")
				{
				/* Read the curve/surface type: */
				std::string csType=objFile.readString();
				bool rational=csType=="rat";
				if(rational)
					csType=objFile.readString();
				objFile.finishLine();
				
				/* Read curve/surface properties: */
				unsigned int degree[2]={0,0};
				int curveDim=-1;
				Scalar pMin,pMax;
				std::vector<int> curve;
				std::vector<Scalar> parms[2];
				while(!objFile.eof())
					{
					/* Read the next tag: */
					std::string tag=objFile.readString();
					if(tag=="end")
						break;
					else if(tag=="deg")
						{
						/* Read the curve's polynomial degree: */
						int i;
						for(i=0;i<2&&!objFile.eol();++i)
							degree[i]=objFile.readUnsignedInteger();
						if(i<1)
							std::cout<<"Truncated polynomial degree at "<<objFile.where()<<std::endl;
						}
					else if(tag=="curv")
						{
						curveDim=1;
						
						/* Read the curve's parameter interval: */
						pMin=objFile.readNumber();
						pMax=objFile.readNumber();
						
						/* Read the curve's vertex indices: */
						curve.clear();
						while(!objFile.eol())
							curve.push_back(objFile.readInteger());
						}
					else if(tag=="parm")
						{
						/* Read the parameter dimension: */
						std::string parm=objFile.readString();
						int parmDim=-1;
						if(parm=="u")
							parmDim=0;
						else if(parm=="v")
							parmDim=1;
						if(parmDim>=0)
							{
							parms[parmDim].clear();
							while(!objFile.eol())
								parms[parmDim].push_back(objFile.readNumber());
							}
						else
							{
							/* Skip unknown parameter type: */
							std::cout<<"Unknown curve parameter "<<parm<<" at "<<objFile.where()<<std::endl;
							}
						}
					else
						std::cout<<"Unknown tag "<<tag<<" at "<<objFile.where()<<std::endl;
					
					objFile.finishLine();
					}

				/* Tesselate the curve/surface: */
				if(csType=="bspline")
					{
					/* Calculate the number of control points in each dimension: */
					unsigned int numCps[2];
					unsigned int totalNumCps=1;
					for(int i=0;i<curveDim;++i)
						{
						numCps[i]=parms[i].size()-degree[i]-1;
						totalNumCps*=numCps[i];
						}
					if(curve.size()!=totalNumCps)
						{
						std::cerr<<"B-spline curve/surface with wrong number of knots at "<<objFile.where()<<std::endl;
						}
					else if(curveDim==1)
						{
						if(rational)
							{
							/* Create a rational b-spline curve: */
							MyCurveSet::RBSC sc(degree[0],numCps[0]);
							
							/* Set the spline's control points: */
							for(unsigned int i=0;i<numCps[0];++i)
								{
								if(curve[i]<0)
									sc.setPoint(i,vertexPositions.end()[curve[i]]);
								else
									sc.setPoint(i,vertexPositions[curve[i]-1]);
								}
							
							/* Set the spline's knots, ignoring the first and last knots: */
							for(unsigned int i=0;i<numCps[0]+degree[0]-1;++i)
								sc.setKnot(i,MyCurveSet::BSC::Parameter(parms[0][i+1]));
							
							/* Add the spline to the curve set: */
							curves->addCurve(sc);
							}
						else
							{
							/* Create a non-rational b-spline curve: */
							MyCurveSet::BSC sc(degree[0],numCps[0]);
							
							/* Set the spline's control points: */
							for(unsigned int i=0;i<numCps[0];++i)
								{
								if(curve[i]<0)
									sc.setPoint(i,vertexPositions.end()[curve[i]].toPoint());
								else
									sc.setPoint(i,vertexPositions[curve[i]-1].toPoint());
								}
							
							/* Set the spline's knots, ignoring the first and last knots: */
							for(unsigned int i=0;i<numCps[0]+degree[0]-1;++i)
								sc.setKnot(i,MyCurveSet::BSC::Parameter(parms[0][i+1]));
							
							/* Add the spline to the curve set: */
							curves->addCurve(sc);
							}
						
						inSubMesh=true;
						}
					else if(curveDim==2)
						{
						/* Create a b-spline patch: */
						std::cout<<"B-spline patch with "<<numCps[0]<<" x "<<numCps[1]<<" control points"<<std::endl;
						}
					}
				else if(csType=="bezier")
					{
					std::cout<<"Bezier curve/surface"<<std::endl;
					}
				else
					std::cout<<"Unknown curve/surface type "<<csType<<" at "<<objFile.where()<<std::endl;
				}
			else if(tag=="g"||tag=="o")
				{
				/* Finish the current submesh if there is one: */
				if(inSubMesh)
					{
					triangles->finishSubMesh();
					curves->finishSubMesh();
					inSubMesh=false;
					}
				
				/* Check for a group name: */
				if(!objFile.eol())
					{
					/* Set the submesh name: */
					subMeshName=trim(objFile.readLine());
					triangles->setSubMeshName(subMeshName);
					}
				
				/* Set the submesh material: */
				triangles->setSubMeshMaterial(currentMaterial);
				}
			else if(tag=="s")
				{
				/* Ignore smoothing group indices: */
				}
			else if(tag=="mtllib")
				{
				/* Read the material library file: */
				std::string materialFileName=baseDirectory;
				materialFileName.append(trim(objFile.readLine()));
				try
					{
					readMaterialFile(materialFileName.c_str(),baseDirectory,materialManager,materialMap,multiplexer);
					}
				catch(std::runtime_error err)
					{
					std::cerr<<"Ignoring material library "<<materialFileName<<" due to exception "<<err.what()<<std::endl;
					}
				}
			else if(tag=="usemtl")
				{
				/* Retrieve the material: */
				std::string materialName=trim(objFile.readLine());
				MaterialMap::Iterator mmIt=materialMap.findEntry(materialName);
				if(!mmIt.isFinished())
					{
					if(currentMaterial!=mmIt->getDest())
						{
						/* Finish the current submesh if there is one: */
						if(inSubMesh)
							{
							triangles->finishSubMesh();
							curves->finishSubMesh();
							inSubMesh=false;
							}
						
						/* Set the next submesh name: */
						triangles->setSubMeshName(subMeshName);
						
						/* Change the material: */
						currentMaterial=mmIt->getDest();
						triangles->setSubMeshMaterial(currentMaterial);
						}
					}
				}
			else
				std::cout<<"Unknown tag "<<tag<<" at "<<objFile.where()<<std::endl;
			
			objFile.finishLine();
			}
		
		/* Finish any dangling submeshes: */
		if(inSubMesh)
			{
			triangles->finishSubMesh();
			curves->finishSubMesh();
			}
		}
	
	if(triangles->getNumVertices()>0)
		{
		/* Finalize the triangle mesh: */
		triangles->sortSubMeshes();
		triangles->createKdTree();
		}
	
	/* Compose the result model: */
	if(triangles->getNumVertices()>0&&curves->getNumCurves()>0)
		{
		/* Join the triangle and line sets: */
		MultiModel* result=new MultiModel;
		result->addPart(triangles.releaseTarget());
		result->addPart(curves.releaseTarget());
		return result;
		}
	else if(triangles->getNumVertices()>0)
		{
		return triangles.releaseTarget();
		}
	else if(curves->getNumCurves()>0)
		{
		return curves.releaseTarget();
		}
	else
		return 0;
	}
