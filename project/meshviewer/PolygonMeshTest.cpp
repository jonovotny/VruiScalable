/***********************************************************************
PolygonMeshTest - Vrui application to test new mesh classes.
Copyright (c) 2009-2012 Oliver Kreylos
***********************************************************************/

#include "PolygonMeshTest.h"

#include <assert.h>
#include <string.h>
#include <iostream>
#include <Misc/FunctionCalls.h>
#include <Misc/ThrowStdErr.h>
#include <Misc/FileNameExtensions.h>
#include <Geometry/Plane.h>
#include <Geometry/LinearUnit.h>
#include <Geometry/OutputOperators.h>
#include <GL/gl.h>
#include <GL/GLVertexTemplates.h>
#include <GL/GLMaterial.h>
#include <GL/GLTransformationWrappers.h>
#include <GLMotif/WidgetManager.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/Menu.h>
#include <GLMotif/PopupMenu.h>
#include <GLMotif/Margin.h>
#include <GLMotif/Label.h>
#include <GLMotif/Button.h>
#include <GLMotif/TextField.h>
#include <Vrui/Vrui.h>
#include <Vrui/InputDevice.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/CoordinateManager.h>
#include <Vrui/Viewer.h>
#include <Vrui/ToolManager.h>
#include <Vrui/DisplayState.h>

#include "MaterialManager.h"
#include "PolygonModel.h"
#include "HierarchicalTriangleSetBase.h"
#include "MultiModel.h"
#include "ReadPLYFile.h"
#include "ReadLWOFile.h"
#include "ReadLWSFile.h"
#include "ReadASEFile.h"
#include "ReadOBJFile.h"

/********************************************************
Static elements of class PolygonMeshTest::ModelProbeTool:
********************************************************/

PolygonMeshTest::ModelProbeToolFactory* PolygonMeshTest::ModelProbeTool::factory=0;

/************************************************
Methods of class PolygonMeshTest::ModelProbeTool:
************************************************/

PolygonMeshTest::ModelProbeTool::ModelProbeTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment)
	:Vrui::Tool(factory,inputAssignment),
	 active(false),
	 haveIntersection(false)
	{
	}

void PolygonMeshTest::ModelProbeTool::buttonCallback(int,Vrui::InputDevice::ButtonCallbackData* cbData)
	{
	active=cbData->newButtonState;
	
	if(active)
		{
		/* Get pointer to input device: */
		Vrui::InputDevice* iDevice=getButtonDevice(0);
		
		/* Calculate ray equation in navigation coordinates: */
		Vrui::Point p0=iDevice->getPosition();
		Vrui::Vector dir=iDevice->getRayDirection();
		Vrui::Point p1=p0+dir*Vrui::getBackplaneDist();
		PolygonModel::Point mp0=Vrui::getInverseNavigationTransformation().transform(p0);
		PolygonModel::Point mp1=Vrui::getInverseNavigationTransformation().transform(p1);
		
		/* Check if the model is a hierarchical triangle set or contains one: */
		const HierarchicalTriangleSetBase* hts=dynamic_cast<HierarchicalTriangleSetBase*>(application->model);
		if(hts==0)
			{
			MultiModel* mm=dynamic_cast<MultiModel*>(application->model);
			if(mm!=0)
				hts=mm->getHierarchicalTriangleSet();
			}
		if(hts!=0)
			{
			/* Find the submesh intersected by the ray: */
			std::cout<<"Searching submesh"<<std::endl;
			application->subMesh=hts->findSubMesh(mp0,mp1);
			}
		else
			application->subMesh=0;
		
		/* Update the application's submesh data dialog: */
		application->updateSubMeshDialog();
		}
	}

void PolygonMeshTest::ModelProbeTool::frame(void)
	{
	if(active)
		{
		/* Get pointer to input device: */
		Vrui::InputDevice* iDevice=getButtonDevice(0);
		
		/* Calculate ray equation in navigation coordinates: */
		Vrui::Point p0=iDevice->getPosition();
		Vrui::Vector dir=iDevice->getRayDirection();
		Vrui::Point p1=p0+dir*Vrui::getBackplaneDist();
		PolygonModel::Point mp0=Vrui::getInverseNavigationTransformation().transform(p0);
		PolygonModel::Point mp1=Vrui::getInverseNavigationTransformation().transform(p1);
		
		/* Probe the shit out of the MODEL!: */
		PolygonModel::Point mp=application->model->intersect(mp0,mp1);
		if(mp!=mp1)
			{
			haveIntersection=true;
			intersection=mp;
			}
		else
			haveIntersection=false;
		}
	}

