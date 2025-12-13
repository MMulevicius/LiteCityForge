#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <gl2d/gl2d.h>
#include <openglErrorReporting.h>
#include <myShader.h>
#include <stb_image/stb_image.h>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imguiThemes.h"



static void error_callback(int error, const char *description)
{
	std::cout << "Error: " <<  description << "\n";
}

void processInput(GLFWwindow *window)
{
	if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

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

	GLFWwindow *window = glfwCreateWindow(1000, 1000, "Practice OpenGL Window", NULL, NULL);
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

	Shader ourShader("../include/triangle.vert",
                 	"../include/triangle.frag");




	float vertices[] = {
    // positions          // colors           // texture coords
     0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // top right
     0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   // bottom right
    -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // bottom left
    -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f    // top left 
};

	unsigned int indices[] = {
	 	0, 1, 2,
	 	0, 2, 3
	 };

	unsigned int VBO, VAO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// color
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	//texture attribute
	 glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6* sizeof(float)));
	 glEnableVertexAttribArray(2);

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);


	// glBindBuffer(GL_ARRAY_BUFFER, 0);

	// glBindVertexArray(0);
	unsigned int texture1, texture2;
	glGenTextures(1, &texture1);
	glBindTexture(GL_TEXTURE_2D, texture1);

	// Set texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Optional: flip image vertically (OpenGL expects 0,0 at bottom-left)
	stbi_set_flip_vertically_on_load(true);
	int texWidth, texHeight, texChannels;
	unsigned char *data = stbi_load("../resources/wall.jpg",
                                &texWidth, &texHeight, &texChannels, 0);

	// Upload pixels to GPU
	if (data) {
		glTexImage2D(GL_TEXTURE_2D,
					0,
					texChannels == 4 ? GL_RGBA : GL_RGB,
					texWidth, texHeight,
					0,
					texChannels == 4 ? GL_RGBA : GL_RGB,
					GL_UNSIGNED_BYTE,
					data);
		glGenerateMipmap(GL_TEXTURE_2D);
    	stbi_image_free(data); // free CPU memory
	}


	// ========== TEXTURE 2: AWESOMEFACE ==========
	glGenTextures(1, &texture2);
	glBindTexture(GL_TEXTURE_2D, texture2);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	data = stbi_load("../resources/awesomeface.png", &texWidth, &texHeight, &texChannels, 0);

	GLenum format2 = (texChannels == 4) ? GL_RGBA : GL_RGB; // PNG likely has alpha
	glTexImage2D(GL_TEXTURE_2D, 0, format2, texWidth, texHeight, 0, format2, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
	stbi_image_free(data);

	ourShader.use();

	glUniform1i(glGetUniformLocation(ourShader.ID, "uTexture"), 0);
	glUniform1i(glGetUniformLocation(ourShader.ID, "uTexture2"), 1);

	// const char* vertexShaderSource = R"(
	// #version 330 core
	// layout (location = 0) in vec3 aPos;
	// layout (location = 1) in vec3 aColor;

	// out vec3 ourColor;

	// void main()
	// {
	// 	gl_Position = vec4(aPos, 1.0);
	// 	ourColor = aColor;
	// }
	// )";

	// const char* fragmentShaderSource = R"(
	// #version 330 core
	// out vec4 FragColor;
	// in vec3 ourColor;


	// void main()
	// {
	// 	FragColor = vec4(ourColor, 1.0);
	// }
	// )";

	

	// unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	// glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	// glCompileShader(vertexShader);

	// // Shader compile error catcher
	// int success;
	// char infoLog[512];
	// glGetShaderiv(vertexShader, GL_COMPILE_STATUS,&success);

	// if (!success)
	// {
	// 	glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
	// 	std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog <<std::endl;
	// }

	// unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	// glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	// glCompileShader(fragmentShader);

	// unsigned int shaderProgram = glCreateProgram();
	// glAttachShader(shaderProgram, vertexShader);
	// glAttachShader(shaderProgram, fragmentShader);
	// glLinkProgram(shaderProgram);

	// // delete shaders after linking
	// glDeleteShader(vertexShader);
	// glDeleteShader(fragmentShader);



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

    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

	ourShader.use();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture1);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture2);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);


    glfwSwapBuffers(window);
    glfwPollEvents();
}

	glfwDestroyWindow(window);

	glfwTerminate();

	return 0;
}

