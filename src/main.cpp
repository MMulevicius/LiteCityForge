#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Primitives.h"
#include "Camera.h"
#include "Road/RoadGenerator.h"
#include "Road/RoadParams.h"
#include "Road/LineRenderer.h"
#include "Road/LotSubdivision.h"
#include "Road/LotParams.h"
#include "Road/LotTypes.h"
#include "Road/SideWalkGenerator.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <gl2d/gl2d.h>
#include <openglErrorReporting.h>
#include <Shader.h>
#include <stb_image/stb_image.h>
#include "Gui.h"
#include <iostream>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imguiThemes.h"


//global Y axis variable for zooming in/out
static float gScrollY = 0.0f;


static void error_callback(int error, const char *description);
void processInput(GLFWwindow *window);
void viewPort_Setup(GLFWwindow *window);

void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);


int main(void)
{

	glfwSetErrorCallback(error_callback);

	if (!glfwInit())
		exit(EXIT_FAILURE);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, 1);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#endif
	//GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	//const GLFWvidmode* mode = glfwGetVideoMode(monitor);
	GLFWwindow *window = glfwCreateWindow(1280, 800, "LiteCityForge", nullptr, nullptr);
	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwSetScrollCallback(window, ScrollCallback);


	glfwMakeContextCurrent(window);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	Gui gui;
	if (!gui.Initialize_GUI(window, "#version 330"))
	{
		return -1;
	}

	
	//ground variables
	Primitives primitives;
	primitives.Initialize_Prim();

	//camera variables
	Camera camera;
	camera.Set3DEnabled(false, glm::vec2(0.0f, 0.0f), gui.GetCityRadius());


	//road variables
	bool showRoads = false;
	road::RoadGenerator roadGen;
	road::RoadParams roadParams;
	road::RoadNetwork roadNet;
	road::LineRenderer highwayLines;
	road::LineRenderer streetLines;
	std::vector<glm::vec3> roadLineVerts;

	//lot variables
	road::LotSubdivision lotGen;
	road::LotCollection lots;
	road::LineRenderer lotLines;
	std::vector<glm::vec3> lotLineVerts;

	//sidewalk variables
	road::LineRenderer sidewalkLines;
	std::vector<glm::vec3> sidewalkLineVerts;

	//garden variables
	road::LineRenderer gardenLines;
	std::vector<glm::vec3> gardenLineVerts;

	//building footprint variables
	road::LineRenderer footprintLines;
	std::vector<glm::vec3> footprintLineVerts;

	

	Shader primShader("../include/basic.vert", "../include/basic.frag");
	Shader lineShader("../include/line.vert", "../include/line.frag");

	if (!highwayLines.Initialize_Road() || !streetLines.Initialize_Road() || 
		!lotLines.Initialize_Road() || !sidewalkLines.Initialize_Road()
		|| !gardenLines.Initialize_Road() || !footprintLines.Initialize_Road())
	{
		std::cout << "Failed to init LineRenderer for highways/streets/lots/roads/garden/footprints\n";
	}

	glEnable(GL_DEPTH_TEST);
	enableReportGlErrors();
	
	//glfwSwapInterval(1); //vsync

	// gl2d::init();
	// gl2d::Renderer2D renderer;
	// renderer.create();

