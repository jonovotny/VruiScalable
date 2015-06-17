/***********************************************************************
VRVolumeRenderer - Test program for texture-based volume rendering
classes.
Copyright (c) 2001-2006 Oliver Kreylos

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

#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <stdexcept>
#include <Misc/File.h>
#include <Geometry/Vector.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Geometry/OrthogonalTransformation.h>
#include <GL/gl.h>
#include <GL/GLColorMap.h>
#include <GL/GLContextData.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/Label.h>
#include <GLMotif/Menu.h>
#include <GLMotif/Popup.h>
#include <GLMotif/PopupMenu.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/Slider.h>
#include <GLMotif/ToggleButton.h>
#include <GLMotif/WidgetManager.h>
#include <PaletteRenderer.h>
#include <Vrui/LocatorTool.h>
#include <Vrui/LocatorToolAdapter.h>
#include <Vrui/ToolManager.h>
#include <Vrui/Vrui.h>
#include <Vrui/Application.h>

#include "PaletteEditor.h"

class VRVolumeRenderer:public Vrui::Application
	{
	/* Embedded classes: */
	private:
	class CuttingPlaneLocator:public Vrui::LocatorToolAdapter // Class to create and manipulate cutting planes
		{
		/* Elements: */
		private:
		GLuint clipPlaneIndex; // Index of OpenGL clip plane used for this tool
		bool active; // Flag whether the cutting plane is enabled
		Vrui::Vector planeNormal; // Normal vector of the cutting plane
		Vrui::Scalar planeOffset; // Offset of the cutting plane
		
		/* Constructors and destructors: */
		public:
		CuttingPlaneLocator(Vrui::LocatorTool* sTool,GLuint sClipPlaneIndex);
		
		/* Methods: */
		virtual void motionCallback(Vrui::LocatorTool::MotionCallbackData* cbData);
		virtual void buttonPressCallback(Vrui::LocatorTool::ButtonPressCallbackData* cbData);
		virtual void buttonReleaseCallback(Vrui::LocatorTool::ButtonReleaseCallbackData* cbData);
		
		/* New methods: */
		int getClipPlaneIndex(void) const // Returns the index of the OpenGL clipping plane allocated for the tool
			{
			return clipPlaneIndex;
			};
		void setGLState(void) const; // Sets OpenGL state for cutting plane rendering
		void resetGLState(void) const; // Resets OpenGL state after rendering
		};
	
	/* Elements: */
	
	/* Model state: */
	PaletteRenderer* renderer; // Pointer to used volume renderer
	GLColorMap* palette; // Pointer to the used color map
	VolumeRenderer::Scalar sliceFactor; // Number of textured slices to generate per cell
	GLfloat transparencyGamma; // Gamma factor for the color map's transparency component
	
	/* UI widgets: */
	GLMotif::PopupMenu* mainMenu; // Program's main menu
	PaletteEditor* paletteEditor; // Pointer to the color map editor
	GLMotif::PopupWindow* renderSettingsDialog; // Dialog box for rendering settings
	GLMotif::Label* sliceFactorValue; // Text field for slice factor value
	GLMotif::Slider* sliceFactorSlider; // Slider for slice factor value
	GLMotif::Label* transparencyGammaValue; // Text field for transparency gamma factor
	GLMotif::Slider* transparencyGammaSlider; // Slider for transparency gamma factor
	
	/* Interaction parameters: */
	VolumeRenderer::Vector viewDirection; // Current view direction
	int numClipPlanes; // Number of clipping planes supported by OpenGL
	bool* clipPlaneAllocateds; // Array of allocation flags for the OpenGL clipping planes
	std::vector<CuttingPlaneLocator*> cuttingPlanes; // List of cutting planes
	
	/* Private methods: */
	GLMotif::PopupMenu* createMainMenu(void);
	GLMotif::PopupWindow* createRenderSettingsDialog(void);
	
	/* Constructors and destructors: */
	public:
	VRVolumeRenderer(int& argc,char**& argv,char**& appDefaults);
	virtual ~VRVolumeRenderer(void);
	
	/* Methods: */
	virtual void toolCreationCallback(Vrui::ToolManager::ToolCreationCallbackData* cbData);
	virtual void toolDestructionCallback(Vrui::ToolManager::ToolDestructionCallbackData* cbData);
	virtual void display(GLContextData& contextData) const;
	virtual void frame(void);
	void centerDisplayCallback(Misc::CallbackData* cbData);
	void colorMapChangedCallback(Misc::CallbackData* cbData);
	void showPaletteEditorCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
	void savePaletteCallback(Misc::CallbackData* cbData);
	void showRenderSettingsDialogCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
	void sliderValueChangedCallback(GLMotif::Slider::ValueChangedCallbackData* cbData);
	void createInputDeviceCallback(Misc::CallbackData* cbData);
	void saveViewCallback(Misc::CallbackData* cbData);
	void loadViewCallback(Misc::CallbackData* cbData);
	};