void PolygonMeshTest::ModelProbeTool::display(GLContextData& contextData) const
	{
	const Vrui::DisplayState& displayState=Vrui::getDisplayState(contextData);
	
	if(haveIntersection)
		{
		glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
		glDisable(GL_LIGHTING);
		glLineWidth(3.0f);
		
		/* Go to navigational coordinates: */
		glPushMatrix();
		glLoadIdentity();
		glMultMatrix(displayState.modelviewNavigational);
		
		Vrui::Scalar s=Vrui::getInverseNavigationTransformation().getScaling()*Vrui::getUiSize()*Vrui::Scalar(4);
		glColor3f(1.0f,0.0f,1.0f);
		glBegin(GL_LINES);
		glVertex(intersection[0]-s,intersection[1],intersection[2]);
		glVertex(intersection[0]+s,intersection[1],intersection[2]);
		glVertex(intersection[0],intersection[1]-s,intersection[2]);
		glVertex(intersection[0],intersection[1]+s,intersection[2]);
		glVertex(intersection[0],intersection[1],intersection[2]-s);
		glVertex(intersection[0],intersection[1],intersection[2]+s);
		glEnd();
		
		glPopMatrix();
		glPopAttrib();
		}
	}

/************************************************************
Static elements of class PolygonMeshTest::ModelProjectorTool:
************************************************************/

PolygonMeshTest::ModelProjectorToolFactory* PolygonMeshTest::ModelProjectorTool::factory=0;

/****************************************************
Methods of class PolygonMeshTest::ModelProjectorTool:
****************************************************/

PolygonMeshTest::ModelProjectorTool::ModelProjectorTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment)
	:Vrui::TransformTool(factory,inputAssignment)
	{
	/* Set the source input device: */
	sourceDevice=getButtonDevice(0);
	}

void PolygonMeshTest::ModelProjectorTool::initialize(void)
	{
	/* Initialize the base tool: */
	TransformTool::initialize();
	
	/* Disable the transformed device's glyph: */
	Vrui::getInputGraphManager()->getInputDeviceGlyph(transformedDevice).disable();
	}

void PolygonMeshTest::ModelProjectorTool::frame(void)
	{
	/* Calculate ray equation in navigation coordinates: */
	Vrui::Point p0=sourceDevice->getPosition();
	Vrui::Vector dir=sourceDevice->getRayDirection();
	Vrui::Point p1=p0+dir*Vrui::getBackplaneDist();
	PolygonModel::Point mp0=Vrui::getInverseNavigationTransformation().transform(p0);
	PolygonModel::Point mp1=Vrui::getInverseNavigationTransformation().transform(p1);
	
	/* Probe the shit out of the MODEL!: */
	PolygonModel::Point mp=application->model->intersect(mp0,mp1);
	if(mp!=mp1)
		{
		/* Set the device position to the intersection point: */
		Vrui::TrackerState ts=Vrui::TrackerState::translateFromOriginTo(Vrui::getNavigationTransformation().transform(mp));
		transformedDevice->setTransformation(ts);
		}
	else
		{
		/* Move the device in the plane it currently inhabits: */
		Vrui::Scalar lambda=(dir*(transformedDevice->getPosition()-p0))/Geometry::sqr(dir);
		Vrui::TrackerState ts=Vrui::TrackerState::translateFromOriginTo(p0+dir*lambda);
		transformedDevice->setTransformation(ts);
		}
	transformedDevice->setDeviceRay(dir,0);
	}

/************************************************
Methods of class PolygonMeshTest::AlignmentState:
************************************************/

void PolygonMeshTest::AlignmentState::set(PolygonModel::Scalar newPlayerHeight,const PolygonModel::Point& newPlayerFoot,PolygonModel::Scalar playerRadius)
	{
	/* Copy the parameters: */
	playerHeight=newPlayerHeight;
	playerFoot=newPlayerFoot;
	
	/* Initialize the player box in z-is-up model coordinates: */
	playerBox.min=playerBox.max=playerFoot;
	for(int i=0;i<2;++i)
		{
		playerBox.min[i]-=playerRadius;
		playerBox.max[i]+=playerRadius;
		}
	playerBox.max[2]+=playerHeight;
	}

