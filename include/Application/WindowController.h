#pragma once

struct GLFWwindow;

namespace application
{
    void HandlePlatformInputAndViewport(GLFWwindow *window);
    void viewPort_Setup(GLFWwindow *window);
    void processInput(GLFWwindow *window);

}