/******************************************************
Methods of class VRVolumeRenderer::CuttingPlaneLocator:
******************************************************/

VRVolumeRenderer::CuttingPlaneLocator::CuttingPlaneLocator(Vrui::LocatorTool* sTool,GLuint sClipPlaneIndex)
	:Vrui::LocatorToolAdapter(sTool),
	 clipPlaneIndex(sClipPlaneIndex),
	 active(false)
	{
	}

void VRVolumeRenderer::CuttingPlaneLocator::motionCallback(Vrui::LocatorTool::MotionCallbackData* cbData)
	{
	/* Update the cutting plane's equation if it is active: */
	if(active)
		{
		planeNormal=cbData->currentTransformation.transform(Vrui::Vector(0,1,0));
		planeOffset=cbData->currentTransformation.getOrigin()*planeNormal;
		}
	}

void VRVolumeRenderer::CuttingPlaneLocator::buttonPressCallback(Vrui::LocatorTool::ButtonPressCallbackData* cbData)
	{
	/* Activate the cutting plane: */
	active=true;
	}

void VRVolumeRenderer::CuttingPlaneLocator::buttonReleaseCallback(Vrui::LocatorTool::ButtonReleaseCallbackData* cbData)
	{
	/* Deactivate the cutting plane: */
	active=false;
	}

void VRVolumeRenderer::CuttingPlaneLocator::setGLState(void) const
	{
	if(active)
		{
		/* Enable the clipping plane: */
		glEnable(GL_CLIP_PLANE0+clipPlaneIndex);
		
		/* Set the clipping plane's equation: */
		double comp[4];
		for(int i=0;i<3;++i)
			comp[i]=double(planeNormal[i]);
		comp[3]=double(-planeOffset);
		glClipPlane(GL_CLIP_PLANE0+clipPlaneIndex,comp);
		}
	}

void VRVolumeRenderer::CuttingPlaneLocator::resetGLState(void) const
	{
	if(active)
		{
		/* Disable the clipping plane: */
		glDisable(GL_CLIP_PLANE0+clipPlaneIndex);
		}
	}

/*********************************
Methods of class VRVolumeRenderer:
*********************************/

