#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <gl2d/gl2d.h>
#include <openglErrorReporting.h>
#include <Shader.h>
#include <stb_image/stb_image.h>

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

	GLFWwindow *window = glfwCreateWindow(1000, 1000, "LiteCityForge", NULL, NULL);
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

	// Shader mainShader("../include/triangle.vert",
    //              	"../include/triangle.frag");


	enableReportGlErrors();
	
	//glfwSwapInterval(1); //vsync

#pragma region imgui
#if REMOVE_IMGUI == 0
	ImGui::CreateContext();
	//ImGui::StyleColorsDark();				//you can use whatever imgui theme you like!
	//imguiThemes::yellow();
	//imguiThemes::gray();
	//imguiThemes::green();
	imguiThemes::red();
	//imguiThemes::embraceTheDarkness();

	ImGuiIO &io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
	//io.ConfigViewportsNoAutoMerge = true;
	//io.ConfigViewportsNoTaskBarIcon = true;

	io.FontGlobalScale = 2.0f; //make text bigger please!

	ImGuiStyle &style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		//style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 0.f;
		style.Colors[ImGuiCol_DockingEmptyBg].w = 0.f;
	}

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");
#endif
#pragma endregion



	gl2d::init();
	gl2d::Renderer2D renderer;
	renderer.create();

while (!glfwWindowShouldClose(window))
{
	processInput(window);
	viewPort_Setup(window);

	//mainShader.use();

    glfwSwapBuffers(window);
    glfwPollEvents();
}

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
    glClear(GL_COLOR_BUFFER_BIT);
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