namespace {

/****************
Helper functions:
****************/

inline float nudgePlus(float value)
	{
	union
		{
		float f;
		int i;
		} fi;
	fi.f=value;
	if(value>=0.0f)
		++fi.i;
	else
		--fi.i;
	return fi.f;
	}

inline double nudgePlus(double value)
	{
	union
		{
		double f;
		long int i;
		} fi;
	fi.f=value;
	if(value>=0.0)
		++fi.i;
	else
		--fi.i;
	return fi.f;
	}

inline float nudgeMinus(float value)
	{
	union
		{
		float f;
		int i;
		} fi;
	fi.f=value;
	if(value>0.0f)
		--fi.i;
	else
		++fi.i;
	return fi.f;
	}

inline double nudgeMinus(double value)
	{
	union
		{
		double f;
		long int i;
		} fi;
	fi.f=value;
	if(value>0.0)
		--fi.i;
	else
		++fi.i;
	return fi.f;
	}

template <class ScalarParam,int dimensionParam>
inline
Geometry::Point<ScalarParam,dimensionParam>
nudgePoint(
	const Geometry::Point<ScalarParam,dimensionParam>& point,
	const Geometry::Vector<ScalarParam,dimensionParam>& direction)
	{
	Geometry::Point<ScalarParam,dimensionParam> result;
	for(int i=0;i<dimensionParam;++i)
		{
		if(direction[i]>ScalarParam(0))
			result[i]=nudgePlus(point[i]);
		if(direction[i]<ScalarParam(0))
			result[i]=nudgeMinus(point[i]);
		}
	
	return result;
	}

}

/********************************
Methods of class PolygonMeshTest:
********************************/

GLMotif::PopupMenu* PolygonMeshTest::createMainMenu(void)
	{
	/* Create a popup shell to hold the main menu: */
	GLMotif::PopupMenu* mainMenuPopup=new GLMotif::PopupMenu("MainMenuPopup",Vrui::getWidgetManager());
	mainMenuPopup->setTitle("Polygon Mesh Viewer");
	
	/* Create the main menu itself: */
	GLMotif::Menu* mainMenu=new GLMotif::Menu("MainMenu",mainMenuPopup,false);
	
	GLMotif::ToggleButton* showBackfacesButton=new GLMotif::ToggleButton("ShowBackfacesButton",mainMenu,"Show Backfaces");
	showBackfacesButton->setToggle(showBackfaces);
	showBackfacesButton->getValueChangedCallbacks().add(this,&PolygonMeshTest::showBackfacesCallback);
	
	GLMotif::Button* resetNavigationButton=new GLMotif::Button("ResetNavigationButton",mainMenu,"Reset Navigation");
	resetNavigationButton->getSelectCallbacks().add(this,&PolygonMeshTest::resetNavigationCallback);
	
	GLMotif::Button* levelModelButton=new GLMotif::Button("LevelModelButton",mainMenu,"Level Model");
	levelModelButton->getSelectCallbacks().add(this,&PolygonMeshTest::levelModelCallback);
	
	GLMotif::Button* scaleOneToOneButton=new GLMotif::Button("ScaleOneToOneButton",mainMenu,"Scale 1:1");
	scaleOneToOneButton->getSelectCallbacks().add(this,&PolygonMeshTest::scaleOneToOneCallback);
	
	/* Finish building the main menu: */
	mainMenu->manageChild();
	
	return mainMenuPopup;
	}

GLMotif::PopupWindow* PolygonMeshTest::createSubMeshDialog(void)
	{
	/* Create the submesh dialog window: */
	GLMotif::PopupWindow* subMeshDialogPopup=new GLMotif::PopupWindow("SubMeshDialogPopup",Vrui::getWidgetManager(),"Submesh Data");
	subMeshDialogPopup->setResizableFlags(true,false);
	subMeshDialogPopup->setCloseButton(true);
	subMeshDialogPopup->getCloseCallbacks().add(this,&PolygonMeshTest::subMeshDialogCloseCallback);
	
	GLMotif::RowColumn* data=new GLMotif::RowColumn("Data",subMeshDialogPopup,false);
	data->setOrientation(GLMotif::RowColumn::VERTICAL);
	data->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	data->setNumMinorWidgets(2);
	
	new GLMotif::Label("NameLabel",data,"Name");
	
	nameField=new GLMotif::TextField("NameField",data,40);
	nameField->setHAlignment(GLFont::Left);
	
	new GLMotif::Label("NumTrianglesLabel",data,"Num Tris");
	
	GLMotif::Margin* numTrianglesMargin=new GLMotif::Margin("NumTrianglesMargin",data,false);
	numTrianglesMargin->setAlignment(GLMotif::Alignment(GLMotif::Alignment::LEFT));
	
	numTrianglesField=new GLMotif::TextField("NumTrianglesField",numTrianglesMargin,10);
	
	numTrianglesMargin->manageChild();
	
	new GLMotif::Label("BboxLabel",data,"Box");
	
	GLMotif::RowColumn* bbox=new GLMotif::RowColumn("Bbox",data,false);
	bbox->setOrientation(GLMotif::RowColumn::VERTICAL);
	bbox->setPacking(GLMotif::RowColumn::PACK_GRID);
	bbox->setNumMinorWidgets(3);
	
	for(int i=0;i<6;++i)
		{
		bboxField[i]=new GLMotif::TextField("BboxField",bbox,10);
		bboxField[i]->setFloatFormat(GLMotif::TextField::SMART);
		bboxField[i]->setPrecision(9);
		}
	
	bbox->manageChild();
	
	new GLMotif::Label("BboxCenterLabel",data,"Center");
	
	GLMotif::RowColumn* bboxCenter=new GLMotif::RowColumn("BboxCenter",data,false);
	bboxCenter->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	bboxCenter->setPacking(GLMotif::RowColumn::PACK_GRID);
	bboxCenter->setNumMinorWidgets(1);
	
	for(int i=0;i<3;++i)
		{
		bboxCenterField[i]=new GLMotif::TextField("BboxCenterField",bboxCenter,10);
		bboxCenterField[i]->setFloatFormat(GLMotif::TextField::SMART);
		bboxCenterField[i]->setPrecision(9);
		}
	
	bboxCenter->manageChild();
	
	data->manageChild();
	
	return subMeshDialogPopup;
	}

