/***********************************************************************
ReadLWSFile - Function to read hierarchical polygon models in Lightwave
scene file format.
Copyright (c) 2010-2011 Oliver Kreylos
***********************************************************************/

#ifndef READLWSFILE_INCLUDED
#define READLWSFILE_INCLUDED

/* Forward declarations: */
namespace Cluster {
class Multiplexer;
}
class PolygonModel;
class MaterialManager;

PolygonModel* readLWSFile(const char* fileName,MaterialManager& materialManager,Cluster::Multiplexer* multiplexer =0); // Reads a Lightwave scene file and returns a polygonal model

#endif
