/***********************************************************************
ReadASEFile - Function to read polygonal models from 3D model files in
3D Studio Max ASCII export file format.
Copyright (c) 2009 Oliver Kreylos
***********************************************************************/

#ifndef READASEFILE_INCLUDED
#define READASEFILE_INCLUDED

/* Forward declarations: */
namespace Cluster {
class Multiplexer;
}
class PolygonModel;

PolygonModel* readASEFile(const char* fileName,Cluster::Multiplexer* multiplexer =0); // Reads a 3D Studio Max ASCII export file and returns a polygonal model

#endif