while (!glfwWindowShouldClose(window))
{
	//GUI
	gui.BeginFrameGUI();
	gui.DrawGUI();

	
	float cityR = showRoads ? roadParams.cityRadius : gui.GetCityRadius();
	glm::vec2 centerXZ = showRoads
		? glm::vec2(roadParams.cityCenter.x, roadParams.cityCenter.y)
		: glm::vec2(0.0f, 0.0f); 
	
	static bool prev3D = false;
	bool is3D = gui._3DEnabled(); 

	if (is3D != prev3D)
	{
		camera.Set3DEnabled(is3D, centerXZ, cityR);
		prev3D = is3D;
	}

	// //reset camera
	if (gui.ConsumeResetCamera())
	{
		camera.ResetViewToCity(centerXZ, cityR);
	}





	processInput(window);
	viewPort_Setup(window);

	// camera  and ground adjustment
	static float last = (float)glfwGetTime();
	float now = (float)glfwGetTime();
	float dt = now - last;
	last = now;
	int w, h;

	camera.Update(dt);
	// only rotate when in 3D mode
	if (is3D)
	{
		bool rmbDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
		double mx, my;
		glfwGetCursorPos(window, &mx, &my);

		camera.OnMouseMove(mx, my, rmbDown);
		glfwSetInputMode(window, GLFW_CURSOR, rmbDown ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
		camera.UpdateFly3D(window, dt);
	}
	else 
	{
		camera.UpdatePanXZ(window, dt);
		camera.ApplyScrollZoom(gScrollY);
		gScrollY = 0.0f;
	}




	glfwGetFramebufferSize(window, &w, &h);
	float aspect = (h == 0) ? 1.0f : (float)w / (float)h;
	glm::mat4 vp = camera.GetVP(aspect);





	// extra ground area beyond city boundries 
	float margin = 20.0f;

	// ground half extents based on city size
	float halfW = cityR + margin;
	float halfH = cityR + margin;

	primitives.DrawGround(primShader, vp, centerXZ, halfW, halfH);


	//generate Roads
	if (showRoads)
	{
		//highway first
		glLineWidth(4.0f);
		lineShader.use();
		glUniform3f(glGetUniformLocation(lineShader.ID, "uColor"), 0.05f, 0.05f, 0.05f);
		highwayLines.Draw(lineShader, vp);

		//streets
		glLineWidth(1.0f);
		lineShader.use();
		glUniform3f(glGetUniformLocation(lineShader.ID, "uColor"), 0.05f, 0.05f, 0.05f);
		streetLines.Draw(lineShader, vp);

		//lots 
		if (gui.ShowLotDebug())
		{
			glLineWidth(2.0f);
			lineShader.use();
			glUniform3f(glGetUniformLocation(lineShader.ID, "uColor"), 0.1f, 0.4f, 1.0f);
			lotLines.Draw(lineShader, vp);
			glLineWidth(1.0f);
		}

		//sidewalks
		if (gui.ShowSideWalks())
		{
			glLineWidth(2.0f);
			lineShader.use();
			glUniform3f(glGetUniformLocation(lineShader.ID, "uColor"), 0.65f, 0.65f, 0.65f);
			sidewalkLines.Draw(lineShader, vp);
			glLineWidth(1.0f);
		}

		//gardens
		if (gui.ShowGardens())
		{
			glLineWidth(2.0f);
			lineShader.use();
			glUniform3f(glGetUniformLocation(lineShader.ID, "uColor"), 0.1f, 0.9f, 0.2f);
			gardenLines.Draw(lineShader, vp);
			glLineWidth(1.0f);
		}

		//building footprints
		if (gui.ShowFootprints())
		{
			glLineWidth(2.0f);
			lineShader.use();
			glUniform3f(glGetUniformLocation(lineShader.ID, "uColor"), 0.95f, 0.65f, 0.15f);
			footprintLines.Draw(lineShader, vp);
			glLineWidth(1.0f);
		}


	}



	if(gui.WantsGenerate())
	{
		roadParams = gui.GetParams();

		std::cout
		<< "[GUI] cityRadius=" << roadParams.cityRadius
		<< " maxIterations=" << roadParams.maxIterations
		<< " maxStreets=" << roadParams.maxStreetSegments   
		<< " maxHighways=" << roadParams.maxHighwaySegments 
		<< "\n";

		std::vector<glm::vec3> highwayVerts;
		std::vector<glm::vec3> streetVerts;

		roadNet = roadGen.Generate(roadParams);
		showRoads = true;

		highwayVerts.clear();
		streetVerts.clear();

		highwayVerts.reserve(roadNet.Segments().size() * 2);
		streetVerts.reserve(roadNet.Segments().size() * 2);

		for (const auto &seg : roadNet.Segments())
		{
			const auto &A = roadNet.Nodes().at(seg.a - 1).pos;
			const auto &B = roadNet.Nodes().at(seg.b - 1).pos;

			glm::vec3 a3(A.x, 0.05f, A.y);
			glm::vec3 b3(B.x, 0.05f, B.y);

			if (seg.type == road::RoadType::Highway)
			{
				highwayVerts.push_back(a3);
				highwayVerts.push_back(b3);
			}
			else
			{
				streetVerts.push_back(a3);
				streetVerts.push_back(b3);
			}
		}
		road::LotParams lotParams = gui.GetLotParams();
		lotParams.seed = roadParams.seed;
		lotParams.cityRadius = roadParams.cityRadius;

		lotParams.streetHalfWidth = roadParams.streetHalfWidth;
		lotParams.highwayHalfWidth = roadParams.highwayHalfWidth;
		lots = lotGen.GenerateLots(roadNet, lotParams);

		int nUrban = 0, nSub = 0, nRural = 0;
		int passZone = 0, passArea = 0, passDepth = 0, passAll = 0;

		for (const auto& l : lots.lots)
		{
			if (l.zone == road::LotZone::Urban) nUrban++;
			else if (l.zone == road::LotZone::Suburban) nSub++;
			else nRural++;

			// zone
			if (l.zone != road::LotZone::Suburban) continue;
			passZone++;

			// area
			if (l.area < lotParams.minLotAreaForGarden) continue;
			passArea++;

			// depth feasibility (recompute from boundary)
			if (l.boundary.size() < 4) continue;
			float fullDepth = glm::length(l.boundary[3] - l.boundary[0]);
			float gardenDepth = fullDepth * lotParams.gardenBackRatio;

			if (gardenDepth < lotParams.minGardenDepth) continue;
			if ((fullDepth - gardenDepth) < 0.5f) continue;
			passDepth++;

			// actual result
			if (l.hasGarden) passAll++;
		}

		std::cout << "Zones => Urban: " << nUrban << " Suburban: " << nSub << " Rural: " << nRural << "\n";
		std::cout << "Garden pipeline => passZone: " << passZone
				<< " passArea: " << passArea
				<< " passDepth: " << passDepth
				<< " hasGarden: " << passAll << "\n";

		float minA = 1e9f, maxA = 0.0f;
		for (const auto& l : lots.lots)
		{
			minA = std::min(minA, l.area);
			maxA = std::max(maxA, l.area);
		}
		std::cout << "Lot area range: min=" << minA << " max=" << maxA
				<< " (threshold=" << lotParams.minLotAreaForGarden << ")\n";


		road::BuildLotLineVerts(lots, lotLineVerts, 0.02f);
		road::BuildSidewalkLineVerts(roadNet, roadParams, sidewalkLineVerts, 0.06f);
		road::BuildGardenLineVerts(lots, gardenLineVerts, 0.021f);
		road::BuildFootprintLineVerts(lots, footprintLineVerts, 0.022f);


		highwayLines.Upload(highwayVerts);
		streetLines.Upload(streetVerts);
		lotLines.Upload(lotLineVerts);
		sidewalkLines.Upload(sidewalkLineVerts);
		gardenLines.Upload(gardenLineVerts);
		footprintLines.Upload(footprintLineVerts);

		std::cout << "Lots count: " << lots.lots.size() << "\n";
		std::cout << "Lot verts: " << lotLineVerts.size() << "\n";
		std::cout << "Garden verts: " << gardenLineVerts.size() << "\n";


	}


	if (gui.WantsQuit())
	{
		glfwSetWindowShouldClose(window, true);
	}


	gui.EndFrameGUI();

    glfwSwapBuffers(window);
    glfwPollEvents();
}
	//all shutdowns
	highwayLines.Shutdown_Road();
	streetLines.Shutdown_Road();
	primitives.Shutdown_Prim();
	lotLines.Shutdown_Road();
	sidewalkLines.Shutdown_Road();
	gardenLines.Shutdown_Road();
	footprintLines.Shutdown_Road();
	gui.ShutdownGUI();
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}

void viewPort_Setup(GLFWwindow *window)
{
	int w = 0, h = 0;
	glfwGetFramebufferSize(window, &w, &h);
	glViewport(0, 0, w, h);

	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	float aspect = (h == 0) ? 1.0f : (float)w / (float)h;

}

void processInput(GLFWwindow *window)
{
	if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

static void error_callback(int error, const char *description)
{
	std::cout << "Error: " <<  description << "\n";
}

void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	ImGuiIO& io = ImGui::GetIO();


    if (io.WantCaptureMouse)
        return;


    gScrollY += (float)yoffset;
}


