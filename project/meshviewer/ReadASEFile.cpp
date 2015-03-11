/***********************************************************************
ReadASEFile - Function to read polygonal models from 3D model files in
3D Studio Max ASCII export file format.
Copyright (c) 2009 Oliver Kreylos
***********************************************************************/

#include "ReadASEFile.h"

#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <Misc/SelfDestructPointer.h>
#include <Misc/ThrowStdErr.h>
#include <Misc/Timer.h>
#include <Misc/StandardHashFunction.h>
#include <Misc/HashTable.h>
#include <IO/TokenSource.h>
#include <Cluster/OpenFile.h>
#include <Geometry/ComponentArray.h>
#include <Geometry/Vector.h>
#include <Geometry/Matrix.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Geometry/AffineTransformation.h>
#include <GL/gl.h>
#include <GL/GLColor.h>
#include <GL/GLColorOperations.h>
#include <GL/GLMaterial.h>

#include "PhongMaterial.h"
#include "MeshVertex.h"
#include "HierarchicalTriangleSet.h"
#include "LineSet.h"
#include "PolygonMesh.h"
#include "MultiModel.h"

#define INDICATE_FLIPPED_NODES 1
#define FLIP_FLIPPED_NODES 0

namespace {

/**************
Helper classes:
**************/

typedef MeshVertex<float> MyMeshVertex;
typedef HierarchicalTriangleSet<MyMeshVertex> MyTriangleSet;
typedef LineSet<MyMeshVertex> MyLineSet;
typedef PolygonMesh<MyMeshVertex> MyPolygonMesh;
typedef MyPolygonMesh::Scalar Scalar;
typedef MyPolygonMesh::Point Point;
typedef MyPolygonMesh::Vector Vector;
typedef MyPolygonMesh::Card Card;
typedef GLColor<GLfloat,3> Color; // Type for RGB colors

struct TextureMap // Structure describing a texture map
	{
	/* Elements: */
	public:
	std::string imageName; // Name of the texture image
	};

struct Material // Structure for ASE file materials
	{
	/* Elements: */
	public:
	std::string name;
	Color ambient;
	Color diffuse;
	Color specular;
	float shininess,shineStrength;
	float transparency;
	float lineWidth;
	TextureMap diffuseMap;
	
	/* Constructors and destructors: */
	Material(void)
		:ambient(1.0f,1.0f,1.0f),
		 diffuse(1.0f,1.0f,1.0f),
		 specular(0.0f,0.0f,0.0f),
		 shininess(0.0f),shineStrength(0.0f),
		 transparency(0.0f),
		 lineWidth(1.0f)
		{
		}
	};

class ASEParser // Helper class to parse ASE files
	{
	/* Embedded classes: */
	private:
	typedef Misc::HashTable<std::string,MyTriangleSet::Card> SubMeshHasher;
	typedef Geometry::AffineTransformation<Scalar,3> ATransform;
	
	/* Elements: */
	private:
	std::string sourceName;
	IO::TokenSource tok;
	size_t currentLine;
	std::vector<MaterialPointer> materials; // List of materials referenced by the ASE file
	MyTriangleSet* triangleSet; // A triangle set object to store triangulated shapes
	SubMeshHasher subMeshHasher; // A hash table mapping ASE node names to submesh indices
	MyLineSet* lineSet; // A line set object to store non-triangulated shapes
	std::string currentNodeName; // Name of the currently parsed node
	ATransform currentNodeTransform; // Transformation to apply to vertices from the current node
	bool currentNodeFlipped; // Flag if the current node's transformation is flipped, i.e., has a negative determinant
	bool explicitNormals; // Flag if the ASE file contains explicit vertex normal vectors
	
	/* Private methods: */
	void skipStuff(void)
		{
		while(true)
			{
			if(tok.peekc()=='\n')
				{
				++currentLine;
				tok.readNextToken();
				}
			else if(tok.peekc()=='/')
				{
				tok.readNextToken();
				if(tok.isToken("//"))
					{
					/* Skip the rest of the line: */
					tok.skipLine();
					++currentLine;
					
					/* Skip whitespace at the beginning of the next line: */
					tok.skipWs();
					}
				}
			else
				break;
			}
		}
	bool eof(void)
		{
		skipStuff();
		return tok.eof();
		}
	int peekc(void)
		{
		skipStuff();
		return tok.peekc();
		}
	const char* readNextToken(void)
		{
		skipStuff();
		return tok.readNextToken();
		}
	void throwParseError(const char* errorString)
		{
		Misc::throwStdErr("readASEFile: %s %s in line %u",sourceName.c_str(),errorString,(unsigned int)currentLine);
		}
	