PolygonMeshTest::PolygonMeshTest(int& argc,char**& argv,char**& appDefaults)
	:Vrui::Application(argc,argv,appDefaults),
	 materialManager(0),
	 model(0),
	 upVector(0,0,1),
	 showBackfaces(false),
	 subMesh(0),
	 mainMenu(0),subMeshDialog(0)
	{
	/* Register the custom tool classes with the Vrui tool manager: */
	ModelProbeToolFactory* toolFactory1=new ModelProbeToolFactory("ModelProbeTool","Model Probe",0,*Vrui::getToolManager());
	toolFactory1->setNumButtons(1);
	toolFactory1->setButtonFunction(0,"Probe Model");
	Vrui::getToolManager()->addClass(toolFactory1,Vrui::ToolManager::defaultToolFactoryDestructor);
	ModelProjectorToolFactory* toolFactory2=new ModelProjectorToolFactory("ModelProjectorTool","Model Projector",Vrui::getToolManager()->loadClass("TransformTool"),*Vrui::getToolManager());
	toolFactory2->setNumButtons(0,true);
	toolFactory2->setButtonFunction(0,"Forwarded Button");
	toolFactory2->setNumValuators(0,true);
	toolFactory2->setValuatorFunction(0,"Forwarded Valuator");
	Vrui::getToolManager()->addClass(toolFactory2,Vrui::ToolManager::defaultToolFactoryDestructor);
	
	/* Parse the command line: */
	const char* imagePrefix="";
	const char* imageReplace="";
	std::vector<const char*> modelFileNames;
	const char* bspTreeFileName=0;
	Geometry::LinearUnit linearUnit;
	for(int i=1;i<argc;++i)
		{
		if(argv[i][0]=='-')
			{
			if(strcasecmp(argv[i]+1,"prefix")==0)
				{
				++i;
				imagePrefix=argv[i];
				}
			else if(strcasecmp(argv[i]+1,"replace")==0)
				{
				++i;
				imageReplace=argv[i];
				}
			else if(strcasecmp(argv[i]+1,"bsp")==0)
				{
				++i;
				bspTreeFileName=argv[i];
				}
			else if(strcasecmp(argv[i]+1,"up")==0)
				{
				for(int j=0;j<3;++j)
					{
					++i;
					upVector[j]=Vrui::Scalar(atof(argv[i]));
					}
				upVector.normalize();
				}
			else if(strcasecmp(argv[i]+1,"unit")==0)
				{
				/* Override the default linear unit from the command line: */
				++i;
				double unitFactor=atof(argv[i]);
				++i;
				linearUnit=Geometry::LinearUnit(argv[i],unitFactor);
				}
			}
		else
			modelFileNames.push_back(argv[i]);
		}
	if(modelFileNames.empty())
		Misc::throwStdErr("PolygonMeshTest::PolygonMeshTest: No model file name given");
	
	/* Create a material manager: */
	materialManager=new MaterialManager(imagePrefix,imageReplace);
	
	/* Load all model files: */
	MultiModel* mm=0;
	std::vector<const char*> objModelFileNames;
	for(std::vector<const char*>::iterator mfnIt=modelFileNames.begin();mfnIt!=modelFileNames.end();++mfnIt)
		{
		/* Find the model file name's end: */
		const char* endPtr;
		for(endPtr=*mfnIt;*endPtr!='\0';++endPtr)
			;
		
		/* Check for and remove a .gz file name extension: */
		if(endPtr-*mfnIt>=3&&strcasecmp(endPtr-3,".gz")==0)
			endPtr-=3;
		
		/* Find the model file name's real extension: */
		const char* extPtr;
		for(extPtr=endPtr;extPtr>=*mfnIt&&*extPtr!='.';--extPtr)
			;
		PolygonModel* part=0;
		if(extPtr>=*mfnIt&&strncasecmp(extPtr,".ply",endPtr-extPtr)==0)
			part=readPLYFile(*mfnIt,Vrui::getClusterMultiplexer());
		else if(extPtr>=*mfnIt&&strncasecmp(extPtr,".lwo",endPtr-extPtr)==0)
			part=readLWOFile(*mfnIt,*materialManager,Vrui::getClusterMultiplexer());
		else if(extPtr>=*mfnIt&&strncasecmp(extPtr,".lws",endPtr-extPtr)==0)
			part=readLWSFile(*mfnIt,*materialManager,Vrui::getClusterMultiplexer());
		else if(extPtr>=*mfnIt&&strncasecmp(extPtr,".ase",endPtr-extPtr)==0)
			part=readASEFile(*mfnIt,Vrui::getClusterMultiplexer());
		else if(extPtr>=*mfnIt&&strncasecmp(extPtr,".obj",endPtr-extPtr)==0)
			{
			/* Save the model for later: */
			objModelFileNames.push_back(*mfnIt);
			}
		else
			Misc::throwStdErr("PolygonMeshTest::PolygonMeshTest: Unrecognized extension in input file %s",*mfnIt);
		
		if(part!=0)
			{
			if(mm!=0)
				mm->addPart(part);
			else if(model!=0)
				{
				mm=new MultiModel;
				mm->addPart(model);
				model=mm;
				}
			else
				model=part;
			}
		}
	
	/* Read all OBJ model files: */
	PolygonModel* part=readOBJFiles(objModelFileNames,*materialManager,Vrui::getClusterMultiplexer());
	if(part!=0)
		{
		if(mm!=0)
			mm->addPart(part);
		else if(model!=0)
			{
			mm=new MultiModel;
			mm->addPart(model);
			model=mm;
			}
		else
			model=part;
		}
	
	if(bspTreeFileName!=0)
		{
		/* Load a BSP tree: */
		std::cout<<"Loading BSP tree from "<<bspTreeFileName<<std::endl;
		model->loadBSPTree(bspTreeFileName);
		}
	
	/* Print the model's bounding box: */
	PolygonModel::Box bbox=model->calcBoundingBox();
	std::cout<<"Model bounding box: "<<bbox.min[0]<<" "<<bbox.min[1]<<" "<<bbox.min[2]<<" "<<bbox.max[0]<<" "<<bbox.max[1]<<" "<<bbox.max[2]<<std::endl;
	
	/* Calculate an epsilon value for collision detection: */
	PolygonModel::Scalar maxCoordinate=PolygonModel::Scalar(0);
	for(int i=0;i<3;++i)
		{
		if(maxCoordinate<Math::abs(bbox.min[i]))
			maxCoordinate=Math::abs(bbox.min[i]);
		if(maxCoordinate<Math::abs(bbox.max[i]))
			maxCoordinate=Math::abs(bbox.max[i]);
		}
	epsilon=nudgePlus(float(maxCoordinate))-float(maxCoordinate);
	std::cout<<"Collision detection epsilon is "<<epsilon<<std::endl;
	
	/* Create the program's main menu: */
	mainMenu=createMainMenu();
	Vrui::setMainMenu(mainMenu);
	
	/* Set the linear unit: */
	Vrui::getCoordinateManager()->setUnit(linearUnit);
	
	/* Initialize the navigation transformation: */
	resetNavigationCallback(0);
	}

