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

#include <stdio.h>
#include <Misc/File.h>
#include <Math/Math.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLVertexTemplates.h>
#include <GL/GLColorMap.h>
#include <GLMotif/Event.h>
#include <GLMotif/Container.h>

#include "ColorMap.h"

namespace GLMotif {

/*************************
Methods of class ColorMap:
*************************/

void ColorMap::updateControlPoints(void)
	{
	GLfloat x1=colorMapAreaBox.getCorner(0)[0];
	GLfloat x2=colorMapAreaBox.getCorner(1)[0];
	GLfloat y1=colorMapAreaBox.getCorner(0)[1];
	GLfloat y2=colorMapAreaBox.getCorner(2)[1];
	for(ControlPoint* cpPtr=&first;cpPtr!=0;cpPtr=cpPtr->right)
		{
		cpPtr->x=GLfloat((cpPtr->value-valueRange.first)/(valueRange.second-valueRange.first))*(x2-x1)+x1;
		cpPtr->y=(cpPtr->color[3]-0.0f)*(y2-y1)/(1.0f-0.0f)+y1;
		}
	}

ColorMap::ColorMap(const char* sName,Container* sParent,bool sManageChild)
	:Widget(sName,sParent,false),
	 marginWidth(0.0f),preferredSize(0.0f,0.0f,0.0f),
	 controlPointSize(marginWidth*0.5f),
	 selectedControlPointColor(1.0f,0.0f,0.0f),
	 valueRange(0.0,1.0),
	 first(0.0,ColorMapValue(0.0f,0.0f,0.0f,0.0f)),last(1.0,ColorMapValue(1.0f,1.0f,1.0f,1.0f)),
	 selected(0),isDragging(false)
	{
	/* Link the first and last control points: */
	first.right=&last;
	last.left=&first;
	
	/* Initialize the control poitns: */
	updateControlPoints();
	
	/* Manage me: */
	if(sManageChild)
		manageChild();
	}

ColorMap::~ColorMap(void)
	{
	/* Delete all intermediate control points: */
	ControlPoint* cpPtr=first.right;
	while(cpPtr!=&last)
		{
		ControlPoint* next=cpPtr->right;
		delete cpPtr;
		cpPtr=next;
		}
	}

Vector ColorMap::calcNaturalSize(void) const
	{
	/* Return size of color map area plus margin: */
	Vector result=preferredSize;
	result[0]+=2.0f*marginWidth;
	result[1]+=2.0f*marginWidth;
	
	return calcExteriorSize(result);
	}

void ColorMap::resize(const Box& newExterior)
	{
	/* Resize the parent class widget: */
	Widget::resize(newExterior);
	
	/* Adjust the color map area position: */
	colorMapAreaBox=getInterior();
	colorMapAreaBox.doInset(Vector(marginWidth,marginWidth,0.0f));
	
	/* Update the color map area positions of all control points: */
	updateControlPoints();
	}

void ColorMap::draw(GLContextData& contextData) const
	{
	/* Draw the parent class widget: */
	Widget::draw(contextData);
	
	/* Draw the margin area with the background color: */
	glColor(backgroundColor);
	glNormal3f(0.0f,0.0f,1.0f);
	glBegin(GL_QUADS);
	glVertex(getInterior().getCorner(0));
	glVertex(colorMapAreaBox.getCorner(0));
	glVertex(colorMapAreaBox.getCorner(2));
	glVertex(getInterior().getCorner(2));
	glVertex(getInterior().getCorner(1));
	glVertex(getInterior().getCorner(3));
	glVertex(colorMapAreaBox.getCorner(3));
	glVertex(colorMapAreaBox.getCorner(1));
	glEnd();
	GLfloat x1=colorMapAreaBox.getCorner(0)[0];
	GLfloat x2=colorMapAreaBox.getCorner(1)[0];
	GLfloat y1=colorMapAreaBox.getCorner(0)[1];
	GLfloat y2=colorMapAreaBox.getCorner(2)[1];
	GLfloat z=colorMapAreaBox.getCorner(0)[2];
	glBegin(GL_TRIANGLE_FAN);
	glVertex(getInterior().getCorner(0));
	glVertex(getInterior().getCorner(1));
	for(const ControlPoint* cpPtr=&last;cpPtr!=0;cpPtr=cpPtr->left)
		glVertex3f(cpPtr->x,y1,z);
	glEnd();
	glBegin(GL_TRIANGLE_FAN);
	glVertex(getInterior().getCorner(3));
	glVertex(getInterior().getCorner(2));
	for(const ControlPoint* cpPtr=&first;cpPtr!=0;cpPtr=cpPtr->right)
		glVertex3f(cpPtr->x,y2,z);
	glEnd();
	
	/* Draw the color map area: */
	GLboolean lightingEnabled=glIsEnabled(GL_LIGHTING);
	if(lightingEnabled)
		glDisable(GL_LIGHTING);
	glBegin(GL_QUAD_STRIP);
	for(const ControlPoint* cpPtr=&first;cpPtr!=0;cpPtr=cpPtr->right)
		{
		glColor(cpPtr->color);
		glVertex3f(cpPtr->x,y2,z);
		glVertex3f(cpPtr->x,y1,z);
		}
	glEnd();
	GLfloat lineWidth;
	glGetFloatv(GL_LINE_WIDTH,&lineWidth);
	glLineWidth(3.0f);
	glColor3f(0.0f,0.0f,0.0f);
	glBegin(GL_LINE_STRIP);
	for(const ControlPoint* cpPtr=&first;cpPtr!=0;cpPtr=cpPtr->right)
		glVertex3f(cpPtr->x,cpPtr->y,z+marginWidth*0.25f);
	glEnd();
	glLineWidth(1.0f);
	glColor3f(1.0f,1.0f,1.0f);
	glBegin(GL_LINE_STRIP);
	for(const ControlPoint* cpPtr=&first;cpPtr!=0;cpPtr=cpPtr->right)
		glVertex3f(cpPtr->x,cpPtr->y,z+marginWidth*0.25f);
	glEnd();
	if(lightingEnabled)
		glEnable(GL_LIGHTING);
	glLineWidth(lineWidth);
	
	/* Draw a button for each control point: */
	GLfloat nl=1.0f/Math::sqrt(3.0f);
	glBegin(GL_TRIANGLES);
	for(const ControlPoint* cpPtr=&first;cpPtr!=0;cpPtr=cpPtr->right)
		{
		if(cpPtr==selected)
			glColor(selectedControlPointColor);
		else
			glColor(foregroundColor);
		glNormal3f(-nl,nl,nl);
		glVertex3f(cpPtr->x-controlPointSize,cpPtr->y,z);
		glVertex3f(cpPtr->x,cpPtr->y,z+controlPointSize);
		glVertex3f(cpPtr->x,cpPtr->y+controlPointSize,z);
		glNormal3f(nl,nl,nl);
		glVertex3f(cpPtr->x,cpPtr->y+controlPointSize,z);
		glVertex3f(cpPtr->x,cpPtr->y,z+controlPointSize);
		glVertex3f(cpPtr->x+controlPointSize,cpPtr->y,z);
		glNormal3f(nl,-nl,nl);
		glVertex3f(cpPtr->x+controlPointSize,cpPtr->y,z);
		glVertex3f(cpPtr->x,cpPtr->y,z+controlPointSize);
		glVertex3f(cpPtr->x,cpPtr->y-controlPointSize,z);
		glNormal3f(-nl,-nl,nl);
		glVertex3f(cpPtr->x,cpPtr->y-controlPointSize,z);
		glVertex3f(cpPtr->x,cpPtr->y,z+controlPointSize);
		glVertex3f(cpPtr->x-controlPointSize,cpPtr->y,z);
		}
	glEnd();
	}

bool ColorMap::findRecipient(Event& event)
	{
	if(isDragging)
		{
		/* This event belongs to me! */
		return event.setTargetWidget(this,event.calcWidgetPoint(this));
		}
	else
		return Widget::findRecipient(event);
	}

void ColorMap::pointerButtonDown(Event& event)
	{
	/* Find the closest control point to the event's location: */
	GLfloat minDist2=Math::sqr(controlPointSize*1.5f);
	ControlPoint* newSelected=0;
	GLfloat x1=colorMapAreaBox.getCorner(0)[0];
	GLfloat x2=colorMapAreaBox.getCorner(1)[0];
	GLfloat y1=colorMapAreaBox.getCorner(0)[1];
	GLfloat y2=colorMapAreaBox.getCorner(2)[1];
	GLfloat z=colorMapAreaBox.getCorner(0)[2];
	for(ControlPoint* cpPtr=&first;cpPtr!=0;cpPtr=cpPtr->right)
		{
		Point cp(cpPtr->x,cpPtr->y,z);
		GLfloat dist2=Geometry::sqrDist(cp,event.getWidgetPoint().getPoint());
		if(minDist2>dist2)
			{
			minDist2=dist2;
			newSelected=cpPtr;
			for(int i=0;i<2;++i)
				dragOffset[i]=Scalar(event.getWidgetPoint().getPoint()[i]-cp[i]);
			dragOffset[2]=Scalar(0);
			}
		}
	
	/* Create a new control point if none was selected: */
	if(newSelected==0)
		{
		/* Find the two control points on either side of the selected value: */
		double newValue=(event.getWidgetPoint().getPoint()[0]-double(x1))*(valueRange.second-valueRange.first)/double(x2-x1)+valueRange.first;
		if(newValue<valueRange.first)
			newValue=valueRange.first;
		else if(newValue>valueRange.second)
			newValue=valueRange.second;
		ControlPoint* cpPtr1=&first;
		ControlPoint* cpPtr2;
		for(cpPtr2=cpPtr1->right;cpPtr2!=&last&&cpPtr2->value<newValue;cpPtr1=cpPtr2,cpPtr2=cpPtr2->right)
			;
		
		/* Interpolate the new control point: */
		GLfloat w2=GLfloat((newValue-cpPtr1->value)/(cpPtr2->value-cpPtr1->value));
		GLfloat w1=GLfloat((cpPtr2->value-newValue)/(cpPtr2->value-cpPtr1->value));
		ColorMapValue newColorValue;
		for(int i=0;i<4;++i)
			newColorValue[i]=cpPtr1->color[i]*w1+cpPtr2->color[i]*w2;
		
		/* Insert the new control point: */
		ControlPoint* newCp=new ControlPoint(newValue,newColorValue);
		newCp->left=cpPtr1;
		cpPtr1->right=newCp;
		newCp->right=cpPtr2;
		cpPtr2->left=newCp;
		
		/* Update all control points: */
		updateControlPoints();
		
		/* Call the color map change callbacks: */
		ColorMapChangedCallbackData cbData(this);
		colorMapChangedCallbacks.call(&cbData);
		
		/* Select the new control point: */
		newSelected=newCp;
		}
	else
		{
		/* Start dragging the selected control point: */
		isDragging=true;
		}
	
	/* Call callbacks if selected control point has changed: */
	if(newSelected!=selected)
		{
		SelectedControlPointChangedCallbackData cbData(this,selected,newSelected);
		selected=newSelected;
		selectedControlPointChangedCallbacks.call(&cbData);
		}
	}

void ColorMap::pointerButtonUp(Event& event)
	{
	if(isDragging)
		{
		/* Stop dragging: */
		isDragging=false;
		}
	}

void ColorMap::pointerMotion(Event& event)
	{
	if(isDragging)
		{
		/* Calculate the new value and opacity: */
		GLfloat x1=colorMapAreaBox.getCorner(0)[0];
		GLfloat x2=colorMapAreaBox.getCorner(1)[0];
		GLfloat y1=colorMapAreaBox.getCorner(0)[1];
		GLfloat y2=colorMapAreaBox.getCorner(2)[1];
		Point p=event.getWidgetPoint().getPoint()-dragOffset;
		double newValue=(p[0]-double(x1))*(valueRange.second-valueRange.first)/double(x2-x1)+valueRange.first;
		if(selected==&first)
			newValue=valueRange.first;
		else if(selected==&last)
			newValue=valueRange.second;
		else if(newValue<selected->left->value)
			newValue=selected->left->value;
		else if(newValue>selected->right->value)
			newValue=selected->right->value;
		GLfloat newOpacity=(p[1]-y1)*(1.0f-0.0f)/(y2-y1)+0.0f;
		if(newOpacity<0.0f)
			newOpacity=0.0f;
		else if(newOpacity>1.0f)
			newOpacity=1.0f;
		selected->value=newValue;
		selected->color[3]=newOpacity;
		
		/* Update all control points: */
		updateControlPoints();
		
		/* Call the color map change callbacks: */
		ColorMapChangedCallbackData cbData(this);
		colorMapChangedCallbacks.call(&cbData);
		}
	}

void ColorMap::setMarginWidth(GLfloat newMarginWidth)
	{
	/* Set the new margin width: */
	marginWidth=newMarginWidth;
	
	if(isManaged)
		{
		/* Try adjusting the widget size to accomodate the new margin width: */
		parent->requestResize(this,calcNaturalSize());
		}
	else
		resize(Box(Vector(0.0f,0.0f,0.0f),calcNaturalSize()));
	}

void ColorMap::setPreferredSize(const Vector& newPreferredSize)
	{
	/* Set the new preferred size: */
	preferredSize=newPreferredSize;
	
	if(isManaged)
		{
		/* Try adjusting the widget size to accomodate the new preferred size: */
		parent->requestResize(this,calcNaturalSize());
		}
	else
		resize(Box(Vector(0.0f,0.0f,0.0f),calcNaturalSize()));
	}

void ColorMap::setControlPointSize(GLfloat newControlPointSize)
	{
	/* Set the new control point size: */
	controlPointSize=newControlPointSize;
	}

void ColorMap::setSelectedControlPointColor(const Color& newSelectedControlPointColor)
	{
	/* Set the new selected control point color: */
	selectedControlPointColor=newSelectedControlPointColor;
	}

int ColorMap::getNumControlPoints(void) const
	{
	int result=0;
	for(const ControlPoint* cpPtr=&first;cpPtr!=0;cpPtr=cpPtr->right)
		++result;
	return result;
	}

void ColorMap::selectControlPoint(int controlPointIndex)
	{
	ControlPoint* cpPtr=0;
	if(controlPointIndex>=0)
		for(cpPtr=&first;controlPointIndex>0&&cpPtr!=0;cpPtr=cpPtr->right,--controlPointIndex)
			;
	
	/* Select the new control point: */
	SelectedControlPointChangedCallbackData cbData(this,selected,cpPtr);
	selected=cpPtr;
	selectedControlPointChangedCallbacks.call(&cbData);
	}

void ColorMap::insertControlPoint(double newControlPointValue)
	{
	/* Insert the new control point if it is inside the value range: */
	if(newControlPointValue>=valueRange.first&&newControlPointValue<=valueRange.second)
		{
		/* Find the two control points on either side of the new control point: */
		ControlPoint* cpPtr1=&first;
		ControlPoint* cpPtr2;
		for(cpPtr2=cpPtr1->right;cpPtr2!=&last&&cpPtr2->value<newControlPointValue;cpPtr1=cpPtr2,cpPtr2=cpPtr2->right)
			;
		
		/* Interpolate the new control point: */
		GLfloat w2=GLfloat((newControlPointValue-cpPtr1->value)/(cpPtr2->value-cpPtr1->value));
		GLfloat w1=GLfloat((cpPtr2->value-newControlPointValue)/(cpPtr2->value-cpPtr1->value));
		ColorMapValue newControlPointColor;
		for(int i=0;i<4;++i)
			newControlPointColor[i]=cpPtr1->color[i]*w1+cpPtr2->color[i]*w2;
		
		/* Insert the new control point: */
		ControlPoint* newCp=new ControlPoint(newControlPointValue,newControlPointColor);
		newCp->left=cpPtr1;
		cpPtr1->right=newCp;
		newCp->right=cpPtr2;
		cpPtr2->left=newCp;
		
		/* Update all control points: */
		updateControlPoints();
		
		{
		/* Call the color map change callbacks: */
		ColorMapChangedCallbackData cbData(this);
		colorMapChangedCallbacks.call(&cbData);
		}
		
		{
		/* Select the new control point: */
		SelectedControlPointChangedCallbackData cbData(this,selected,newCp);
		selected=newCp;
		selectedControlPointChangedCallbacks.call(&cbData);
		}
		}
	}

void ColorMap::insertControlPoint(double newControlPointValue,const ColorMap::ColorMapValue& newControlPointColor)
	{
	/* Insert the new control point if it is inside the value range: */
	if(newControlPointValue>=valueRange.first&&newControlPointValue<=valueRange.second)
		{
		/* Find the two control points on either side of the new control point: */
		ControlPoint* cpPtr1=&first;
		ControlPoint* cpPtr2;
		for(cpPtr2=cpPtr1->right;cpPtr2!=&last&&cpPtr2->value<newControlPointValue;cpPtr1=cpPtr2,cpPtr2=cpPtr2->right)
			;
		
		/* Insert the new control point: */
		ControlPoint* newCp=new ControlPoint(newControlPointValue,newControlPointColor);
		newCp->left=cpPtr1;
		cpPtr1->right=newCp;
		newCp->right=cpPtr2;
		cpPtr2->left=newCp;
		
		/* Update all control points: */
		updateControlPoints();
		
		{
		/* Call the color map change callbacks: */
		ColorMapChangedCallbackData cbData(this);
		colorMapChangedCallbacks.call(&cbData);
		}
		
		{
		/* Select the new control point: */
		SelectedControlPointChangedCallbackData cbData(this,selected,newCp);
		selected=newCp;
		selectedControlPointChangedCallbacks.call(&cbData);
		}
		}
	}

void ColorMap::deleteSelectedControlPoint(void)
	{
	/* Remove the selected control point if possible: */
	if(selected!=0&&selected!=&first&&selected!=&last)
		{
		ControlPoint* delPtr=selected;
		
		{
		/* Unselect the selected control point: */
		SelectedControlPointChangedCallbackData cbData(this,selected,0);
		selected=0;
		selectedControlPointChangedCallbacks.call(&cbData);
		}
		
		/* Remove the control point from the list: */
		delPtr->left->right=delPtr->right;
		delPtr->right->left=delPtr->left;
		delete delPtr;
		
		/* Update all control points: */
		updateControlPoints();
		
		{
		/* Call the color map change callbacks: */
		ColorMapChangedCallbackData cbData(this);
		colorMapChangedCallbacks.call(&cbData);
		}
		}
	}

void ColorMap::setSelectedControlPointValue(double newControlPointValue)
	{
	if(selected!=0&&selected->left!=0&&selected->right!=0)
		{
		if(newControlPointValue<first.value)
			selected->value=first.value;
		else if(newControlPointValue>last.value)
			selected->value=last.value;
		else
			selected->value=newControlPointValue;
		
		/* Update all control points: */
		updateControlPoints();
		
		/* Call the color map change callbacks: */
		ColorMapChangedCallbackData cbData(this);
		colorMapChangedCallbacks.call(&cbData);
		}
	}

void ColorMap::setSelectedControlPointColorValue(const ColorMap::ColorMapValue& newControlPointColorValue)
	{
	if(selected!=0)
		{
		/* Set the control point's color value's RGB components: */
		for(int i=0;i<3;++i)
			selected->color[i]=newControlPointColorValue[i];
		
		/* Update all control points: */
		updateControlPoints();
		
		/* Call the color map change callbacks: */
		ColorMapChangedCallbackData cbData(this);
		colorMapChangedCallbacks.call(&cbData);
		}
	}

void ColorMap::exportColorMap(GLColorMap& glColorMap) const
	{
	/* Get the number of colors from the GL color map and create the new color array: */
	int numEntries=glColorMap.getNumEntries();
	GLColorMap::Color* entries=new GLColorMap::Color[numEntries];
	
	/* Interpolate all colors in the color array: */
	for(int i=0;i<numEntries;++i)
		{
		/* Calculate the map value for the color array entry: */
		double value=double(i)*(valueRange.second-valueRange.first)/double(numEntries-1)+valueRange.first;
		
		/* Find the two control points on either side of the value: */
		const ControlPoint* cpPtr1=&first;
		const ControlPoint* cpPtr2;
		for(cpPtr2=cpPtr1->right;cpPtr2!=&last&&cpPtr2->value<value;cpPtr1=cpPtr2,cpPtr2=cpPtr2->right)
			;
		
		/* Interpolate the new control point: */
		GLfloat w2=GLfloat((value-cpPtr1->value)/(cpPtr2->value-cpPtr1->value));
		GLfloat w1=GLfloat((cpPtr2->value-value)/(cpPtr2->value-cpPtr1->value));
		for(int j=0;j<4;++j)
			entries[i][j]=cpPtr1->color[j]*w1+cpPtr2->color[j]*w2;
		}
	
	/* Set the color map's color array: */
	glColorMap.setColors(numEntries,entries);
	delete[] entries;
	}

void ColorMap::createColorMap(ColorMap::ColorMapCreationType colorMapType,double valueRangeMin,double valueRangeMax)
	{
	if(selected!=0)
		{
		/* Deselect the selected control point: */
		SelectedControlPointChangedCallbackData cbData(this,selected,0);
		selected=0;
		selectedControlPointChangedCallbacks.call(&cbData);
		}
	
	/* Delete all intermediate control points: */
	ControlPoint* cpPtr=first.right;
	while(cpPtr!=&last)
		{
		ControlPoint* next=cpPtr->right;
		delete cpPtr;
		cpPtr=next;
		}
	first.right=&last;
	last.left=&first;
	
	if(colorMapType==GREYSCALE)
		{
		/* Create a greyscale color map: */
		first.value=valueRangeMin;
		first.color=ColorMapValue(0.0f,0.0f,0.0f,0.0f);
		last.value=valueRangeMax;
		last.color=ColorMapValue(1.0f,1.0f,1.0f,1.0f);
		}
	else
		{
		/* Create a rainbow color map: */
		first.value=valueRangeMin;
		first.color=ColorMapValue(1.0f,0.0f,0.0f,0.0f);
		ControlPoint* cs[6];
		cs[0]=&first;
		cs[1]=new ControlPoint((1.0/5.0)*(valueRangeMax-valueRangeMin)+valueRangeMin,ColorMapValue(1.0f,1.0f,0.0f,1.0f/5.0f));
		cs[2]=new ControlPoint((2.0/5.0)*(valueRangeMax-valueRangeMin)+valueRangeMin,ColorMapValue(0.0f,1.0f,0.0f,2.0f/5.0f));
		cs[3]=new ControlPoint((3.0/5.0)*(valueRangeMax-valueRangeMin)+valueRangeMin,ColorMapValue(0.0f,1.0f,1.0f,3.0f/5.0f));
		cs[4]=new ControlPoint((4.0/5.0)*(valueRangeMax-valueRangeMin)+valueRangeMin,ColorMapValue(0.0f,0.0f,1.0f,4.0f/5.0f));
		cs[5]=&last;
		last.value=valueRangeMax;
		last.color=ColorMapValue(1.0f,0.0f,1.0f,1.0f);
		for(int i=0;i<5;++i)
			{
			cs[i+1]->left=cs[i];
			cs[i]->right=cs[i+1];
			}
		}
	
	/* Update the value range: */
	valueRange.first=valueRangeMin;
	valueRange.second=valueRangeMax;
	
	/* Update all control points: */
	updateControlPoints();
	
	/* Call the color map change callbacks: */
	ColorMapChangedCallbackData cbData(this);
	colorMapChangedCallbacks.call(&cbData);
	}

void ColorMap::loadColorMap(const char* colorMapFileName)
	{
	/* Open the color map file: */
	Misc::File colorMapFile(colorMapFileName,"rt");
	
	if(selected!=0)
		{
		/* Deselect the selected control point: */
		SelectedControlPointChangedCallbackData cbData(this,selected,0);
		selected=0;
		selectedControlPointChangedCallbacks.call(&cbData);
		}
	
	/* Delete all intermediate control points: */
	ControlPoint* cpPtr=first.right;
	while(cpPtr!=&last)
		{
		ControlPoint* next=cpPtr->right;
		delete cpPtr;
		cpPtr=next;
		}
	first.right=&last;
	last.left=&first;
	
	/* Read all control points from the color map file: */
	int numPointsSet=0;
	while(!colorMapFile.eof())
		{
		/* Read the next line from the file: */
		char line[256];
		colorMapFile.gets(line,sizeof(line));
		
		/* Extract the control point from the line (ignore blank and comment lines): */
		double value;
		ColorMapValue color;
		if(line[0]!='#'&&sscanf(line,"%lf %f %f %f %f",&value,&color[0],&color[1],&color[2],&color[3])==5)
			{
			if(numPointsSet==0)
				{
				/* Set the first control point: */
				first.value=value;
				first.color=color;
				}
			else
				{
				if(numPointsSet>1)
					{
					/* Copy the last control point to an intermediate one: */
					ControlPoint* newCp=new ControlPoint(last.value,last.color);
					newCp->left=last.left;
					newCp->left->right=newCp;
					newCp->right=&last;
					last.left=newCp;
					}
				
				/* Set the last control point: */
				last.value=value;
				last.color=color;
				}
			++numPointsSet;
			}
		}
	
	/* Update the value range: */
	valueRange.first=first.value;
	valueRange.second=last.value;
	
	/* Update all control points: */
	updateControlPoints();
	
	/* Call the color map change callbacks: */
	ColorMapChangedCallbackData cbData(this);
	colorMapChangedCallbacks.call(&cbData);
	}

void ColorMap::saveColorMap(const char* colorMapFileName) const
	{
	/* Open the color map file: */
	Misc::File colorMapFile(colorMapFileName,"wt");
	
	/* Write all control points to the file: */
	for(const ControlPoint* cpPtr=&first;cpPtr!=0;cpPtr=cpPtr->right)
		fprintf(colorMapFile.getFilePtr(),"%lf %f %f %f %f\n",cpPtr->value,cpPtr->color[0],cpPtr->color[1],cpPtr->color[2],cpPtr->color[3]);
	}

}
