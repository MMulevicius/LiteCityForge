#include "Application/WindowController.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace application
{
    // handles per-frame paltform input and viewport setup
    void HandlePlatformInputAndViewport(GLFWwindow *window)
    {

        processInput(window);
        viewPort_Setup(window);
    }

    // updates the OpenGL viewport and clears the frame buffers
    void viewPort_Setup(GLFWwindow *window)
    {
        int w = 0, h = 0;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);

        glClearColor(0.40f, 0.52f, 0.43f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    // handles basic window input such as closing on escape
    void processInput(GLFWwindow *window)
    {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
    }
}