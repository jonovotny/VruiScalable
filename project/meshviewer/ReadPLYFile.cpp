/***********************************************************************
ReadPLYFile - Function to read polygonal models from 3D model files in
PLY file format.
Copyright (c) 2011 Oliver Kreylos
***********************************************************************/

#include "ReadPLYFile.h"

#include <Misc/SelfDestructPointer.h>
#include <Misc/ThrowStdErr.h>
#include <IO/File.h>
#include <IO/ValueSource.h>
#include <Cluster/OpenFile.h>

#include "PhongMaterial.h"
#include "MeshVertex.h"
#include "TriangleSet.h"
#include "PolygonMesh.h"
#include "PlyFileStructures.h"

namespace {

/**************
Helper classes:
**************/

typedef MeshVertex<float> MyMeshVertex;
typedef TriangleSet<MyMeshVertex> MyTriangleSet;
typedef PolygonMesh<MyMeshVertex> MyPolygonMesh;

/****************
Helper functions:
****************/

template <class PLYFileParam>
void readPlyFileElements(const PLYFileHeader& header,PLYFileParam& ply,MyTriangleSet& triangleSet)
	{
	/* Create a temporary polygon mesh to calculate normal vectors: */
	MyPolygonMesh mesh;
	
	/* Process all PLY file elements in order: */
	for(size_t elementIndex=0;elementIndex<header.getNumElements();++elementIndex)
		{
		/* Get the next element: */
		const PLYElement& element=header.getElement(elementIndex);
		
		/* Check if it's the vertex or face element: */
		if(element.isElement("vertex"))
			{
			/* Get the indices of all relevant vertex value components: */
			unsigned int posIndex[3];
			posIndex[0]=element.getPropertyIndex("x");
			posIndex[1]=element.getPropertyIndex("y");
			posIndex[2]=element.getPropertyIndex("z");
			
			/* Read the vertex element: */
			PLYElement::Value vertexValue(element);
			for(size_t i=0;i<element.getNumValues();++i)
				{
				/* Read vertex element from file: */
				vertexValue.read(ply);
				
				/* Extract vertex coordinates from vertex element: */
				MyMeshVertex vertex;
				for(int j=0;j<3;++j)
					vertex.position[j]=MyMeshVertex::Scalar(vertexValue.getValue(posIndex[j]).getScalar()->getDouble());
				
				/* Add the vertex to the mesh: */
				mesh.addVertex(vertex);
				}
			}
		else if(element.isElement("face"))
			{
			if(mesh.getNumVertices()==0)
				Misc::throwStdErr("readPlyFile: Face element before vertex element");
			
			/* Read all face vertex indices in the mesh file: */
			PLYElement::Value faceValue(element);
			unsigned int vertexIndicesIndex=element.getPropertyIndex("vertex_indices");
			for(size_t i=0;i<element.getNumValues();++i)
				{
				/* Read face element from file: */
				faceValue.read(ply);
				
				/* Extract vertex indices from face element: */
				unsigned int numFaceVertices=faceValue.getValue(vertexIndicesIndex).getListSize()->getUnsignedInt();
				mesh.startFace();
				for(unsigned int j=0;j<numFaceVertices;++j)
					mesh.addFaceVertex(faceValue.getValue(vertexIndicesIndex).getListElement(j)->getUnsignedInt());
				mesh.finishFace();
				}
			}
		else
			{
			/* Skip the entire element: */
			skipElement(element,ply);
			}
		}
	
	/* Calculate normal vectors for the temporary mesh: */
	mesh.calcVertexNormals();
	
	/* Triangulate the temporary mesh and create the result triangle set: */
	mesh.triangulate(triangleSet);
	}

}

PolygonModel* readPLYFile(const char* fileName,Cluster::Multiplexer* multiplexer)
	{
	/* Create the result model: */
	Misc::SelfDestructPointer<MyTriangleSet> result(new MyTriangleSet);
	
	/* Create a default material to render the model: */
	GLMaterial m(GLMaterial::Color(0.5f,0.5f,0.5f),GLMaterial::Color(1.0f,1.0f,1.0f),25.0f);
	result->setSubMeshMaterial(result->addMaterial(new PhongMaterial(m)));
	
	/* Open the input file: */
	IO::FilePtr plyFile(Cluster::openFile(multiplexer,fileName));
	
	/* Read the PLY file's header: */
	PLYFileHeader header(*plyFile);
	if(!header.isValid())
		Misc::throwStdErr("readPLYFile: Input file %s is not a valid PLY file",fileName);
	
	/* Read the PLY file in ASCII or binary mode: */
	if(header.getFileType()==PLYFileHeader::Ascii)
		{
		/* Attach a value source to the PLY file: */
		IO::ValueSource ply(plyFile);
		
		/* Read the PLY file in ASCII mode: */
		readPlyFileElements(header,ply,*result);
		}
	else
		{
		/* Set the PLY file's endianness: */
		plyFile->setEndianness(header.getFileEndianness());
		
		/* Read the PLY file in binary mode: */
		readPlyFileElements(header,*plyFile,*result);
		}
	
	/* Finalize and return the result mesh: */
	result->finishSubMesh();
	return result.releaseTarget();
	}