GLMotif::PopupMenu* VRVolumeRenderer::createMainMenu(void)
	{
	const GLMotif::StyleSheet& ss=*Vrui::getWidgetManager()->getStyleSheet();
	
	GLMotif::PopupMenu* mainMenuPopup=new GLMotif::PopupMenu("MainMenuPopup",Vrui::getWidgetManager());
	mainMenuPopup->setBorderWidth(0.0f);
	mainMenuPopup->setBorderType(GLMotif::Widget::RAISED);
	mainMenuPopup->setBorderColor(ss.bgColor);
	mainMenuPopup->setBackgroundColor(ss.bgColor);
	mainMenuPopup->setForegroundColor(ss.fgColor);
	mainMenuPopup->setMarginWidth(ss.size);
	mainMenuPopup->setTitleSpacing(ss.size);
	mainMenuPopup->setTitle("VR Volume Renderer",ss.font);
	
	GLMotif::Menu* mainMenu=new GLMotif::Menu("MainMenu",mainMenuPopup,false);
	mainMenu->setBorderWidth(0.0f);
	mainMenu->setOrientation(GLMotif::RowColumn::VERTICAL);
	mainMenu->setNumMinorWidgets(1);
	mainMenu->setMarginWidth(0.0f);
	mainMenu->setSpacing(ss.size);
	
	GLMotif::Button* centerDisplayButton=new GLMotif::Button("CenterDisplayButton",mainMenu,"Center Display",ss.font);
	centerDisplayButton->getSelectCallbacks().add(this,&VRVolumeRenderer::centerDisplayCallback);
	
	GLMotif::ToggleButton* showPaletteEditorToggle=new GLMotif::ToggleButton("ShowPaletteEditorToggle",mainMenu,"Show Palette Editor",ss.font);
	showPaletteEditorToggle->getValueChangedCallbacks().add(this,&VRVolumeRenderer::showPaletteEditorCallback);
	showPaletteEditorToggle->setToggle(false);
	
	GLMotif::Button* savePaletteButton=new GLMotif::Button("SavePaletteButton",mainMenu,"Save Palette",ss.font);
	savePaletteButton->getSelectCallbacks().add(this,&VRVolumeRenderer::savePaletteCallback);
	
	GLMotif::ToggleButton* showRenderSettingsDialogToggle=new GLMotif::ToggleButton("ShowRenderSettingsDialogToggle",mainMenu,"Show Render Settings Dialog",ss.font);
	showRenderSettingsDialogToggle->getValueChangedCallbacks().add(this,&VRVolumeRenderer::showRenderSettingsDialogCallback);
	showRenderSettingsDialogToggle->setToggle(false);
	
	GLMotif::Button* createInputDeviceButton=new GLMotif::Button("CreateInputDeviceButton",mainMenu,"Create Input Device",ss.font);
	createInputDeviceButton->getSelectCallbacks().add(this,&VRVolumeRenderer::createInputDeviceCallback);
	
	GLMotif::Button* saveViewButton=new GLMotif::Button("SaveViewButton",mainMenu,"Save View",ss.font);
	saveViewButton->getSelectCallbacks().add(this,&VRVolumeRenderer::saveViewCallback);
	
	GLMotif::Button* loadViewButton=new GLMotif::Button("LoadViewButton",mainMenu,"Load View",ss.font);
	loadViewButton->getSelectCallbacks().add(this,&VRVolumeRenderer::loadViewCallback);
	
	mainMenu->manageChild();
	
	return mainMenuPopup;
	}

