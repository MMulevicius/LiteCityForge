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
#include <GLFW/glfw3.h>
#include <iostream>
#include <gl2d/gl2d.h>
#include <openglErrorReporting.h>
#include <Shader.h>
#include <stb_image/stb_image.h>
#include "Gui.h"

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
	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);
	GLFWwindow *window = glfwCreateWindow(mode->width, mode->height, "LiteCityForge", monitor, nullptr);
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
	road::LineRenderer roadLines;
	std::vector<glm::vec3> roadLineVerts;

	Shader primShader("../include/basic.vert", "../include/basic.frag");
	Shader lineShader("../include/line.vert", "../include/line.frag");

	if (!roadLines.Initialize_Road())
	{
		std::cout << "Failed to init LineRenderer\n";
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
		glLineWidth(2.0f);
		roadLines.Draw(lineShader, vp);
	}


	if(gui.WantsGenerate())
	{
		// roadParams.cityRadius = 40.0f;
		// roadParams.cityCenter = {0.0f, 0.0f};
		// roadParams.maxSegments = 150;
		// roadParams.initialRays = 4;
		// roadParams.highwayLength = 10.0f;
		// roadParams.seed = 1337;

		roadNet = roadGen.Generate(roadParams);
		showRoads = true;

		roadLineVerts.clear();
		roadLineVerts.reserve(roadNet.Segments().size() * 2);

		for (const auto &seg : roadNet.Segments())
		{
			const auto &A = roadNet.Nodes().at(seg.a - 1).pos;
			const auto &B = roadNet.Nodes().at(seg.b - 1).pos;

			roadLineVerts.push_back(glm::vec3(A.x, 0.05f, A.y));
			roadLineVerts.push_back(glm::vec3(B.x, 0.05f, B.y));
		}

		roadLines.Upload(roadLineVerts);
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
	roadLines.Shutdown_Road();
	primitives.Shutdown_Prim();
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

