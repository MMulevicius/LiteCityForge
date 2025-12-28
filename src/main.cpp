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
#include "Road/RoadNetwork.h"
#include "Road/BlockExtractor.h"
#include "Road/BlockTypes.h"
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
	road::LotParams lotParams;
	road::LotCollection lots;

	road::LineRenderer lotLines;
	std::vector<glm::vec3> lotLineVerts;

	road::BlockExtractor blockExtractor;
	std::vector<road::Block> blocks;
	road::LineRenderer blockLines;
	std::vector<glm::vec3> blockLineVerts;
	road::LineRenderer blockCentroidLines;
	std::vector<glm::vec3> blockCentroidVerts;
	

	Shader primShader("../include/basic.vert", "../include/basic.frag");
	Shader lineShader("../include/line.vert", "../include/line.frag");

	if (!highwayLines.Initialize_Road() || !streetLines.Initialize_Road() || 
		!lotLines.Initialize_Road() || !blockLines.Initialize_Road() ||
		!blockCentroidLines.Initialize_Road())
	{
		std::cout << "Failed to init LineRenderer for highways/streets/lots/blocks\n";
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

		if (gui.ShowBlocks())
		{
			glLineWidth(2.0f);
			lineShader.use();
			glUniform3f(glGetUniformLocation(lineShader.ID, "uColor"), 1.0f, 0.1f, 0.1f);
			blockLines.Draw(lineShader, vp);
			glLineWidth(1.0f);

			glLineWidth(3.0f);
			lineShader.use();
			glUniform3f(glGetUniformLocation(lineShader.ID, "uColor"), 1.0f, 0.0f, 0.0f);
			blockCentroidLines.Draw(lineShader, vp);

			glLineWidth(1.0f);
		}
	}



	if(gui.WantsGenerate())
	{
		road::RoadParams roadParams = gui.GetParams();
		std::vector<glm::vec3> highwayVerts;
		std::vector<glm::vec3> streetVerts;

		roadNet = roadGen.Generate(roadParams);
		std::cout << "Unsplit intersections: " << CountUnsplitIntersections(roadNet) << "\n";
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

		lotParams.seed = roadParams.seed;
		lots = lotGen.GenerateLots(roadNet, lotParams);
		
		blocks = blockExtractor.ExtractBlocks(roadNet);
		std::cout << "Blocks found: " <<blocks.size() << "\n";

		road::BuildLotLineVerts(lots, lotLineVerts, 0.02f);
		road::BuildBlockOutlineVerts(blocks, blockLineVerts, 0.03f);
		road::BuildBlockCentroidsVerts(blocks, blockCentroidVerts, 0.10f, 0.6f);


		//blockLineVerts.clear();
		// for(const auto& b : blocks)
		// {
		// 	const auto& poly = b.boundary;
		// 	for (size_t i = 0; i < poly.size(); i++)
		// 	{
		// 		const auto& p0 = poly[i];
		// 		const auto& p1 = poly[(i + 1) % poly.size()];
		// 		blockLineVerts.push_back(glm::vec3(p0.x, 0.20f, p0.y));
		// 		blockLineVerts.push_back(glm::vec3(p1.x, 0.20f, p1.y));
		// 	}
		// }

		highwayLines.Upload(highwayVerts);
		streetLines.Upload(streetVerts);
		lotLines.Upload(lotLineVerts);
		blockLines.Upload(blockLineVerts);
		blockCentroidLines.Upload(blockCentroidVerts);
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
	blockCentroidLines.Shutdown_Road();
	blockLines.Shutdown_Road();
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

