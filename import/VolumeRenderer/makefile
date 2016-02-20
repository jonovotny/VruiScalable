########################################################################
# Makefile for generic volume rendering application.
# Copyright (c) 2000-2006 Oliver Kreylos
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or (at
# your option) any later version.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
# USA
########################################################################

# Set the Vrui installation directory:
VRUIDIR = /gpfs/data/dhl/gfx/Vrui

# Set up additional flags for the C++ compiler:
CFLAGS = 

# Include the Vrui application makefile fragment:
ifdef DEBUG
  # Build debug version of the applications, using the debug version of Vrui:
  include $(VRUIDIR)/install_linux/share/Vrui-3.1/Vrui.debug.makeinclude
  CFLAGS += -g2 -O0
else
  # Build release version of the applications, using the release version of Vrui:
  include $(VRUIDIR)/install_linux/share/Vrui-3.1/Vrui.makeinclude
  CFLAGS += -g0 -O3
endif

# Pattern rule to compile C++ sources:
o/%.o: %.cpp
	@mkdir -p o/$(*D)
	@echo Compiling $<...
	@g++ -c -o $@ $(VRUI_CFLAGS) $(CFLAGS) -I. -DNDEBUG $<

# List of all final targets:
ALL = VRVolumeRenderer
.PHONY: all
all: $(ALL)

# Rule to clean build results:
clean:
	-rm -rf o
	-rm -f $(ALL)

# List of required source files:
VRVOLUMERENDERER_SOURCES = VolumeRenderer.cpp \
                           PaletteRenderer.cpp \
                           ColorMap.cpp \
                           PaletteEditor.cpp \
                           VRVolumeRenderer.cpp

# Build rule for application:
VRVolumeRenderer: VRUI_CFLAGS += -I.
VRVolumeRenderer: $(VRVOLUMERENDERER_SOURCES:%.cpp=o/%.o)
	@echo Linking $@...
	@g++ -o $@ $^ $(VRUI_LINKFLAGS)
