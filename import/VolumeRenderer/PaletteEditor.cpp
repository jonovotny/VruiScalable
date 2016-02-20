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

#include <stdio.h>
#include <Misc/File.h>
#include <GL/GLColorMap.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/Blind.h>
#include <GLMotif/Label.h>
#include <GLMotif/Button.h>
#include <GLMotif/Slider.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/WidgetManager.h>
#include <Vrui/Vrui.h>

#include "PaletteEditor.h"

/******************************
Methods of class PaletteEditor:
******************************/

void PaletteEditor::selectedControlPointChangedCallback(Misc::CallbackData* cbData)
	{
	GLMotif::ColorMap::SelectedControlPointChangedCallbackData* cbData2=static_cast<GLMotif::ColorMap::SelectedControlPointChangedCallbackData*>(cbData);
	
	if(cbData2->newSelectedControlPoint!=0)
		{
		/* Copy the selected control point's color value to the color editor: */
		GLMotif::ColorMap::ColorMapValue colorValue=colorMap->getSelectedControlPointColorValue();
		colorPanel->setBackgroundColor(colorValue);
		for(int i=0;i<3;++i)
			colorSliders[i]->setValue(colorValue[i]);
		}
	else
		{
		colorPanel->setBackgroundColor(GLMotif::Color(0.5f,0.5f,0.5f));
		for(int i=0;i<3;++i)
			colorSliders[i]->setValue(0.5);
		}
	}

void PaletteEditor::colorSliderValueChangedCallback(Misc::CallbackData* cbData)
	{
	/* Calculate the new selected control point color: */
	GLMotif::ColorMap::ColorMapValue newColor;
	for(int i=0;i<3;++i)
		newColor[i]=colorSliders[i]->getValue();
	newColor[3]=1.0f; // Opacity is ignored, but better to play it safe
	
	/* Copy the new color value to the color panel and the selected control point: */
	colorPanel->setBackgroundColor(newColor);
	colorMap->setSelectedControlPointColorValue(newColor);
	}

void PaletteEditor::removeControlPointCallback(Misc::CallbackData* cbData)
	{
	/* Remove the currently selected control point: */
	colorMap->deleteSelectedControlPoint();
	}

