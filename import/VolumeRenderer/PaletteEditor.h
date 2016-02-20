/***********************************************************************
PaletteEditor - Class to represent a GLMotif popup window to edit
one-dimensional transfer functions with RGB color and opacity.
Copyright (c) 2005-2006 Oliver Kreylos

This file is part of the 3D Data Visualization Library (3DVis).

The 3D Data Visualization Library is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The 3D Data Visualization Library is distributed in the hope that it
will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the 3D Data Visualization Library; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#ifndef PALETTEEDITOR_INCLUDED
#define PALETTEEDITOR_INCLUDED

#include <GLMotif/PopupWindow.h>

#include "ColorMap.h"

/* Forward declarations: */
namespace Misc {
class CallbackData;
class CallbackList;
}
class GLColorMap;
namespace GLMotif {
class Blind;
class Slider;
}

class PaletteEditor:public GLMotif::PopupWindow
	{
	/* Embedded classes: */
	public:
	typedef GLMotif::ColorMap::ColorMapCreationType ColorMapCreationType;
	
	/* Elements: */
	private:
	GLMotif::ColorMap* colorMap; // The color map widget
	GLMotif::Blind* colorPanel; // The control point color display widget
	GLMotif::Slider* colorSliders[3]; // The color selection slider widgets
	
	/* Private methods: */
	void selectedControlPointChangedCallback(Misc::CallbackData* cbData);
	void colorMapChangedCallback(Misc::CallbackData* cbData);
	void colorSliderValueChangedCallback(Misc::CallbackData* cbData);
	void removeControlPointCallback(Misc::CallbackData* cbData);
	
	/* Constructors and destructors: */
	public:
	PaletteEditor(void);
	
	/* Methods: */
	GLMotif::ColorMap* getColorMap(void) // Returns pointer to the underlying color map widget
		{
		return colorMap;
		};
	void createPalette(ColorMapCreationType colorMapType,double valueRangeMin,double valueRangeMax); // Creates a standard palette
	void loadPalette(const char* paletteFileName); // Loads a palette from a palette file
	void savePalette(const char* paletteFileName) const; // Saves the current palette to a palette file
	Misc::CallbackList& getColorMapChangedCallbacks(void) // Returns list of color map change callbacks
		{
		return colorMap->getColorMapChangedCallbacks();
		};
	void exportColorMap(GLColorMap& glColorMap) const; // Exports color map to GLColorMap object
	};

#endif
