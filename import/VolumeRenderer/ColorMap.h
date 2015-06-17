/***********************************************************************
ColorMap - A widget to display color maps (one-dimensional transfer
functions with RGB color and opacity).
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

#ifndef COLORMAP_INCLUDED
#define COLORMAP_INCLUDED

#include <Misc/CallbackData.h>
#include <Misc/CallbackList.h>
#include <GL/gl.h>
#include <GL/GLColor.h>
#include <GLMotif/Types.h>
#include <GLMotif/Widget.h>

/* Forward declarations: */
class GLColorMap;

namespace GLMotif {

class ColorMap:public Widget
	{
	/* Embedded classes: */
	public:
	typedef GLColor<GLfloat,4> ColorMapValue; // Type for colors with opacity values
	
	enum ColorMapCreationType // Enumerated type for color map types
		{
		GREYSCALE,RAINBOW
		};
	
	struct ControlPoint // Structure for color map control points
		{
		/* Elements: */
		public:
		double value; // Control point mapping value
		ColorMapValue color; // Control point color
		GLfloat x,y; // Position of the control point in widget coordinates
		ControlPoint* left; // Pointer to left control point
		ControlPoint* right; // Pointer to right control point
		
		/* Constructors and destructors: */
		ControlPoint(double sValue,const ColorMapValue& sColor)
			:value(sValue),color(sColor),
			 left(0),right(0)
			{
			};
		};
	
	class CallbackData:public Misc::CallbackData // Base class for callback data sent by color maps
		{
		/* Elements: */
		public:
		ColorMap* colorMap; // Pointer to the color map that sent the callback
		
		/* Constructors and destructors: */
		CallbackData(ColorMap* sColorMap)
			:colorMap(sColorMap)
			{
			};
		};
	
	class SelectedControlPointChangedCallbackData:public CallbackData
		{
		/* Elements: */
		public:
		ControlPoint* oldSelectedControlPoint; // Pointer to previously selected control point (or 0 if no control point was selected)
		ControlPoint* newSelectedControlPoint; // Pointer to new selected control point (or 0 if no control point is selected)
		
		/* Constructors and destructors: */
		SelectedControlPointChangedCallbackData(ColorMap* sColorMap,ControlPoint* sOldSelectedControlPoint,ControlPoint* sNewSelectedControlPoint)
			:CallbackData(sColorMap),
			 oldSelectedControlPoint(sOldSelectedControlPoint),
			 newSelectedControlPoint(sNewSelectedControlPoint)
			{
			};
		};
	
	class ColorMapChangedCallbackData:public CallbackData
		{
		/* Constructors and destructors: */
		public:
		ColorMapChangedCallbackData(ColorMap* sColorMap)
			:CallbackData(sColorMap)
			{
			};
		};
	
	/* Elements: */
	private:
	GLfloat marginWidth; // Width of margin around color map area
	Vector preferredSize; // The color map area's preferred size
	Box colorMapAreaBox; // Position and size of color map area
	GLfloat controlPointSize; // Size of control points
	Color selectedControlPointColor; // Color to use for the selected control point
	std::pair<double,double> valueRange; // Range of color map values
	ControlPoint first; // First control point
	ControlPoint last; // Last control point
	Misc::CallbackList selectedControlPointChangedCallbacks; // List of callbacks to be called when the selected control point changes
	Misc::CallbackList colorMapChangedCallbacks; // List of callbacks to be called when the color map changes
	ControlPoint* selected; // Pointer to currently selected control point
	bool isDragging; // Flag whether a control point is being dragged
	Point::Vector dragOffset; // Offset between pointer and dragged control point in widget coordinates
	
	/* Private methods: */
	void updateControlPoints(void); // Updates the widget positions of all control points
	
	/* Constructors and destructors: */
	public:
	ColorMap(const char* sName,Container* sParent,bool sManageChild =true);
	virtual ~ColorMap(void);
	
	/* Methods inherited from Widget: */
	virtual Vector calcNaturalSize(void) const;
	virtual void resize(const Box& newExterior);
	virtual void draw(GLContextData& contextData) const;
	virtual bool findRecipient(Event& event);
	virtual void pointerButtonDown(Event& event);
	virtual void pointerButtonUp(Event& event);
	virtual void pointerMotion(Event& event);
	
	/* New methods: */
	void setMarginWidth(GLfloat newMarginWidth); // Changes the margin width
	void setPreferredSize(const Vector& newPreferredSize); // Sets a new preferred size
	void setControlPointSize(GLfloat newControlPointSize); // Sets a new size for control points
	void setSelectedControlPointColor(const Color& newSelectedControlPointColor); // Sets the color for the selected control points
	Misc::CallbackList& getSelectedControlPointChangedCallbacks(void) // Returns list of selected control point change callbacks
		{
		return selectedControlPointChangedCallbacks;
		};
	Misc::CallbackList& getColorMapChangedCallbacks(void) // Returns list of color map change callbacks
		{
		return colorMapChangedCallbacks;
		};
	const std::pair<double,double>& getValueRange(void) const // Returns the color map's value range
		{
		return valueRange;
		};
	int getNumControlPoints(void) const; // Returns the number of control points
	void selectControlPoint(int controlPointIndex); // Selects the control point of the given index
	void insertControlPoint(double controlPointValue); // Inserts a new control point by interpolating the current color map
	void insertControlPoint(double controlPointValue,const ColorMapValue& controlPointColorValue); // Inserts a new control point with the given color
	void deleteSelectedControlPoint(void); // Deletes the selected intermediate control point (first and last cannot be deleted)
	const ColorMapValue& getSelectedControlPointColorValue(void) // Returns the color map value of the selected control point
		{
		return selected->color;
		};
	void setSelectedControlPointValue(double newControlPointValue); // Changes the value of the selected control point (only between its neighbours)
	void setSelectedControlPointColorValue(const ColorMapValue& newControlPointColorValue); // Changes the color of the selected control point
	void exportColorMap(GLColorMap& glColorMap) const; // Exports color map to GLColorMap object (does not change number of colors or mapping range)
	void createColorMap(ColorMapCreationType colorMapType,double valueRangeMin,double valueRangeMax); // Creates a default color map for the given value range
	void loadColorMap(const char* colorMapFileName); // Loads a color map from the given color map file
	void saveColorMap(const char* colorMapFileName) const; // Saves color map to the given file
	};

}

#endif
