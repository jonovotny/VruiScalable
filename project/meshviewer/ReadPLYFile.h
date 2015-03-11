/***********************************************************************
ReadPLYFile - Function to read polygonal models from 3D model files in
PLY file format.
Copyright (c) 2011 Oliver Kreylos
***********************************************************************/

#ifndef READPLYFILE_INCLUDED
#define READPLYFILE_INCLUDED

/* Forward declarations: */
namespace Cluster {
class Multiplexer;
}
class PolygonModel;

PolygonModel* readPLYFile(const char* fileName,Cluster::Multiplexer* multiplexer); // Reads a PLY file and returns a polygonal model

#endif
