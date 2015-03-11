########################################################################
# Makefile for MeshViewer, an application to render static polygonal
# meshes loaded from a variety of mesh file formats.
# Copyright (c) 2011-2013 Oliver Kreylos
#
# This file is part of the WhyTools Build Environment.
# 
# The WhyTools Build Environment is free software; you can redistribute
# it and/or modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2 of the
# License, or (at your option) any later version.
# 
# The WhyTools Build Environment is distributed in the hope that it will
# be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
# of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with the WhyTools Build Environment; if not, write to the Free
# Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
# 02111-1307 USA
########################################################################

# Directory containing the Vrui build system. The directory below
# matches the default Vrui installation; if Vrui's installation
# directory was changed during Vrui's installation, the directory below
# must be adapted.
VRUI_MAKEDIR := $(G)/install_linux/share/Vrui-3.1/make

# Base installation directory for MeshViewer. If this is set to the
# default of $(PWD), MeshViewer does not have to be installed to be run.
# MeshViewer's executables will be installed in the bin directory
# underneath the given base directory.
INSTALLDIR = $(G)/install_linux

########################################################################
# Everything below here should not have to be changed
########################################################################

# Version number for installation subdirectories. This is used to keep
# subsequent release versions of MeshViewer from clobbering each other.
# The value should be identical to the major.minor version number found
# in VERSION in the root package directory.
VERSION = 0.3

# Include definitions for the system environment and system-provided
# packages
include $(VRUI_MAKEDIR)/SystemDefinitions
include $(VRUI_MAKEDIR)/Packages.System
include $(VRUI_MAKEDIR)/Configuration.Vrui
include $(VRUI_MAKEDIR)/Packages.Vrui

# Set installation directory structure:
EXECUTABLEINSTALLDIR = $(INSTALLDIR)/$(EXEDIR)

########################################################################
# List common packages used by all components of this project
# (Supported packages can be found in $(VRUI_MAKEDIR)/Packages.*)
########################################################################

PACKAGES = MYVRUI

########################################################################
# Specify all final targets
########################################################################

ALL = $(EXEDIR)/MeshViewer

PHONY: all
all: $(ALL)

########################################################################
# Specify other actions to be performed on a `make clean'
########################################################################

.PHONY: extraclean
extraclean:

.PHONY: extrasqueakyclean
extrasqueakyclean:

# Include basic makefile
include $(VRUI_MAKEDIR)/BasicMakefile

########################################################################
# Specify build rules for executables
########################################################################

MESHVIEWER_SOURCES = Texture.cpp \
                     MaterialManager.cpp \
                     PhongMaterial.cpp \
                     TextureMaterial.cpp \
                     PhongTextureMaterial.cpp \
                     PolygonMesh.cpp \
                     LineSet.cpp \
                     CurveSet.cpp \
                     TriangleSet.cpp \
                     TriangleKdTree.cpp \
                     RenderBSPTree.cpp \
                     HierarchicalTriangleSet.cpp \
                     MultiModel.cpp \
                     PlyFileStructures.cpp \
                     ReadPLYFile.cpp \
                     ReadLWOFile.cpp \
                     ReadLWSFile.cpp \
                     ReadASEFile.cpp \
                     ReadOBJFile.cpp \
                     PolygonMeshTest.cpp

$(EXEDIR)/MeshViewer: $(MESHVIEWER_SOURCES:%.cpp=$(OBJDIR)/%.o)
.PHONY: MeshViewer
MeshViewer: $(EXEDIR)/MeshViewer

install: $(ALL)
	@echo Installing MeshViewer in $(INSTALLDIR)...
	@install -d $(INSTALLDIR)
	@install -d $(EXECUTABLEINSTALLDIR)
	@install $(ALL) $(EXECUTABLEINSTALLDIR)