GLMotif::PopupWindow* VRVolumeRenderer::createRenderSettingsDialog(void)
	{
	const GLMotif::StyleSheet& ss=*Vrui::getWidgetManager()->getStyleSheet();
	
	GLMotif::PopupWindow* renderSettingsDialog=new GLMotif::PopupWindow("RenderSettingsDialog",Vrui::getWidgetManager(),"Rendering Settings",ss.font);
	renderSettingsDialog->setBorderColor(ss.bgColor);
	renderSettingsDialog->setBackgroundColor(ss.bgColor);
	renderSettingsDialog->setForegroundColor(ss.fgColor);
	renderSettingsDialog->setTitleBarColor(ss.titlebarBgColor);
	renderSettingsDialog->setTitleBarTextColor(ss.titlebarFgColor);
	renderSettingsDialog->setChildBorderWidth(ss.size);
	
	GLMotif::RowColumn* renderSettings=new GLMotif::RowColumn("RenderSettings",renderSettingsDialog,false);
	renderSettings->setBorderWidth(0.0f);
	renderSettings->setOrientation(GLMotif::RowColumn::VERTICAL);
	renderSettings->setNumMinorWidgets(3);
	renderSettings->setMarginWidth(0.0f);
	renderSettings->setSpacing(ss.size);
	
	/* Create a slider/textfield combo to change the slice factor: */
	GLMotif::Label* sliceFactorLabel=new GLMotif::Label("SliceFactorLabel",renderSettings,"Slice Factor",ss.font);
	
	char sliceFactorValueString[40];
	snprintf(sliceFactorValueString,sizeof(sliceFactorValueString),"%4.2lf",double(sliceFactor));
	sliceFactorValue=new GLMotif::Label("SliceFactorValue",renderSettings,sliceFactorValueString,ss.font);
	sliceFactorValue->setBorderWidth(ss.size*0.5f);
	sliceFactorValue->setBorderType(GLMotif::Widget::LOWERED);
	sliceFactorValue->setBackgroundColor(ss.textfieldBgColor);
	sliceFactorValue->setForegroundColor(ss.textfieldFgColor);
	sliceFactorValue->setMarginWidth(ss.size*0.5f);
	sliceFactorValue->setHAlignment(GLFont::Right);
	
	sliceFactorSlider=new GLMotif::Slider("SliceFactorSlider",renderSettings,GLMotif::Slider::HORIZONTAL,ss.sliderWidth,ss.fontHeight*10.0f);
	sliceFactorSlider->setSliderColor(ss.sliderHandleColor);
	sliceFactorSlider->setShaftColor(ss.sliderShaftColor);
	sliceFactorSlider->setValueRange(0.1,4.0,0.01);
	sliceFactorSlider->setValue(double(sliceFactor));
	sliceFactorSlider->getValueChangedCallbacks().add(this,&VRVolumeRenderer::sliderValueChangedCallback);
	
	/* Create a slider/textfield combo to change the transparency gamma factor: */
	GLMotif::Label* transparencyGammaLabel=new GLMotif::Label("TransparencyGammaLabel",renderSettings,"Transparency Gamma",ss.font);
	
	char transparencyGammaValueString[40];
	snprintf(transparencyGammaValueString,sizeof(transparencyGammaValueString),"%4.2lf",double(transparencyGamma));
	transparencyGammaValue=new GLMotif::Label("TransparencyGammaValue",renderSettings,transparencyGammaValueString,ss.font);
	transparencyGammaValue->setBorderWidth(ss.size*0.5f);
	transparencyGammaValue->setBorderType(GLMotif::Widget::LOWERED);
	transparencyGammaValue->setBackgroundColor(ss.textfieldBgColor);
	transparencyGammaValue->setForegroundColor(ss.textfieldFgColor);
	transparencyGammaValue->setMarginWidth(ss.size*0.5f);
	transparencyGammaValue->setHAlignment(GLFont::Right);
	
	transparencyGammaSlider=new GLMotif::Slider("TransparencyGammaSlider",renderSettings,GLMotif::Slider::HORIZONTAL,ss.sliderWidth,ss.fontHeight*10.0f);
	transparencyGammaSlider->setSliderColor(ss.sliderHandleColor);
	transparencyGammaSlider->setShaftColor(ss.sliderShaftColor);
	transparencyGammaSlider->setValueRange(0.1,4.0,0.01);
	transparencyGammaSlider->setValue(double(transparencyGamma));
	transparencyGammaSlider->getValueChangedCallbacks().add(this,&VRVolumeRenderer::sliderValueChangedCallback);
	
	renderSettings->manageChild();
	
	return renderSettingsDialog;
	}