PolygonMeshTest::~PolygonMeshTest(void)
	{
	delete mainMenu;
	delete subMeshDialog;
	delete model;
	delete materialManager;
	}

void PolygonMeshTest::toolCreationCallback(Vrui::ToolManager::ToolCreationCallbackData* cbData)
	{
	/* Call the base class method: */
	Vrui::Application::toolCreationCallback(cbData);
	
	/* Check if the new tool is a surface navigation tool: */
	Vrui::SurfaceNavigationTool* surfaceNavigationTool=dynamic_cast<Vrui::SurfaceNavigationTool*>(cbData->tool);
	if(surfaceNavigationTool!=0)
		{
		/* Set the new tool's alignment function: */
		surfaceNavigationTool->setAlignFunction(Misc::createFunctionCall(this,&PolygonMeshTest::alignSurfaceFrame));
		}
	}

void PolygonMeshTest::frame(void)
	{
	#if 0
	
	/* Trace the test box from its current position to the center of the display: */
	PolygonModel::Box testBox;
	// testBoxCenter=PolygonModel::Point(6.75,1.25,-8.26500034+0.5);
	for(int i=0;i<3;++i)
		{
		testBox.min[i]=testBoxCenter[i]-PolygonModel::Scalar(0.5);
		testBox.max[i]=testBoxCenter[i]+PolygonModel::Scalar(0.5);
		}
	
	PolygonModel::Vector displacement=PolygonModel::Point(Vrui::getInverseNavigationTransformation().transform(Vrui::getDisplayCenter()))-testBoxCenter;
	// displacement=PolygonModel::Vector(0.0,0.0,-1.0);
	PolygonModel::Vector hitNormal;
	PolygonModel::Scalar lambda=model->traceBox(testBox,displacement,hitNormal);
	
	if(lambda<PolygonModel::Scalar(1))
		{
		lambda-=PolygonModel::Scalar(0.001)/displacement.mag();
		if(lambda<PolygonModel::Scalar(0.0))
			lambda=PolygonModel::Scalar(0.0);
		testBoxCenter+=displacement*lambda;
		
		/* Let it slide: */
		for(int i=0;i<3;++i)
			{
			testBox.min[i]=testBoxCenter[i]-PolygonModel::Scalar(0.5);
			testBox.max[i]=testBoxCenter[i]+PolygonModel::Scalar(0.5);
			}
		displacement*=(PolygonModel::Scalar(1)-lambda);
		displacement.orthogonalize(hitNormal);
		lambda=model->traceBox(testBox,displacement,hitNormal);
		
		if(lambda<PolygonModel::Scalar(1))
			{
			lambda-=PolygonModel::Scalar(0.001)/displacement.mag();
			if(lambda<PolygonModel::Scalar(0.0))
				lambda=PolygonModel::Scalar(0.0);
			testBoxCenter+=displacement*lambda;
			}
		else
			testBoxCenter+=displacement;
		}
	else
		testBoxCenter+=displacement;
	
	std::cout<<testBoxCenter<<std::endl;
	
	#endif
	}