PaletteEditor::PaletteEditor(void)
	:GLMotif::PopupWindow("PaletteEditorPopup",Vrui::getWidgetManager(),"Palette Editor",Vrui::getUiFont())
	{
	const GLMotif::StyleSheet& ss=*Vrui::getWidgetManager()->getStyleSheet();
	
	/* Create the palette editor GUI: */
	GLMotif::PopupWindow::setBorderColor(ss.bgColor);
	GLMotif::PopupWindow::setBackgroundColor(ss.bgColor);
	GLMotif::PopupWindow::setForegroundColor(ss.fgColor);
	GLMotif::PopupWindow::setTitleBarColor(ss.titlebarBgColor);
	GLMotif::PopupWindow::setTitleBarTextColor(ss.titlebarFgColor);
	GLMotif::PopupWindow::setChildBorderWidth(ss.size);
	
	GLMotif::RowColumn* colorMapDialog=new GLMotif::RowColumn("ColorMapDialog",this,false);
	colorMapDialog->setBorderWidth(0.0f);
	colorMapDialog->setOrientation(GLMotif::RowColumn::VERTICAL);
	colorMapDialog->setMarginWidth(0.0f);
	colorMapDialog->setSpacing(ss.size);
	
	colorMap=new GLMotif::ColorMap("ColorMap",colorMapDialog);
	colorMap->setBorderWidth(ss.size*0.5f);
	colorMap->setBorderType(GLMotif::Widget::LOWERED);
	colorMap->setForegroundColor(GLMotif::Color(0.0f,1.0f,0.0f));
	colorMap->setMarginWidth(ss.size);
	colorMap->setPreferredSize(GLMotif::Vector(ss.fontHeight*20.0,ss.fontHeight*10.0,0.0f));
	colorMap->setControlPointSize(ss.size);
	colorMap->setSelectedControlPointColor(GLMotif::Color(1.0f,0.0f,0.0f));
	colorMap->getSelectedControlPointChangedCallbacks().add(this,&PaletteEditor::selectedControlPointChangedCallback);
	
	/* Create the RGB color editor: */
	GLMotif::RowColumn* colorEditor=new GLMotif::RowColumn("ColorEditor",colorMapDialog,false);
	colorEditor->setBorderWidth(0.0f);
	colorEditor->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	colorEditor->setMarginWidth(0.0f);
	colorEditor->setSpacing(ss.size);
	
	new GLMotif::Label("ColorEditorLabel",colorEditor,"Control Point Color:",ss.font);
	
	colorPanel=new GLMotif::Blind("ColorPanel",colorEditor);
	colorPanel->setBorderWidth(ss.size*0.5f);
	colorPanel->setBorderType(GLMotif::Widget::LOWERED);
	colorPanel->setBackgroundColor(GLMotif::Color(0.5f,0.5f,0.5f));
	colorPanel->setPreferredSize(GLMotif::Vector(ss.fontHeight*5.0f,ss.fontHeight*5.0f,0.0f));
	
	GLMotif::RowColumn* colorSlidersBox=new GLMotif::RowColumn("ColorSliders",colorEditor,false);
	colorSlidersBox->setBorderWidth(0.0f);
	colorSlidersBox->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	colorSlidersBox->setPacking(GLMotif::RowColumn::PACK_GRID);
	colorSlidersBox->setMarginWidth(0.0f);
	colorSlidersBox->setSpacing(0.0f);
	
	colorSliders[0]=new GLMotif::Slider("RedSlider",colorSlidersBox,GLMotif::Slider::VERTICAL,ss.sliderWidth,ss.fontHeight*5.0f);
	colorSliders[0]->setSliderColor(GLMotif::Color(1.0f,0.0f,0.0f));
	colorSliders[0]->setShaftColor(ss.sliderShaftColor);
	colorSliders[0]->setValueRange(0.0,1.0,0.01);
	colorSliders[0]->setValue(0.5);
	colorSliders[0]->getValueChangedCallbacks().add(this,&PaletteEditor::colorSliderValueChangedCallback);
	
	colorSliders[1]=new GLMotif::Slider("GreenSlider",colorSlidersBox,GLMotif::Slider::VERTICAL,ss.sliderWidth,ss.fontHeight*5.0f);
	colorSliders[1]->setSliderColor(GLMotif::Color(0.0f,1.0f,0.0f));
	colorSliders[1]->setShaftColor(ss.sliderShaftColor);
	colorSliders[1]->setValueRange(0.0,1.0,0.01);
	colorSliders[1]->setValue(0.5);
	colorSliders[1]->getValueChangedCallbacks().add(this,&PaletteEditor::colorSliderValueChangedCallback);
	
	colorSliders[2]=new GLMotif::Slider("BlueSlider",colorSlidersBox,GLMotif::Slider::VERTICAL,ss.sliderWidth,ss.fontHeight*5.0f);
	colorSliders[2]->setSliderColor(GLMotif::Color(0.0f,0.0f,1.0f));
	colorSliders[2]->setShaftColor(ss.sliderShaftColor);
	colorSliders[2]->setValueRange(0.0,1.0,0.01);
	colorSliders[2]->setValue(0.5);
	colorSliders[2]->getValueChangedCallbacks().add(this,&PaletteEditor::colorSliderValueChangedCallback);
	
	colorSlidersBox->manageChild();
	
	new GLMotif::Blind("Filler",colorEditor);
	
	colorEditor->manageChild();
	
	/* Create the button box: */
	GLMotif::RowColumn* buttonBox=new GLMotif::RowColumn("ButtonBox",colorMapDialog,false);
	buttonBox->setBorderWidth(0.0f);
	buttonBox->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	buttonBox->setMarginWidth(0.0f);
	buttonBox->setSpacing(ss.size);
	
	GLMotif::Button* removeControlPointButton=new GLMotif::Button("RemoveControlPointButton",buttonBox,"Remove Control Point",ss.font);
	removeControlPointButton->getSelectCallbacks().add(this,&PaletteEditor::removeControlPointCallback);
	
	new GLMotif::Blind("Filler",buttonBox);
	
	buttonBox->manageChild();
	
	colorMapDialog->manageChild();
	}

void PaletteEditor::createPalette(PaletteEditor::ColorMapCreationType colorMapType,double valueRangeMin,double valueRangeMax)
	{
	colorMap->createColorMap(colorMapType,valueRangeMin,valueRangeMax);
	}

void PaletteEditor::loadPalette(const char* paletteFileName)
	{
	colorMap->loadColorMap(paletteFileName);
	}

void PaletteEditor::savePalette(const char* paletteFileName) const
	{
	colorMap->saveColorMap(paletteFileName);
	}

void PaletteEditor::exportColorMap(GLColorMap& glColorMap) const
	{
	/* Update the color map's colors: */
	colorMap->exportColorMap(glColorMap);
	
	/* Update the color map's value range: */
	glColorMap.setScalarRange(colorMap->getValueRange().first,colorMap->getValueRange().second);
	}