VRVolumeRenderer::VRVolumeRenderer(int& argc,char**& argv,char**& appDefaults)
	:Vrui::Application(argc,argv,appDefaults),
	 renderer(0),palette(0),sliceFactor(1),transparencyGamma(1.0f),
	 mainMenu(0),
	 paletteEditor(0),
	 renderSettingsDialog(0),sliceFactorValue(0),transparencyGammaValue(0),
	 numClipPlanes(0),clipPlaneAllocateds(0)
	{
	/* Parse the command line: */
	char* volumeFileName=0;
	char* paletteFileName=0;
	char* viewFileName=0;
	for(int i=1;i<argc;++i)
		{
		if(argv[i][0]=='-')
			{
			if(strcasecmp(argv[i]+1,"VIEW")==0)
				{
				++i;
				viewFileName=argv[i];
				}
			else if(strcasecmp(argv[i]+1,"SLICE")==0)
				{
				++i;
				sliceFactor=atof(argv[i]);
				transparencyGamma=float(sliceFactor);
				}
			}
		else if(volumeFileName==0)
			volumeFileName=argv[i];
		else
			paletteFileName=argv[i];
		}
	
	if(volumeFileName==0)
		throw std::runtime_error("VRVolumeRenderer: Volume data file name required");
	
	/* Load palette renderer from file: */
	renderer=new PaletteRenderer(volumeFileName);
	
	/* Initialize renderer settings: */
	// renderer->setUseNPOTDTextures(true);
	renderer->setVoxelAlignment(VolumeRenderer::CELL_CENTERED);
	// renderer->setRenderingMode(VolumeRenderer::AXIS_ALIGNED);
	renderer->setRenderingMode(VolumeRenderer::VIEW_PERPENDICULAR);
	renderer->setInterpolationMode(VolumeRenderer::LINEAR);
	renderer->setTextureFunction(VolumeRenderer::REPLACE);
	renderer->setSliceFactor(sliceFactor);
	renderer->setAutosaveGLState(true);
	renderer->setTextureCaching(true);
	renderer->setSharePalette(false);
	
	/* Create the user interface: */
	mainMenu=createMainMenu();
	Vrui::setMainMenu(mainMenu);
	
	/* Initialize the palette: */
	palette=new GLColorMap(GLColorMap::RAINBOW|GLColorMap::RAMP_ALPHA,1.0f,1.0f,0.0,255.0);
	paletteEditor=new PaletteEditor;
	paletteEditor->getColorMapChangedCallbacks().add(this,&VRVolumeRenderer::colorMapChangedCallback);
	if(paletteFileName!=0)
		paletteEditor->loadPalette(paletteFileName);
	else
		paletteEditor->createPalette(GLMotif::ColorMap::RAINBOW,0.0,255.0);
	
	/* Create the render settings dialog: */
	renderSettingsDialog=createRenderSettingsDialog();
	
	/* Initialize navigation transformation: */
	if(viewFileName!=0)
		{
		/* Open viewpoint file: */
		Misc::File viewpointFile(viewFileName,"rb",Misc::File::LittleEndian);
		
		/* Read the navigation transformation: */
		Vrui::NavTransform::Vector translation;
		viewpointFile.read(translation.getComponents(),3);
		Vrui::NavTransform::Rotation::Scalar quaternion[4];
		viewpointFile.read(quaternion,4);
		Vrui::NavTransform::Scalar scaling=viewpointFile.read<Vrui::NavTransform::Scalar>();
		
		/* Set the navigation transformation: */
		Vrui::setNavigationTransformation(Vrui::NavTransform(translation,Vrui::NavTransform::Rotation::fromQuaternion(quaternion),scaling));
		}
	else
		centerDisplayCallback(0);
	
	/* Create the OpenGL clipping plane allocation array: */
	// glGetIntegerv(GL_MAX_CLIP_PLANES,&numClipPlanes);
	numClipPlanes=6;
	clipPlaneAllocateds=new bool[numClipPlanes];
	for(int i=0;i<numClipPlanes;++i)
		clipPlaneAllocateds[i]=false;
	};

VRVolumeRenderer::~VRVolumeRenderer(void)
	{
	delete renderer;
	delete palette;
	delete mainMenu;
	delete paletteEditor;
	delete renderSettingsDialog;
	delete clipPlaneAllocateds;
	
	/* Delete all cutting plane tools: */
	for(std::vector<CuttingPlaneLocator*>::iterator cpIt=cuttingPlanes.begin();cpIt!=cuttingPlanes.end();++cpIt)
		delete *cpIt;
	}

