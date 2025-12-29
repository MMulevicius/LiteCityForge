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


static void error_callback(int error, const char *description);
void processInput(GLFWwindow *window);
void viewPort_Setup(GLFWwindow *window);


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

	

	Primitives primitives;
	primitives.Initialize_Prim();

	Camera camera;

	bool showRoads = false;
	road::RoadGenerator roadGen;
	road::RoadParams roadParams;
	road::RoadNetwork roadNet;
	//road::LineRenderer roadLines;
	road::LineRenderer highwayLines;
	road::LineRenderer streetLines;
	std::vector<glm::vec3> roadLineVerts;

	road::LotSubdivision lotGen;
	road::LotCollection lots;

	road::LineRenderer lotLines;
	std::vector<glm::vec3> lotLineVerts;

	road::LineRenderer sidewalkLines;
	std::vector<glm::vec3> sidewalkLineVerts;

	road::LineRenderer gardenLines;
	std::vector<glm::vec3> gardenLineVerts;

	

	Shader primShader("../include/basic.vert", "../include/basic.frag");
	Shader lineShader("../include/line.vert", "../include/line.frag");

	if (!highwayLines.Initialize_Road() || !streetLines.Initialize_Road() || 
		!lotLines.Initialize_Road() || !sidewalkLines.Initialize_Road()
		|| !gardenLines.Initialize_Road())
	{
		std::cout << "Failed to init LineRenderer for highways/streets/lots/roads/garden\n";
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

	processInput(window);
	viewPort_Setup(window);
	// Camera adjustment
	static float last = (float)glfwGetTime();
	float now = (float)glfwGetTime();
	float dt = now - last;
	last = now;


	int w, h;
	glfwGetFramebufferSize(window, &w, &h);
	float aspect = (h == 0) ? 1.0f : (float)w / (float)h;
	camera.UpdatePanXZ(window, dt);

	glm::mat4 proj = glm::ortho(-50.0f * aspect, 50.0f * aspect, -50.0f, 50.0f, -100.0f, 100.0f);
	glm::mat4 view = glm::lookAt(glm::vec3(0, 40, 40), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	glm::mat4 vp = camera.GetVPOrtho(aspect);

	//Draw ground
	primitives.DrawGround(primShader, vp);


	//Generate Roads
	if (showRoads)
	{
		//Highway first
		glLineWidth(4.0f);
		lineShader.use();
		glUniform3f(glGetUniformLocation(lineShader.ID, "uColor"), 0.05f, 0.05f, 0.05f);
		highwayLines.Draw(lineShader, vp);

		//Streets
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

	}



	if(gui.WantsGenerate())
	{
		roadParams = gui.GetParams();
		std::vector<glm::vec3> highwayVerts;
		std::vector<glm::vec3> streetVerts;

		roadNet = roadGen.Generate(roadParams);
		//std::cout << "Unsplit intersections: " << CountUnsplitIntersections(roadNet) << "\n";
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

		// lotParams.minLotAreaForGarden = 0.0f;
		// lotParams.minGardenDepth = 0.1f;
		// lotParams.gardenBackRatio = 0.35f;


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

			// Step 1: zone
			if (l.zone != road::LotZone::Suburban) continue;
			passZone++;

			// Step 2: area
			if (l.area < lotParams.minLotAreaForGarden) continue;
			passArea++;

			// Step 3: depth feasibility (recompute from boundary)
			if (l.boundary.size() < 4) continue;
			float fullDepth = glm::length(l.boundary[3] - l.boundary[0]);
			float gardenDepth = fullDepth * lotParams.gardenBackRatio;

			if (gardenDepth < lotParams.minGardenDepth) continue;
			if ((fullDepth - gardenDepth) < 0.5f) continue;
			passDepth++;

			// Step 4: actual result
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

		highwayLines.Upload(highwayVerts);
		streetLines.Upload(streetVerts);
		lotLines.Upload(lotLineVerts);
		sidewalkLines.Upload(sidewalkLineVerts);
		gardenLines.Upload(gardenLineVerts);

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
	gui.ShutdownGUI();
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}

void viewPort_Setup(GLFWwindow *window)
{
	int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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

