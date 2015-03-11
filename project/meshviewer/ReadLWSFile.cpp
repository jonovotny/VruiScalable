/***********************************************************************
ReadLWSFile - Function to read hierarchical polygon models in Lightwave
scene file format.
Copyright (c) 2010-2011 Oliver Kreylos
***********************************************************************/

#include "ReadLWSFile.h"

#include <stdlib.h>
#include <string>
#include <stdexcept>
#include <Misc/SelfDestructPointer.h>
#include <Misc/ThrowStdErr.h>
#include <IO/ValueSource.h>
#include <Cluster/OpenFile.h>

#include "MeshVertex.h"
#include "TriangleSet.h"
#include "ReadLWOFile.h"

namespace {

/**************
Helper classes:
**************/

typedef MeshVertex<float> MyMeshVertex;
typedef TriangleSet<MyMeshVertex> MyTriangleSet;

struct Object // Structure describing an object to be loaded
	{
	/* Elements: */
	public:
	std::string lwoFileName; // Name of LWO file containing object
	MyTriangleSet::Transform transform; // Transformation of object relative to its parent
	int parentIndex; // Index of parent object; -1 for root object
	};

/****************
Helper functions:
****************/

void loadObject(const std::vector<Object>& objects,int index,const MyTriangleSet::Transform& parentTransform,MyTriangleSet& triangleSet,MaterialManager& materialManager,Cluster::Multiplexer* multiplexer)
	{
	/* Calculate this object's transformation: */
	MyTriangleSet::Transform transform=parentTransform*objects[index].transform;
	
	if(objects[index].lwoFileName!="")
		{
		try
			{
			/* Load this object's LWO file: */
			PolygonModel* pm=readLWOFile(objects[index].lwoFileName.c_str(),materialManager,multiplexer);
			MyTriangleSet* ts=dynamic_cast<MyTriangleSet*>(pm);
			if(ts!=0)
				{
				/* Join the LWO file with the result triangle set: */
				triangleSet.addTriangleSet(*ts,transform);
				}
			delete pm;
			}
		catch(std::runtime_error err)
			{
			std::cout<<"readLWSFile: Ignoring LWO file "<<objects[index].lwoFileName<<" due to exception "<<err.what()<<std::endl;
			}
		}
	
	/* Process all children of this object: */
	for(int i=0;i<int(objects.size());++i)
		if(objects[i].parentIndex==index)
			loadObject(objects,i,transform,triangleSet,materialManager,multiplexer);
	}

void readLWSFile(IO::ValueSource& lwsFile,std::string sceneBaseDirectory,MyTriangleSet& triangleSet,MaterialManager& materialManager,Cluster::Multiplexer* multiplexer)
	{
	if(lwsFile.readString()!="LWSC"||lwsFile.readChar()!='\n')
		Misc::throwStdErr("is not a valid Lightwave scene file");
	
	/* Check the file format version: */
	if(lwsFile.readInteger()!=1||lwsFile.readChar()!='\n')
		Misc::throwStdErr("is not of a supported Lightwave scene file format");
	
	/* Read all objects declared in the file: */
	std::vector<Object> objects;
	int rootIndex=-1; // Index of root object
	Object newObject;
	bool haveObject=false;
	while(!lwsFile.eof())
		{
		/* Skip empty lines: */
		while(lwsFile.peekc()=='\n')
			lwsFile.readChar();
		
		/* Read the next tag: */
		std::string tag=lwsFile.readString();
		
		/* Read the next value: */
		std::string value=lwsFile.readLine();
		lwsFile.skipWs();
		
		/* Remove trailing spaces from the value: */
		std::string::iterator vIt=value.end();
		for(;isspace(vIt[-1]);--vIt)
			;
		value.erase(vIt,value.end());
		
		/* Parse the tag: */
		if(tag=="AddLight")
			{
			/* Save the previous object: */
			if(haveObject)
				{
				if(newObject.parentIndex==-1)
					rootIndex=objects.size();
				objects.push_back(newObject);
				}
			
			haveObject=false;
			}
		else if(tag=="LoadObject"||tag=="AddNullObject")
			{
			/* Save the previous object: */
			if(haveObject)
				{
				if(newObject.parentIndex==-1)
					rootIndex=objects.size();
				objects.push_back(newObject);
				}
			
			/* Start a new object: */
			if(tag=="LoadObject")
				{
				/* Find the LWO file's base name: */
				std::string::iterator baseNameIt=value.begin();
				for(std::string::iterator vIt=value.begin();vIt!=value.end();++vIt)
					if(*vIt=='\\'||*vIt=='/')
						baseNameIt=vIt+1;
				
				/* Replace the LWO file name's directory prefix with the real directory string: */
				newObject.lwoFileName=sceneBaseDirectory;
				newObject.lwoFileName.append(baseNameIt,value.end());
				}
			else
				newObject.lwoFileName="";
			newObject.transform=MyTriangleSet::Transform::identity;
			newObject.parentIndex=-1;
			haveObject=true;
			}
		else if(tag=="ObjectMotion")
			{
			/* Read an object motion block: */
			lwsFile.setWhitespace('\n',true);
			unsigned int numChannels=lwsFile.readUnsignedInteger();
			unsigned int numFrames=lwsFile.readUnsignedInteger();
			double channels[9];
			
			/* Read all frames: */
			for(unsigned int frame=0;frame<numFrames;++frame)
				{
				/* Read the channels: */
				for(unsigned int i=0;i<numChannels;++i)
					channels[i]=lwsFile.readNumber();
				
				/* Skip the timing data: */
				for(unsigned int i=0;i<5;++i)
					lwsFile.readNumber();
				}
			
			if(haveObject)
				{
				/* Create a transformation from the last channel set: */
				newObject.transform=MyTriangleSet::Transform::translate(MyTriangleSet::Vector(channels[0],channels[2],channels[1]));
				newObject.transform*=MyTriangleSet::Transform::rotate(MyTriangleSet::Transform::Rotation::rotateX(Math::rad(-channels[4])));
				newObject.transform*=MyTriangleSet::Transform::rotate(MyTriangleSet::Transform::Rotation::rotateZ(Math::rad(-channels[3])));
				newObject.transform*=MyTriangleSet::Transform::rotate(MyTriangleSet::Transform::Rotation::rotateY(Math::rad(-channels[5])));
				newObject.transform*=MyTriangleSet::Transform::scale(MyTriangleSet::Transform::Scale(channels[6],channels[8],channels[7]));
				}
			
			lwsFile.setPunctuation('\n',true);
			}
		else if(tag=="ParentObject")
			{
			if(haveObject)
				{
				/* Store the parent object: */
				newObject.parentIndex=atoi(value.c_str())-1;
				}
			}
		}
	
	/* Save the last object: */
	if(haveObject)
		{
		if(newObject.parentIndex==-1)
			rootIndex=objects.size();
		objects.push_back(newObject);
		}
	
	/* Add all objects to the polygon model creator, starting at the root: */
	if(rootIndex==-1)
		Misc::throwStdErr("does not contain a root object");
	loadObject(objects,rootIndex,MyTriangleSet::Transform::identity,triangleSet,materialManager,multiplexer);
	}

}

PolygonModel* readLWSFile(const char* fileName,MaterialManager& materialManager,Cluster::Multiplexer* multiplexer)
	{
	/* Create the result triangle set: */
	Misc::SelfDestructPointer<MyTriangleSet> result(new MyTriangleSet);
	
	/* Find the directory prefix of the file name: */
	const char* slashPtr=0;
	for(const char* fnPtr=fileName;*fnPtr!='\0';++fnPtr)
		if(*fnPtr=='/')
			slashPtr=fnPtr;
	std::string directory=slashPtr!=0?std::string(fileName,slashPtr+1):"";
	
	/* Open the input file: */
	IO::ValueSource lwsFile(Cluster::openFile(multiplexer,fileName));
	lwsFile.setPunctuation('\n',true);
	lwsFile.skipWs();
	
	/* Read the file: */
	try
		{
		readLWSFile(lwsFile,directory,*result,materialManager,multiplexer);
		}
	catch(std::runtime_error err)
		{
		Misc::throwStdErr("readLWSFile: %s %s",fileName,err.what());
		}
	
	/* Return the result triangle set: */
	return result.releaseTarget();
	}