void VRVolumeRenderer::toolCreationCallback(Vrui::ToolManager::ToolCreationCallbackData* cbData)
	{
	/* Check if the new tool is a locator tool: */
	Vrui::LocatorTool* locatorTool=dynamic_cast<Vrui::LocatorTool*>(cbData->tool);
	if(locatorTool!=0)
		{
		/* Check if OpenGL has another clipping plane available: */
		for(int i=0;i<numClipPlanes;++i)
			if(!clipPlaneAllocateds[i])
				{
				/* Create a cutting plane locator object and associate it with the new tool: */
				CuttingPlaneLocator* newLocator=new CuttingPlaneLocator(locatorTool,i);
				
				/* Mark the OpenGL clipping plane as allocated: */
				clipPlaneAllocateds[i]=true;
				
				/* Add new locator to list: */
				cuttingPlanes.push_back(newLocator);
				
				break;
				}
		}
	}

void VRVolumeRenderer::toolDestructionCallback(Vrui::ToolManager::ToolDestructionCallbackData* cbData)
	{
	/* Check if the to-be-destroyed tool is a locator tool: */
	Vrui::LocatorTool* locatorTool=dynamic_cast<Vrui::LocatorTool*>(cbData->tool);
	if(locatorTool!=0)
		{
		/* Find the data locator associated with the tool in the list: */
		std::vector<CuttingPlaneLocator*>::iterator cpIt;
		for(cpIt=cuttingPlanes.begin();cpIt!=cuttingPlanes.end();++cpIt)
			if((*cpIt)->getTool()==locatorTool)
				{
				/* Release the OpenGL clipping plane allocated for the tool: */
				clipPlaneAllocateds[(*cpIt)->getClipPlaneIndex()]=false;
				
				/* Remove the cutting plane locator: */
				delete *cpIt;
				cuttingPlanes.erase(cpIt);
				break;
				}
		}
	}

void VRVolumeRenderer::frame(void)
	{
	/* Retrieve current viewing direction: */
	viewDirection=renderer->getCenter()-VolumeRenderer::Point(Vrui::getHeadPosition());
	viewDirection.normalize();
	}

void VRVolumeRenderer::display(GLContextData& contextData) const
	{
	/* Enable all cutting planes: */
	for(std::vector<CuttingPlaneLocator*>::const_iterator cpIt=cuttingPlanes.begin();cpIt!=cuttingPlanes.end();++cpIt)
		(*cpIt)->setGLState();
	
	/* Enable alpha test: */
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER,0.0f);
	
	/* Draw the volume: */
	renderer->renderBlock(contextData,viewDirection);
	
	glDisable(GL_ALPHA_TEST);
	
	/* Disable all cutting planes: */
	for(std::vector<CuttingPlaneLocator*>::const_iterator cpIt=cuttingPlanes.begin();cpIt!=cuttingPlanes.end();++cpIt)
		(*cpIt)->resetGLState();
	}

void VRVolumeRenderer::centerDisplayCallback(Misc::CallbackData* cbData)
	{
	/* Calculate initial navigation transformation: */
	Vrui::setNavigationTransformation(Vrui::Point(renderer->getCenter()),Vrui::Scalar(renderer->getRadius()));
	}

void VRVolumeRenderer::colorMapChangedCallback(Misc::CallbackData* cbData)
	{
	/* Export the changed color map to the palette: */
	paletteEditor->exportColorMap(*palette);
	
	/* Set the palette in the volume renderer: */
	palette->changeTransparency(transparencyGamma);
	palette->premultiplyAlpha();
	renderer->setColorMap(palette);
	
	Vrui::requestUpdate();
	}

void VRVolumeRenderer::showPaletteEditorCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
	{
	/* Hide or show palette editor based on toggle button state: */
	if(cbData->set)
		{
		/* Pop up the palette editor at the same position as the main menu: */
		Vrui::getWidgetManager()->popupPrimaryWidget(paletteEditor,Vrui::getWidgetManager()->calcWidgetTransformation(mainMenu));
		}
	else
		Vrui::popdownPrimaryWidget(paletteEditor);
	}

void VRVolumeRenderer::savePaletteCallback(Misc::CallbackData* cbData)
	{
	paletteEditor->savePalette("Palette.pal");
	}

