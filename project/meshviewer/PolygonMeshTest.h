/***********************************************************************
PolygonMeshTest - Vrui application to test new mesh classes.
Copyright (c) 2009-2012 Oliver Kreylos
***********************************************************************/

#ifndef POLYGONMESHTEST_INCLUDED
#define POLYGONMESHTEST_INCLUDED

#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/OrthogonalTransformation.h>
#include <GLMotif/ToggleButton.h>
#include <Vrui/Geometry.h>
#include <Vrui/Tool.h>
#include <Vrui/GenericToolFactory.h>
#include <Vrui/SurfaceNavigationTool.h>
#include <Vrui/TransformTool.h>
#include <Vrui/Application.h>

#include "PolygonModel.h"
#include "HierarchicalTriangleSetBase.h"

/* Forward declarations: */
namespace GLMotif {
class PopupMenu;
class PopupWindow;
class TextField;
}
class MaterialManager;

class PolygonMeshTest:public Vrui::Application
	{
	/* Embedded classes: */
	private:
	class ModelProbeTool;
	typedef Vrui::GenericToolFactory<ModelProbeTool> ModelProbeToolFactory; // Tool class uses the generic factory class
	
	class ModelProbeTool:public Vrui::Tool,public Vrui::Application::Tool<PolygonMeshTest>
		{
		friend class Vrui::GenericToolFactory<ModelProbeTool>;
		
		/* Elements: */
		private:
		static ModelProbeToolFactory* factory; // Pointer to the factory object for this class
		
		bool active;
		bool haveIntersection;
		Vrui::Point intersection;
		
		/* Constructors and destructors: */
		public:
		ModelProbeTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment);
		
		/* Methods from Vrui::Tool: */
		virtual const Vrui::ToolFactory* getFactory(void) const
			{
			return factory;
			}
		virtual void buttonCallback(int buttonSlotIndex,Vrui::InputDevice::ButtonCallbackData* cbData);
		virtual void frame(void);
		virtual void display(GLContextData& contextData) const;
		};
	
	class ModelProjectorTool;
	typedef Vrui::GenericToolFactory<ModelProjectorTool> ModelProjectorToolFactory; // Tool class uses the generic factory class
	
	class ModelProjectorTool:public Vrui::TransformTool,public Vrui::Application::Tool<PolygonMeshTest>
		{
		friend class Vrui::GenericToolFactory<ModelProjectorTool>;
		
		/* Elements: */
		private:
		static ModelProjectorToolFactory* factory; // Pointer to the factory object for this class
		
		/* Constructors and destructors: */
		public:
		ModelProjectorTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment);
		
		/* Methods from Vrui::Tool: */
		virtual void initialize(void);
		virtual const Vrui::ToolFactory* getFactory(void) const
			{
			return factory;
			}
		virtual void frame(void);
		};
	
	class AlignmentState:public Vrui::SurfaceNavigationTool::AlignmentState // Application data to be retained between alignSurfaceFrame calls
		{
		/* Elements: */
		public:
		PolygonModel::Scalar playerHeight; // Total height of player from eye to foot
		PolygonModel::Point playerFoot; // Foot position of player
		PolygonModel::Box playerBox; // Bounding box enclosing player "avatar" in z-is-up model coordinates
		
		/* Methods: */
		void set(PolygonModel::Scalar newPlayerHeight,const PolygonModel::Point& newPlayerFoot,PolygonModel::Scalar playerRadius); // Updates the alignment state with new player parameters
		};
	
	/* Elements: */
	private:
	MaterialManager* materialManager; // A material manager
	PolygonModel* model; // The MODEL!
	Vrui::Vector upVector; // Vector defining the "up" direction in model space
	PolygonModel::Scalar epsilon; // Small fudge value to robustify collision detection inside the model
	bool showBackfaces; // Flag whether to render back-facing polygons
	const HierarchicalTriangleSetBase::SubMesh* subMesh; // Pointer to highlighted submesh
	GLMotif::PopupMenu* mainMenu; // The application's main menu
	GLMotif::PopupWindow* subMeshDialog;
	GLMotif::TextField* nameField;
	GLMotif::TextField* numTrianglesField;
	GLMotif::TextField* bboxField[6];
	GLMotif::TextField* bboxCenterField[3];
	
	/* Private methods: */
	GLMotif::PopupMenu* createMainMenu(void);
	GLMotif::PopupWindow* createSubMeshDialog(void);
	
	/* Constructors and destructors: */
	public:
	PolygonMeshTest(int& argc,char**& argv,char**& appDefaults);
	virtual ~PolygonMeshTest(void);
	
	/* Methods from Vrui::Application: */
	virtual void toolCreationCallback(Vrui::ToolManager::ToolCreationCallbackData* cbData);
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	
	/* New methods: */
	void resetNavigationCallback(Misc::CallbackData* cbData);
	void showBackfacesCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
	void levelModelCallback(Misc::CallbackData* cbData);
	void scaleOneToOneCallback(Misc::CallbackData* cbData);
	void subMeshDialogCloseCallback(Misc::CallbackData* cbData);
	void updateSubMeshDialog(void);
	void alignSurfaceFrame(Vrui::SurfaceNavigationTool::AlignmentData& alignmentData);
	};

#endif
