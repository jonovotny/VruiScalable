/***********************************************************************
ReadLWOFile - Function to read polygonal models from 3D model files in
Lightwave object file format.
Copyright (c) 2008-2011 Oliver Kreylos
***********************************************************************/

#ifndef READLWOFILE_INCLUDED
#define READLWOFILE_INCLUDED

/* Forward declarations: */
namespace Cluster {
class Multiplexer;
}
class PolygonModel;
class MaterialManager;

PolygonModel* readLWOFile(const char* fileName,MaterialManager& materialManager,Cluster::Multiplexer* multiplexer =0); // Reads a Lightwave object file and returns a polygonal model

#endif