void PolygonMeshTest::display(GLContextData& contextData) const
	{
	glPushAttrib(GL_ENABLE_BIT|GL_LIGHTING_BIT|GL_LINE_BIT|GL_POLYGON_BIT);
	glDisable(GL_COLOR_MATERIAL);
	glMaterial(GLMaterialEnums::FRONT_AND_BACK,GLMaterial(GLMaterial::Color(0.6f,0.6f,0.6f),GLMaterial::Color(0.5f,0.5f,0.5f),25.0f));
	if(showBackfaces)
		{
		/* Disable backface culling and enable two-sided lighting: */
		glDisable(GL_CULL_FACE);
		glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_TRUE);
		}
	model->glRenderAction(contextData);
	
	if(subMesh!=0)
		{
		/* Check if the model is a hierarchical triangle set or contains one: */
		const HierarchicalTriangleSetBase* hts=dynamic_cast<const HierarchicalTriangleSetBase*>(model);
		if(hts==0)
			{
			const MultiModel* mm=dynamic_cast<MultiModel*>(model);
			if(mm!=0)
				hts=mm->getHierarchicalTriangleSet();
			}
		
		glMaterial(GLMaterialEnums::FRONT_AND_BACK,GLMaterial(GLMaterial::Color(1.0f,0.5f,0.5f)));
		hts->drawSubMesh(*subMesh,contextData);
		}
	
	#if 0
	
	glDisable(GL_LIGHTING);
	glLineWidth(1.0f);
	glColor3f(0.0f,1.0f,0.0f);
	glBegin(GL_LINE_STRIP);
	glVertex3f(testBoxCenter[0]-0.5f,testBoxCenter[1]-0.5f,testBoxCenter[2]-0.5f);
	glVertex3f(testBoxCenter[0]+0.5f,testBoxCenter[1]-0.5f,testBoxCenter[2]-0.5f);
	glVertex3f(testBoxCenter[0]+0.5f,testBoxCenter[1]+0.5f,testBoxCenter[2]-0.5f);
	glVertex3f(testBoxCenter[0]-0.5f,testBoxCenter[1]+0.5f,testBoxCenter[2]-0.5f);
	glVertex3f(testBoxCenter[0]-0.5f,testBoxCenter[1]-0.5f,testBoxCenter[2]-0.5f);
	glVertex3f(testBoxCenter[0]-0.5f,testBoxCenter[1]-0.5f,testBoxCenter[2]+0.5f);
	glVertex3f(testBoxCenter[0]+0.5f,testBoxCenter[1]-0.5f,testBoxCenter[2]+0.5f);
	glVertex3f(testBoxCenter[0]+0.5f,testBoxCenter[1]+0.5f,testBoxCenter[2]+0.5f);
	glVertex3f(testBoxCenter[0]-0.5f,testBoxCenter[1]+0.5f,testBoxCenter[2]+0.5f);
	glVertex3f(testBoxCenter[0]-0.5f,testBoxCenter[1]-0.5f,testBoxCenter[2]+0.5f);
	glEnd();
	glBegin(GL_LINES);
	glVertex3f(testBoxCenter[0]+0.5f,testBoxCenter[1]-0.5f,testBoxCenter[2]-0.5f);
	glVertex3f(testBoxCenter[0]+0.5f,testBoxCenter[1]-0.5f,testBoxCenter[2]+0.5f);
	glVertex3f(testBoxCenter[0]+0.5f,testBoxCenter[1]+0.5f,testBoxCenter[2]-0.5f);
	glVertex3f(testBoxCenter[0]+0.5f,testBoxCenter[1]+0.5f,testBoxCenter[2]+0.5f);
	glVertex3f(testBoxCenter[0]-0.5f,testBoxCenter[1]+0.5f,testBoxCenter[2]-0.5f);
	glVertex3f(testBoxCenter[0]-0.5f,testBoxCenter[1]+0.5f,testBoxCenter[2]+0.5f);
	glEnd();
	
	#endif
	
	glPopAttrib();
	}

