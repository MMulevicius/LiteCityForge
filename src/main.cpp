#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Primitives.h"
#include "Camera.h"
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

	Shader primShader("../include/basic.vert", "../include/basic.frag");

	Primitives primitives;
	primitives.Initialize_Prim();

	Camera camera;

	bool showCube = false;

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


	//Draw gorund
	primitives.DrawGround(primShader, vp);

	//Spawn cube if "Generate" is pressed
	if (gui.WantsGenerate()) showCube = true;
	if (gui.WantsClear()) showCube = false;

	if (showCube)
	{
		glm::mat4 model(1.0f);
		model = glm::translate(model, glm::vec3(0,0.5f, 0));
		model = glm::scale(model, glm::vec3(5, 1, 5));
		primitives.DrawCube(primShader, vp, model);
	}
	if (gui.WantsQuit())
	{
		glfwSetWindowShouldClose(window, true);
	}


	gui.EndFrameGUI();

    glfwSwapBuffers(window);
    glfwPollEvents();
}
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