void VRVolumeRenderer::showRenderSettingsDialogCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
	{
	/* Hide or show render settings dialog based on toggle button state: */
	if(cbData->set)
		{
		/* Pop up the render settings dialog at the same position as the main menu: */
		Vrui::getWidgetManager()->popupPrimaryWidget(renderSettingsDialog,Vrui::getWidgetManager()->calcWidgetTransformation(mainMenu));
		}
	else
		Vrui::popdownPrimaryWidget(renderSettingsDialog);
	}

void VRVolumeRenderer::sliderValueChangedCallback(GLMotif::Slider::ValueChangedCallbackData* cbData)
	{
	if(cbData->slider==sliceFactorSlider)
		{
		/* Change the slice factor (and transparency gamma to keep total opacity constant): */
		double newSliceFactor=cbData->value;
		transparencyGamma=float((double(transparencyGamma)*newSliceFactor)/double(sliceFactor));
		sliceFactor=VolumeRenderer::Scalar(cbData->value);
		
		/* Update the slice factor value label: */
		char value[40];
		snprintf(value,sizeof(value),"%4.2lf",cbData->value);
		sliceFactorValue->setLabel(value);
		
		/* Update the transparency gamma value label: */
		snprintf(value,sizeof(value),"%4.2lf",cbData->value);
		transparencyGammaValue->setLabel(value);
		
		/* Update the transparency gamma slider: */
		transparencyGammaSlider->setValue(double(transparencyGamma));
		
		/* Update the volume renderer and color map: */
		renderer->setSliceFactor(sliceFactor);
		colorMapChangedCallback(0);
		}
	else if(cbData->slider==transparencyGammaSlider)
		{
		/* Change the transparency gamma factor: */
		transparencyGamma=GLfloat(cbData->value);
		
		/* Update the transparency gamma value label: */
		char value[40];
		snprintf(value,sizeof(value),"%4.2lf",cbData->value);
		transparencyGammaValue->setLabel(value);
		
		/* Update the color map: */
		colorMapChangedCallback(0);
		}
	}

void VRVolumeRenderer::createInputDeviceCallback(Misc::CallbackData* cbData)
	{
	Vrui::addVirtualInputDevice("Virtual",1,0);
	}

void VRVolumeRenderer::saveViewCallback(Misc::CallbackData* cbData)
	{
	/* Open viewpoint file: */
	Misc::File viewpointFile("Viewpoint.dat","wb",Misc::File::LittleEndian);
	
	/* Get the current navigation transformation: */
	const Vrui::NavTransform& nt=Vrui::getNavigationTransformation();
	
	/* Write the navigation transformation: */
	viewpointFile.write(nt.getTranslation().getComponents(),3);
	viewpointFile.write(nt.getRotation().getQuaternion(),4);
	viewpointFile.write(nt.getScaling());
	}

void VRVolumeRenderer::loadViewCallback(Misc::CallbackData* cbData)
	{
	/* Open viewpoint file: */
	Misc::File viewpointFile("Viewpoint.dat","rb",Misc::File::LittleEndian);
	
	/* Read the navigation transformation: */
	Vrui::NavTransform::Vector translation;
	viewpointFile.read(translation.getComponents(),3);
	Vrui::NavTransform::Rotation::Scalar quaternion[4];
	viewpointFile.read(quaternion,4);
	Vrui::NavTransform::Scalar scaling=viewpointFile.read<Vrui::NavTransform::Scalar>();
	
	/* Set the navigation transformation: */
	Vrui::setNavigationTransformation(Vrui::NavTransform(translation,Vrui::NavTransform::Rotation::fromQuaternion(quaternion),scaling));
	}

int main(int argc,char* argv[])
	{
	try
		{
		char** appDefaults=0;
		VRVolumeRenderer vr(argc,argv,appDefaults);
		vr.run();
		}
	catch(std::runtime_error err)
		{
		fprintf(stderr,"Caught exception %s\n",err.what());
		return 1;
		}
	
	return 0;
	}
