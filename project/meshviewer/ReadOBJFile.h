/***********************************************************************
ReadOBJFile - Function to read polygonal models from 3D model files in
Alias Wavefront OBJ file format.
Copyright (c) 2010-2012 Oliver Kreylos
***********************************************************************/

#ifndef READOBJFILE_INCLUDED
#define READOBJFILE_INCLUDED

#include <vector>

/* Forward declarations: */
namespace Cluster {
class Multiplexer;
}
class PolygonModel;
class MaterialManager;

PolygonModel* readOBJFiles(const std::vector<const char*>& fileNames,MaterialManager& materialManager,Cluster::Multiplexer* multiplexer =0); // Reads a set of Alias|Wavefront OBJ files and returns a joined polygonal model

#endif