void PolygonMeshTest::resetNavigationCallback(Misc::CallbackData* cbData)
	{
	/* Initialize the navigation transformation: */
	PolygonModel::Box bbox=model->calcBoundingBox();
	Vrui::setNavigationTransformation(Geometry::mid(bbox.min,bbox.max),Geometry::dist(bbox.min,bbox.max),upVector);
	}

void PolygonMeshTest::showBackfacesCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
	{
	showBackfaces=cbData->set;
	}

void PolygonMeshTest::levelModelCallback(Misc::CallbackData* cbData)
	{
	/* Rotate such that the up vector and Vrui's up direction are aligned: */
	Vrui::NavTransform nav=Vrui::getNavigationTransformation();
	Vrui::Vector physUp=nav.transform(upVector);
	nav.leftMultiply(Vrui::NavTransform::translateFromOriginTo(Vrui::getDisplayCenter()));
	nav.leftMultiply(Vrui::NavTransform::rotate(Vrui::Rotation::rotateFromTo(physUp,Vrui::getUpDirection())));
	nav.leftMultiply(Vrui::NavTransform::translateToOriginFrom(Vrui::getDisplayCenter()));
	Vrui::setNavigationTransformation(nav);
	}

void PolygonMeshTest::scaleOneToOneCallback(Misc::CallbackData* cbData)
	{
	/* Calculate the correct scaling factor: */
	Vrui::Scalar newNavScale;
	const Geometry::LinearUnit& unit=Vrui::getCoordinateManager()->getUnit();
	if(unit.isImperial())
		{
		/* Calculate scale factor through imperial units: */
		newNavScale=Vrui::getInchFactor()/unit.getInchFactor();
		}
	else
		{
		/* Calculate scale factor through metric units: */
		newNavScale=Vrui::getMeterFactor()/unit.getMeterFactor();
		}
	
	/* Pre-multiply the navigation transformation with a scaling around the current display center: */
	Vrui::NavTransform nav=Vrui::getNavigationTransformation();
	nav.leftMultiply(Vrui::NavTransform::scaleAround(Vrui::getDisplayCenter(),newNavScale/nav.getScaling()));
	
	/* Set the new navigation transformation: */
	Vrui::setNavigationTransformation(nav);
	}

void PolygonMeshTest::subMeshDialogCloseCallback(Misc::CallbackData* cbData)
	{
	/* Delete the submesh data dialog: */
	Vrui::getWidgetManager()->deleteWidget(subMeshDialog);
	subMeshDialog=0;
	}

void PolygonMeshTest::updateSubMeshDialog(void)
	{
	if(subMesh!=0)
		{
		/* Create and open the submesh data dialog if it doesn't exist: */
		if(subMeshDialog==0)
			{
			subMeshDialog=createSubMeshDialog();
			Vrui::popupPrimaryWidget(subMeshDialog);
			}

		/* Update the submesh data: */
		nameField->setString(subMesh->getName().c_str());
		numTrianglesField->setValue((unsigned int)(subMesh->getNumTriangles()));
		PolygonModel::Box bbox=subMesh->getBoundingBox();
		for(int i=0;i<3;++i)
			{
			bboxField[0+i]->setValue(bbox.min[i]);
			bboxField[3+i]->setValue(bbox.max[i]);
			}
		for(int i=0;i<3;++i)
			bboxCenterField[i]->setValue(Math::mid(bbox.min[i],bbox.max[i]));
		}
	else if(subMeshDialog!=0)
		{
		/* Reset the submesh data dialog: */
		nameField->setString("");
		numTrianglesField->setString("");
		for(int i=0;i<6;++i)
			bboxField[i]->setString("");
		}
	}