	/* Constructors and destructors: */
	public:
	ASEParser(const char* sSourceName,IO::FilePtr sCharacterSource)
		:sourceName(sSourceName),
		 tok(sCharacterSource),
		 currentLine(1),
		 triangleSet(new MyTriangleSet),
		 subMeshHasher(17),
		 lineSet(new MyLineSet)
		{
		/* Initialize the token source: */
		tok.setPunctuation("{},\n");
		tok.setQuotes("\"");
		tok.skipWs();
		}
	~ASEParser(void)
		{
		delete triangleSet;
		delete lineSet;
		}
	
	/* Methods: */
	MyTriangleSet* retrieveTriangleSet(void)
		{
		/* Calculate the triangle set's collision kd-tree: */
		std::cout<<"Creating collision kd-tree..."<<std::flush;
		Misc::Timer t1;
		triangleSet->createKdTree();
		t1.elapse();
		std::cout<<" done in "<<t1.getTime()*1000.0<<" ms"<<std::endl;
		
		/* Return the triangle set: */
		MyTriangleSet* result=triangleSet;
		triangleSet=0;
		return result;
		}
	MyLineSet* retrieveLineSet(void)
		{
		MyLineSet* result=lineSet;
		lineSet=0;
		return result;
		}

	void parseUnrecognized(void)
		{
		if(tok.isCaseToken("*COMMENT"))
			{
			/* Skip the comment: */
			readNextToken();
			}
		else
			{
			// DEBUGGING
			std::cout<<"Skipping tag "<<tok.getToken()<<std::endl;
			
			/* Read all tag values and check for braces: */
			int braceLevel=0;
			while(!eof()&&(braceLevel>0||(peekc()!='}'&&peekc()!='*')))
				{
				const char* token=readNextToken();
				
				if(token[0]=='{')
					++braceLevel;
				else if(token[0]=='}')
					--braceLevel;
				}
			
			if(braceLevel>0)
				throwParseError("has an unterminated unrecognized group");
			}
		}
	void parseMap(TextureMap& map)
		{
		/* Check for opening brace: */
		if(readNextToken()[0]!='{')
			throwParseError("has a missing opening brace in a MAP group");
		
		/* Parse tag/value pairs until closing brace: */
		while(!eof())
			{
			/* Read the next tag: */
			const char* tag=readNextToken();
			
			if(tag[0]=='}')
				return;
			else if(tag[0]!='*')
				throwParseError("is missing a tag inside a MAP group");
			else if(tok.isCaseToken("*BITMAP"))
				{
				/* Read the bitmap name: */
				map.imageName=readNextToken();
				}
			else
				parseUnrecognized();
			}
		
		throwParseError("has an unterminated MAP group");
		}
	void parseMaterial(int materialIndex)
		{
		/* Initialize the material parser: */
		Material material;
		
		/* Check for opening brace: */
		if(readNextToken()[0]!='{')
			throwParseError("has a missing opening brace in a MATERIAL group");
		
		/* Parse tag/value pairs until closing brace: */
		bool closed=false;
		while(!eof())
			{
			/* Read the next tag: */
			const char* tag=readNextToken();
			
			if(tag[0]=='}')
				{
				closed=true;
				break;
				}
			else if(tag[0]!='*')
				throwParseError("is missing a tag inside a MATERIAL group");
			else if(tok.isCaseToken("*MATERIAL_NAME"))
				{
				/* Read the material name: */
				std::string materialName=readNextToken();
				}
			else if(tok.isCaseToken("*MATERIAL_CLASS"))
				{
				/* Read the material class: */
				std::string materialClass=readNextToken();
				}
			else if(tok.isCaseToken("*MATERIAL_AMBIENT"))
				{
				/* Read the ambient color: */
				for(int i=0;i<3;++i)
					material.ambient[i]=float(atof(readNextToken()));
				}
			else if(tok.isCaseToken("*MATERIAL_DIFFUSE"))
				{
				/* Read the diffuse color: */
				for(int i=0;i<3;++i)
					material.diffuse[i]=float(atof(readNextToken()));
				}
			else if(tok.isCaseToken("*MATERIAL_SPECULAR"))
				{
				/* Read the specular color: */
				for(int i=0;i<3;++i)
					material.specular[i]=float(atof(readNextToken()));
				}
			else if(tok.isCaseToken("*MATERIAL_SHINE"))
				material.shininess=float(atof(readNextToken()));
			else if(tok.isCaseToken("*MATERIAL_SHINESTRENGTH"))
				material.shineStrength=float(atof(readNextToken()));
			else if(tok.isCaseToken("*MATERIAL_TRANSPARENCY"))
				material.transparency=float(atof(readNextToken()));
			else if(tok.isCaseToken("*MATERIAL_WIRESIZE"))
				material.lineWidth=float(atof(readNextToken()));
			else if(tok.isCaseToken("*NUMSUBMTLS"))
				{
				int numSubMaterials=atoi(readNextToken());
				}
			else if(tok.isCaseToken("*MAP_DIFFUSE"))
				{
				/* Start the diffuse map parser: */
				parseMap(material.diffuseMap);
				}
			else if(tok.isCaseToken("*SUBMATERIAL"))
				{
				/* Skip the submaterial index: */
				readNextToken();
				
				/* Start the submaterial parser: */
				parseMaterial(-1);
				}
			else
				parseUnrecognized();
			}
		
		if(!closed)
			throwParseError("has an unterminated MATERIAL group");
		
		if(materialIndex>=0)
			{
			/* Create a Phong material for the just-parsed material: */
			GLMaterial m;
			m.ambient=GLMaterial::Color(material.ambient);
			m.diffuse=GLMaterial::Color(material.diffuse);
			if(material.shininess>0.0f)
				m.specular=GLMaterial::Color(material.specular)*GLMaterial::Color::Scalar(material.shineStrength);
			else
				m.specular=GLMaterial::Color(0.0f,0.0f,0.0f);
			m.shininess=float(material.shininess)*128.0f;
			
			/* Store the material: */
			materials[materialIndex]=new PhongMaterial(m);
			}
		}
	void parseMaterialList(void)
		{
		/* Initialize the material list parser: */
		materials.clear();
		
		/* Check for opening brace: */
		if(readNextToken()[0]!='{')
			throwParseError("has a missing opening brace in a MATERIAL_LIST group");
		
		/* Parse tag/value pairs until closing brace: */
		while(!eof())
			{
			/* Read the next tag: */
			const char* tag=readNextToken();
			
			if(tag[0]=='}')
				{
				#if INDICATE_FLIPPED_NODES
				/* Add another fake material to render flipped geometry: */
				GLMaterial m;
				m.ambient=GLMaterial::Color(1.0f,0.0f,0.0f);
				m.diffuse=GLMaterial::Color(1.0f,0.0f,0.0f);
				m.specular=GLMaterial::Color(1.0f,1.0f,1.0f);
				m.shininess=25.0f;
				
				/* Store the material: */
				materials.push_back(new PhongMaterial(m));
				#endif
				
				return;
				}
			else if(tag[0]!='*')
				throwParseError("is missing a tag inside a MATERIAL_LIST group");
			else if(tok.isCaseToken("*MATERIAL_COUNT"))
				{
				/* Read the number of materials: */
				int numMaterials=atoi(readNextToken());
				
				/* Initialize the material name list: */
				for(int i=0;i<numMaterials;++i)
					materials.push_back(0);
				}
			else if(tok.isCaseToken("*MATERIAL"))
				{
				/* Read the the material index: */
				int materialIndex=atoi(readNextToken());
				if(materialIndex>int(materials.size()))
					throwParseError("has an out-of-bounds material index");
				else if(materialIndex==int(materials.size()))
					materials.push_back(0);
				
				/* Start the material parser: */
				parseMaterial(materialIndex);
				}
			else
				parseUnrecognized();
			}
		
		throwParseError("has an unterminated MATERIAL_LIST group");
		}
	void parseNodeTm(void)
		{
		typedef Geometry::ComponentArray<Scalar,3> Size;
		typedef Geometry::Vector<Scalar,3> Vector;
		typedef Geometry::Matrix<Scalar,3,4> Matrix;
		typedef Geometry::OrthogonalTransformation<Scalar,3> OGTransform;
		typedef Geometry::AffineTransformation<Scalar,3> ATransform;
		
		/* Check for opening brace: */
		if(readNextToken()[0]!='{')
			throwParseError("has a missing opening brace in a NODE_TM group");
		
		/* Assemble the node transformation as a matrix and a component-wise transformation: */
		Matrix matrix=Matrix::zero;
		Vector translation=Vector::zero;
		Vector rotationAxis=Vector::zero;
		Scalar rotationAngle=Scalar(0);
		Size scale(1,1,1);
		Vector scaleAxis=Vector::zero;
		Scalar scaleAngle=Scalar(0);
		
		/* Parse tag/value pairs until closing brace: */
		bool closed=false;
		while(!eof())
			{
			/* Read the next tag: */
			const char* tag=readNextToken();
			
			if(tag[0]=='}')
				{
				closed=true;
				break;
				}
			else if(tag[0]!='*')
				throwParseError("is missing a tag inside a NODE_TM group");
			else if(tok.isCaseToken("*NODE_NAME"))
				{
				/* Read the node name and compare it with the current node's name: */
				std::string nodeName=readNextToken();
				if(nodeName!=currentNodeName)
					std::cout<<"Node name: "<<nodeName<<" in *NODE_TM does not match "<<currentNodeName<<std::endl;
				}
			else if(tok.isCaseToken("*INHERIT_POS"))
				{
				/* Read the inherited translation: */
				Vector inheritedTranslation;
				for(int i=0;i<3;++i)
					inheritedTranslation[i]=Scalar(atof(readNextToken()));
				}
			else if(tok.isCaseToken("*INHERIT_ROT"))
				{
				/* Read the inherited rotation: */
				Vector inheritedRotation;
				for(int i=0;i<3;++i)
					inheritedRotation[i]=Scalar(atof(readNextToken()));
				}
			else if(tok.isCaseToken("*INHERIT_SCL"))
				{
				/* Read the inherited scaling: */
				Vector inheritedScaling;
				for(int i=0;i<3;++i)
					inheritedScaling[i]=Scalar(atof(readNextToken()));
				}
			else if(tok.isCaseToken("*TM_POS"))
				{
				/* Read the translation: */
				for(int i=0;i<3;++i)
					translation[i]=Scalar(atof(readNextToken()));
				}
			else if(tok.isCaseToken("*TM_ROTAXIS"))
				{
				/* Read the rotation axis: */
				for(int i=0;i<3;++i)
					rotationAxis[i]=Scalar(atof(readNextToken()));
				}
			else if(tok.isCaseToken("*TM_ROTANGLE"))
				{
				/* Read the rotation angle: */
				rotationAngle=Scalar(atof(readNextToken()));
				}
			else if(tok.isCaseToken("*TM_SCALE"))
				{
				/* Read the scaling factors: */
				for(int i=0;i<3;++i)
					scale[i]=Scalar(atof(readNextToken()));
				}
			else if(tok.isCaseToken("*TM_SCALEAXIS"))
				{
				/* Read the scaling axis: */
				for(int i=0;i<3;++i)
					scaleAxis[i]=Scalar(atof(readNextToken()));
				}
			else if(tok.isCaseToken("*TM_SCALEAXISANG"))
				{
				/* Read the scaling angle: */
				scaleAngle=Scalar(atof(readNextToken()));
				}
			else if(strncasecmp(tok.getToken(),"*TM_ROW",7)==0&&tok.getToken()[7]>='0'&&tok.getToken()[7]<='3')
				{
				/* Get the matrix column index: */
				int column=int(tok.getToken()[7]-'0');
				
				/* Read the matrix column: */
				for(int i=0;i<3;++i)
					matrix(i,column)=Scalar(atof(readNextToken()));
				}
			else
				parseUnrecognized();
			}
		
		if(!closed)
			throwParseError("has an unterminated NODE_TM group");
		
		/* Save the transformation matrix: */
		currentNodeTransform=ATransform(matrix);
		
		/* Check the matrix' handedness: */
		Scalar det=matrix(0,0)*(matrix(1,1)*matrix(2,2)-matrix(2,1)*matrix(1,2))
		          +matrix(0,1)*(matrix(1,2)*matrix(2,0)-matrix(2,2)*matrix(1,0))
		          +matrix(0,2)*(matrix(1,0)*matrix(2,1)-matrix(2,0)*matrix(1,1));
		currentNodeFlipped=det<Scalar(0);
		
		#if 0
		/* Assemble a second matrix from the component-wise transformation and compare the two: */
		ATransform transform=ATransform::translate(translation);
		if(rotationAngle!=Scalar(0)&&rotationAxis!=Vector::zero)
			transform*=ATransform::rotate(ATransform::Rotation::rotateAxis(rotationAxis,-rotationAngle));
		if(scale[0]!=Scalar(1)||scale[1]!=Scalar(1)||scale[2]!=Scalar(1))
			{
			if(scaleAngle!=Scalar(0)&&scaleAxis!=Vector::zero)
				{
				/* Apply the scaling in its own coordinate system: */
				ATransform::Rotation scaleRot=ATransform::Rotation::rotateAxis(scaleAxis,-scaleAngle);
				transform*=ATransform::rotate(scaleRot);
				transform*=ATransform::scale(scale);
				transform*=ATransform::rotate(Geometry::invert(scaleRot));
				}
			else
				{
				/* Apply the scaling: */
				transform*=ATransform::scale(scale);
				}
			}
		
		Scalar rms2=Scalar(0);
		for(int i=0;i<3;++i)
			for(int j=0;j<4;++j)
				rms2+=Math::sqr(transform.getMatrix()(i,j)-matrix(i,j));
		Scalar rms=Math::sqrt(rms2/Scalar(12));
		if(rms>=Scalar(1.0e-5))
			{
			std::cout<<"Mismatching transformation matrices in node "<<currentNodeName<<std::endl;
			std::cout.setf(std::ios::fixed);
			std::cout.precision(4);
			for(int i=0;i<3;++i)
				{
				for(int j=0;j<3;++j)
					std::cout<<std::setw(7)<<transform.getMatrix()(i,j)<<" ";
				std::cout<<std::setw(10)<<transform.getMatrix()(i,3)<<" ";
				std::cout<<"    ";
				for(int j=0;j<3;++j)
					std::cout<<std::setw(7)<<matrix(i,j)<<" ";
				std::cout<<std::setw(10)<<matrix(i,3)<<" ";
				std::cout<<std::endl;
				}
			std::cout<<"Matrix determinant: "<<det<<std::endl;
			}
		#endif
		}
	void parseMeshVertexList(MyPolygonMesh& mesh)
		{
		/* Check for opening brace: */
		if(readNextToken()[0]!='{')
			throwParseError("has a missing opening brace in a MESH_VERTEX_LIST group");
		
		/* Parse tag/value pairs until closing brace: */
		while(!eof())
			{
			/* Read the next tag: */
			const char* tag=readNextToken();
			
			if(tag[0]=='}')
				return;
			else if(tag[0]!='*')
				throwParseError("is missing a tag inside a MESH_VERTEX_LIST group");
			else if(tok.isCaseToken("*MESH_VERTEX"))
				{
				/* Read the vertex index: */
				Card vertexIndex=Card(atoi(readNextToken()));
				
				/* Read the vertex position: */
				Point p;
				for(int i=0;i<3;++i)
					p[i]=Scalar(atof(readNextToken()));
				
				/* Store the vertex: */
				if(mesh.addVertex(p)!=vertexIndex)
					throwParseError("has out-of-order vertices in a MESH_VERTEX_LIST group");
				}
			else
				parseUnrecognized();
			}
		
		throwParseError("has an unterminated MESH_VERTEX_LIST group");
		}
	void parseMeshFaceList(MyPolygonMesh& mesh)
		{
		/* Initialize the mesh face list parser: */
		Card currentFaceIndex=MyPolygonMesh::invalidIndex;
		
		/* Check for opening brace: */
		if(readNextToken()[0]!='{')
			throwParseError("has a missing opening brace in a MESH_FACE_LIST group");
		
		/* Parse tag/value pairs until closing brace: */
		while(!eof())
			{
			/* Read the next tag: */
			const char* tag=readNextToken();
			
			if(tag[0]=='}')
				return;
			else if(tag[0]!='*')
				throwParseError("is missing a tag inside a MESH_FACE_LIST group");
			else if(tok.isCaseToken("*MESH_FACE"))
				{
				/* Read the face index: */
				Card faceIndex=Card(atoi(readNextToken()));
				
				/* Read the face vertex indices: */
				Card vertexIndices[3];
				static const char* vertexIndexLabels[3]={"A:","B:","C:"};
				for(int i=0;i<3;++i)
					{
					/* Skip the vertex index label: */
					if(strcasecmp(readNextToken(),vertexIndexLabels[i])!=0)
						throwParseError("has an invalid vertex index label in a MESH_FACE_LIST group");
					
					/* Read the vertex index: */
					vertexIndices[i]=Card(atoi(readNextToken()));
					}
				
				/* Skip the face edge flags: */
				static const char* edgeFlagLabels[3]={"AB:","BC:","CA:"};
				for(int i=0;i<3;++i)
					{
					/* Skip the edge flag label: */
					if(strcasecmp(readNextToken(),edgeFlagLabels[i])!=0)
						throwParseError("has an invalid edge flag label in a MESH_FACE_LIST group");
					
					/* Read the edge flag: */
					int edgeFlag=atoi(readNextToken());
					}
				
				#if FLIP_FLIPPED_NODES
				/* Flip the face's vertex order if the current node is mirrored: */
				if(currentNodeFlipped)
					{
					Card t=vertexIndices[0];
					vertexIndices[0]=vertexIndices[2];
					vertexIndices[2]=t;
					}
				#endif
				
				/* Store the face: */
				if((currentFaceIndex=mesh.addFace(3,vertexIndices))!=faceIndex)
					throwParseError("has out-of-order face indices in a MESH_FACE_LIST group");
				}
			else if(tok.isCaseToken("*MESH_SMOOTHING"))
				{
				if(currentFaceIndex==MyPolygonMesh::invalidIndex)
					throwParseError("has a misplaced MESH_SMOOTHING tag in a MESH_FACE_LIST group");
				
				/* Read the list of smoothing groups: */
				unsigned int smoothingGroupMask=0x0;
				while(!eof()&&peekc()!='}'&&peekc()!='*')
					{
					const char* token=readNextToken();
					if(token[0]!=',')
						{
						int smoothingGroupIndex=atoi(token)-1;
						if(smoothingGroupIndex>=0&&smoothingGroupIndex<32)
							smoothingGroupMask|=0x1U<<smoothingGroupIndex;
						else
							throwParseError("has an out-of-bounds smoothing group index in a MESH_FACE_LIST group");
						}
					}
				
				/* Set the current face's smoothing group mask: */
				mesh.setFaceSmoothingGroupMask(currentFaceIndex,smoothingGroupMask);
				}
			else if(tok.isCaseToken("*MESH_MTLID"))
				{
				if(currentFaceIndex==MyPolygonMesh::invalidIndex)
					throwParseError("has a misplaced MESH_MTLID tag in a MESH_FACE_LIST group");
				
				/* Read the submaterial index: */
				int subMaterialIndex=atoi(readNextToken());
				}
			else
				parseUnrecognized();
			}
		
		throwParseError("has an unterminated MESH_FACE_LIST group");
		}
	void parseMeshTvertList(void)
		{
		/* Check for opening brace: */
		if(readNextToken()[0]!='{')
			throwParseError("has a missing opening brace in a MESH_TVERT_LIST group");
		
		/* Parse tag/value pairs until closing brace: */
		while(!eof())
			{
			/* Read the next tag: */
			const char* tag=readNextToken();
			
			if(tag[0]=='}')
				return;
			else if(tag[0]!='*')
				throwParseError("is missing a tag inside a MESH_TVERT_LIST group");
			else if(tok.isCaseToken("*MESH_TVERT"))
				{
				/* Read the vertex index: */
				int vertexIndex=atoi(readNextToken());
				
				/* Read the vertex texture coordinates: */
				Point p;
				for(int i=0;i<3;++i)
					p[i]=Scalar(atof(readNextToken()));
				}
			else
				parseUnrecognized();
			}
		
		throwParseError("has an unterminated MESH_TVERT_LIST group");
		}
	void parseMeshTfaceList(void)
		{
		/* Check for opening brace: */
		if(readNextToken()[0]!='{')
			throwParseError("has a missing opening brace in a MESH_TFACE_LIST group");
		
		/* Parse tag/value pairs until closing brace: */
		while(!eof())
			{
			/* Read the next tag: */
			const char* tag=readNextToken();
			
			if(tag[0]=='}')
				return;
			else if(tag[0]!='*')
				throwParseError("is missing a tag inside a MESH_TFACE_LIST group");
			else if(tok.isCaseToken("*MESH_TFACE"))
				{
				/* Read the face index: */
				int faceIndex=atoi(readNextToken());
				
				/* Read the face texture vertex indices: */
				Card textureVertexIndices[3];
				for(int i=0;i<3;++i)
					textureVertexIndices[currentNodeFlipped?2-i:i]=Card(atoi(readNextToken()));
				}
			else
				parseUnrecognized();
			}
		
		throwParseError("has an unterminated MESH_TFACE_LIST group");
		}
	void parseMeshNormals(MyPolygonMesh& mesh)
		{
		/* Check for opening brace: */
		if(readNextToken()[0]!='{')
			throwParseError("has a missing opening brace in a MESH_NORMALS group");
		
		/* We now have explicit normal vectors: */
		// explicitNormals=true;
		
		/* Parse tag/value pairs until closing brace: */
		Card currentFaceIndex=MyPolygonMesh::invalidIndex;
		Card numFaceVertexNormals=3;
		while(!eof())
			{
			/* Read the next tag: */
			const char* tag=readNextToken();
			
			if(tag[0]=='}')
				return;
			else if(tag[0]!='*')
				throwParseError("is missing a tag inside a MESH_NORMALS group");
			else if(strcasecmp(tag,"*MESH_FACENORMAL")==0)
				{
				/* Read the face index: */
				currentFaceIndex=atoi(readNextToken());
				
				/* Read the face normal vector: */
				Vector normal;
				for(int i=0;i<3;++i)
					normal[i]=Scalar(atof(readNextToken()));
				normal.normalize();
				
				/* Set the face normal: */
				// mesh.setFaceNormal(currentFaceIndex,normal);
				
				/* Prepare to read per-vertex vertex normals: */
				numFaceVertexNormals=0;
				}
			else if(strcasecmp(tag,"*MESH_VERTEXNORMAL")==0)
				{
				if(numFaceVertexNormals<3)
					{
					/* Read the vertex index: */
					Card vertexIndex=atoi(readNextToken());
					
					/* Read the vertex normal vector: */
					Vector normal;
					for(int i=0;i<3;++i)
						normal[i]=Scalar(atof(readNextToken()));
					normal.normalize();
					
					/* Set the per-face vertex normal: */
					// mesh.setFaceVertexNormal(currentFaceIndex,vertexIndex,normal);
					
					++numFaceVertexNormals;
					}
				else
					throwParseError("has a spurious per-face vertex normal");
				}
			else
				parseUnrecognized();
			}
		
		throwParseError("has an unterminated MESH_NORMALS group");
		}
	void parseMesh(MyPolygonMesh& mesh)
		{
		/* Initialize the mesh parser: */
		explicitNormals=false;
		
		/* Check for opening brace: */
		if(readNextToken()[0]!='{')
			throwParseError("has a missing opening brace in a MESH group");
		
		/* Parse tag/value pairs until closing brace: */
		while(!eof())
			{
			/* Read the next tag: */
			const char* tag=readNextToken();
			
			if(tag[0]=='}')
				return;
			else if(tag[0]!='*')
				throwParseError("is missing a tag inside a MESH group");
			else if(tok.isCaseToken("*TIMEVALUE"))
				{
				/* Read the mesh time value: */
				double timeValue=atof(readNextToken());
				}
			else if(tok.isCaseToken("*MESH_NUMVERTEX"))
				{
				/* Read the number of vertices in the mesh: */
				int numVertices=atoi(readNextToken());
				}
			else if(tok.isCaseToken("*MESH_VERTEX_LIST"))
				{
				/* Start the mesh vertex list parser: */
				parseMeshVertexList(mesh);
				}
			else if(tok.isCaseToken("*MESH_NUMFACES"))
				{
				/* Read the number of faces in the mesh: */
				int numFaces=atoi(readNextToken());
				}
			else if(tok.isCaseToken("*MESH_FACE_LIST"))
				{
				/* Start the mesh face list parser: */
				parseMeshFaceList(mesh);
				}
			else if(tok.isCaseToken("*MESH_NUMTVERTEX"))
				{
				/* Read the number of texture vertices in the mesh: */
				int numTverts=atoi(readNextToken());
				}
			else if(tok.isCaseToken("*MESH_TVERTLIST"))
				{
				/* Start the mesh texture vertex list parser: */
				parseMeshTvertList();
				}
			else if(tok.isCaseToken("*MESH_NUMTVFACES"))
				{
				/* Read the number of texture faces in the mesh: */
				int numTfaces=atoi(readNextToken());
				}
			else if(tok.isCaseToken("*MESH_TFACELIST"))
				{
				/* Start the mesh texture face list parser: */
				parseMeshTfaceList();
				}
			else if(tok.isCaseToken("*MESH_NUMCVERTEX"))
				{
				/* Read the number of color vertices in the mesh: */
				int numColorVertices=atoi(readNextToken());
				}
			else if(tok.isCaseToken("*MESH_NORMALS"))
				{
				/* Start the mesh normals parser: */
				parseMeshNormals(mesh);
				}
			else
				parseUnrecognized();
			}
		
		throwParseError("has an unterminated MESH group");
		}
	void parseLine(void)
		{
		/* Initialize the line parser: */
		Card nextVertexIndex=0;
		Point firstVertex,lastVertex;
		bool lineClosed=false;
		
		/* Read the line index: */
		int lineIndex=atoi(readNextToken());
		
		/* Check for opening brace: */
		if(readNextToken()[0]!='{')
			throwParseError("has a missing opening brace in a SHAPE_LINE group");
		
		/* Parse tag/value pairs until closing brace: */
		bool closed=false;
		while(!eof())
			{
			/* Read the next tag: */
			const char* tag=readNextToken();
			
			if(tag[0]=='}')
				{
				closed=true;
				break;
				}
			else if(tag[0]!='*')
				throwParseError("is missing a tag inside a SHAPE_LINE group");
			else if(tok.isCaseToken("*SHAPE_CLOSED"))
				{
				/* Set the closed flag: */
				lineClosed=true;
				}
			else if(tok.isCaseToken("*SHAPE_VERTEXCOUNT"))
				{
				/* Read the number of vertices in the line: */
				int numVertices=atoi(readNextToken());
				}
			else if(tok.isCaseToken("*SHAPE_VERTEX_KNOT")||tok.isCaseToken("*SHAPE_VERTEX_INTERP"))
				{
				/* Read the vertex index: */
				Card vertexIndex=Card(atoi(readNextToken()));
				if(vertexIndex!=nextVertexIndex)
					throwParseError("has out-of-order vertices inside a SHAPE_LINE group");
				
				/* Read the vertex position: */
				Point p;
				for(int i=0;i<3;++i)
					p[i]=Scalar(atof(readNextToken()));
				
				/* Store the line segment: */
				if(vertexIndex>0)
					{
					lineSet->addVertex(lastVertex);
					lineSet->addVertex(p);
					}
				else
					firstVertex=p;
				
				++nextVertexIndex;
				lastVertex=p;
				}
			else
				parseUnrecognized();
			}
		
		if(!closed)
			throwParseError("has an unterminated SHAPE_LINE group");
		
		if(lineClosed)
			{
			/* Close the line: */
			lineSet->addVertex(lastVertex);
			lineSet->addVertex(firstVertex);
			}
		
		/* Finalize the sub line set: */
		lineSet->setSubMeshColor(MyLineSet::Color(0.0f,1.0f,0.0f));
		lineSet->finishSubMesh();
		}
	void parseGeomobject(void)
		{
		/* Initialize the geomobject parser: */
		currentNodeName="";
		std::string currentParentName="";
		currentNodeTransform=ATransform::identity;
		currentNodeFlipped=false;
		int materialIndex=-1;
		MyPolygonMesh mesh;
		
		/* Check for opening brace: */
		if(readNextToken()[0]!='{')
			throwParseError("has a missing opening brace in a GEOMOBJECT group");
		
		/* Parse tag/value pairs until closing brace: */
		bool closed=false;
		while(!eof())
			{
			/* Read the next tag: */
			const char* tag=readNextToken();
			
			if(tag[0]=='}')
				{
				closed=true;
				break;
				}
			else if(tag[0]!='*')
				throwParseError("is missing a tag inside a GEOMOBJECT group");
			else if(tok.isCaseToken("*NODE_NAME"))
				{
				/* Read the node name: */
				currentNodeName=readNextToken();
				}
			else if(tok.isCaseToken("*NODE_PARENT"))
				{
				/* Read the node's parent name: */
				currentParentName=readNextToken();
				}
			else if(tok.isCaseToken("*NODE_TM"))
				{
				/* Start the node transformation parser: */
				parseNodeTm();
				}
			else if(tok.isCaseToken("*MESH"))
				{
				/* Start the mesh parser: */
				parseMesh(mesh);
				}
			else if(tok.isCaseToken("*SHAPE_LINECOUNT"))
				{
				/* Read the number of lines to follow: */
				int numLines=atoi(readNextToken());
				}
			else if(tok.isCaseToken("*SHAPE_LINE"))
				{
				/* Start the line parser: */
				parseLine();
				}
			else if(tok.isCaseToken("*MATERIAL_REF"))
				{
				/* Read the material index: */
				materialIndex=atoi(readNextToken());
				}
			else if(tok.isCaseToken("*WIREFRAME_COLOR"))
				{
				/* Read the wireframe rendering color: */
				Color wireframeColor;
				for(int i=0;i<3;++i)
					wireframeColor[i]=float(atof(readNextToken()));
				}
			else if(tok.isCaseToken("*PROP_MOTIONBLUR"))
				{
				/* Read the motion blur flag: */
				int motionBlur=atoi(readNextToken());
				}
			else if(tok.isCaseToken("*PROP_CASTSHADOW"))
				{
				/* Read the shadow casting flag: */
				int castShadow=atoi(readNextToken());
				}
			else if(tok.isCaseToken("*PROP_RECVSHADOW"))
				{
				/* Read the shadow receiving flag: */
				int receiveShadow=atoi(readNextToken());
				}
			else
				parseUnrecognized();
			}
		
		if(!closed)
			throwParseError("has an unterminated GEOMOBJECT group");
		
		if(mesh.getNumFaces()!=0)
			{
			if(!explicitNormals)
				{
				/* Find all crease edges based on the faces' smoothing groups: */
				mesh.findSmoothingGroupCreaseEdges();
				
				/* Calculate all vertex normal vectors: */
				mesh.calcVertexNormals();
				}
			
			/* Find the current submesh's parent node: */
			SubMeshHasher::Iterator parentSmIt=subMeshHasher.findEntry(currentParentName);
			if(!parentSmIt.isFinished())
				triangleSet->setSubMeshParentIndex(parentSmIt->getDest());
			
			/* Set the current submesh's name: */
			triangleSet->setSubMeshName(currentNodeName);
			
			/* Set the current submesh's material: */
			#if INDICATE_FLIPPED_NODES
			if(currentNodeFlipped)
				materialIndex=materials.size()-1;
			#endif
			if(materialIndex>=0)
				triangleSet->setSubMeshMaterial(materials[materialIndex]);
			
			/* Triangulate the polygon mesh: */
			mesh.triangulate(*triangleSet);
			
			/* Finish the current submesh: */
			triangleSet->finishSubMesh();
			}
		}
	void parseFile(void)
		{
		std::cout<<"Parsing ASE file..."<<std::flush;
		Misc::Timer parseTimer;
		
		/* Parse tag/value pairs until the file is over: */
		while(!eof())
			{
			/* Read the next tag: */
			const char* tag=readNextToken();
			
			if(tag[0]=='}')
				throwParseError("has an extra closing brace");
			else if(tag[0]!='*')
				throwParseError("is missing a tag");
			else if(tok.isCaseToken("*3DSMAX_ASCIIEXPORT"))
				{
				/* Read the file version number: */
				int version=atoi(readNextToken());
				
				// DEBUGGING
				//std::cout<<"ASE file version "<<version<<std::endl;
				}
			else if(tok.isCaseToken("*MATERIAL_LIST"))
				{
				/* Start the material list parser: */
				parseMaterialList();
				}
			else if(tok.isCaseToken("*GEOMOBJECT")||tok.isCaseToken("*SHAPEOBJECT"))
				{
				/* Start the geometry object parser: */
				parseGeomobject();
				}
			else
				parseUnrecognized();
			}
		
		parseTimer.elapse();
		std::cout<<" done in "<<parseTimer.getTime()*1000.0<<" ms"<<std::endl;
		}
	};

}

PolygonModel* readASEFile(const char* fileName,Cluster::Multiplexer* multiplexer)
	{
	/* Read the input file: */
	ASEParser aseParser(fileName,Cluster::openFile(multiplexer,fileName));
	aseParser.parseFile();
	
	/* Create the result model: */
	Misc::SelfDestructPointer<MultiModel> result(new MultiModel);
	
	/* Store the triangle set: */
	result->addPart(aseParser.retrieveTriangleSet());
	
	/* Store the line set: */
	// result->addPart(aseParser.retrieveLineSet());
	
	return result.releaseTarget();
	}