void PolygonMeshTest::alignSurfaceFrame(Vrui::SurfaceNavigationTool::AlignmentData& alignmentData)
	{
	typedef PolygonModel::Scalar Scalar;
	typedef PolygonModel::Point Point;
	typedef PolygonModel::Vector Vector;
	typedef PolygonModel::Box Box;
	
	/* Calculate the player's current head height: */
	Vrui::Point headPosPhys=Vrui::getMainViewer()->getHeadPosition();
	const Vrui::Vector& floorNormal=Vrui::getFloorPlane().getNormal();
	Vrui::Scalar floorLambda=(Vrui::getFloorPlane().getOffset()-headPosPhys*floorNormal)/(Vrui::getUpDirection()*floorNormal);
	Vrui::Point footPosPhys=headPosPhys+Vrui::getUpDirection()*floorLambda;
	Vrui::Scalar playerHeightPhys=Geometry::dist(headPosPhys,footPosPhys);
	Scalar playerHeight=Scalar(playerHeightPhys*alignmentData.surfaceFrame.getScaling());
	
	/* Extract the new surface frame's base point: */
	Vrui::Point base=alignmentData.surfaceFrame.getOrigin();
	Point foot(base);
	
	/* Create a local coordinate frame to account for upVector != (0,0,1): */
	Vrui::Rotation localFrame=Vrui::Rotation::rotateFromTo(Vrui::Vector(0,0,1),upVector);
	
	/* Initialize the alignment state if it doesn't already exist: */
	AlignmentState* as=static_cast<AlignmentState*>(alignmentData.alignmentState);
	if(as==0)
		{
		/* Create a new alignment state object: */
		alignmentData.alignmentState=as=new AlignmentState;
		as->set(playerHeight,foot,alignmentData.probeSize);
		}
	
	/* Raise the player's foot and transform the player box to model coordinates: */
	Box player=as->playerBox;
	player.min[2]+=alignmentData.maxClimb;
	player.transform(Vrui::ONTransform::rotateAround(Vrui::Point(as->playerFoot),localFrame));
	
	/* Trace the player box from its old to its new position: */
	Vector displacement=foot-as->playerFoot;
	Vector hitNormal;
	Scalar lambda=model->traceBox(player,displacement,hitNormal);
	Point newFoot=as->playerFoot+displacement*lambda;
	
	/* Check if the box hit something: */
	if(lambda<Scalar(1))
		{
		#if 1
		
		/* Move the box to its intermediate position: */
		as->set(playerHeight,newFoot,alignmentData.probeSize);
		
		/* Raise the player's foot and transform the player box to model coordinates: */
		player=as->playerBox;
		player.min[2]+=alignmentData.maxClimb;
		player.transform(Vrui::ONTransform::rotateAround(Vrui::Point(as->playerFoot),localFrame));
		
		/* Slide along the hit plane towards the intended end position: */
		Vector slideDisplacement=displacement*(Scalar(1)-lambda);
		slideDisplacement.orthogonalize(hitNormal);
		if(displacement*hitNormal>Scalar(0))
			slideDisplacement-=hitNormal*(epsilon/hitNormal.mag());
		else
			slideDisplacement+=hitNormal*(epsilon/hitNormal.mag());
		lambda=model->traceBox(player,slideDisplacement,hitNormal);
		newFoot=as->playerFoot+slideDisplacement*lambda;
		
		#endif
		}
	
	/* Move the box to its end position: */
	as->set(playerHeight,newFoot,alignmentData.probeSize);
	
	#if 1
	
	/* Drop the player's foot back to the floor: */
	player=as->playerBox;
	player.min[2]+=alignmentData.maxClimb;
	player.transform(Vrui::ONTransform::rotateAround(Vrui::Point(as->playerFoot),localFrame));
	displacement=upVector*(-alignmentData.maxClimb*Scalar(4));
	lambda=model->traceBox(player,displacement,hitNormal);
	
	if(lambda<Scalar(1))
		{
		/* The player stepped on something; raise the head instead: */
		// ...
		}
	
	/* Move the box to its final position: */
	as->set(playerHeight,as->playerFoot+upVector*alignmentData.maxClimb+displacement*lambda,alignmentData.probeSize);
	
	#endif
	
	/* Update the passed frame: */
	alignmentData.surfaceFrame=Vrui::NavTransform(Vrui::Point(as->playerFoot)-Vrui::Point::origin,localFrame,alignmentData.surfaceFrame.getScaling());
	}

/*************
Main function:
*************/

int main(int argc,char* argv[])
	{
	char** appDefaults=0;
	PolygonMeshTest pmt(argc,argv,appDefaults);
	pmt.run();
	
	return 0;
	}
