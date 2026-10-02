#define GLFW_INCLUDE_NONE
#include "Application/CameraController.h"
#include <GLFW/glfw3.h>

namespace application
{
    // updates free-fly 3D camera controls using mouse + keyboard input
    void UpdateCamera3DControls(Camera &camera, GLFWwindow *window, float deltaTime)
    {

        bool rmbDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

        double mx = 0.0, my = 0.0;
        glfwGetCursorPos(window, &mx, &my);

        camera.OnMouseMove(mx, my, rmbDown);

        glfwSetInputMode(window, GLFW_CURSOR, rmbDown ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

        camera.UpdateFly3D(window, deltaTime);
    }

    // updates 2D pan and zoom camera controls
    void UpdateCamera2DControls(Camera &camera, GLFWwindow *window, float deltaTime, float &scrollY)
    {

        camera.UpdatePanXZ(window, deltaTime);
        camera.ApplyScrollZoom(scrollY);
        scrollY = 0.0f;
    }

    // routes camera updates to either 2D or 3D controls
    void UpdateCameraPerFrame(Camera &camera, GLFWwindow *window, float deltaTime, bool is3D, float &scrollY)
    {

        if (is3D)
        {
            UpdateCamera3DControls(camera, window, deltaTime);
        }
        else
        {
            UpdateCamera2DControls(camera, window, deltaTime, scrollY);
        }
    }

    // applies GUI-driven camera mode changes and resets requests
    bool UpdateCameraModeAndResetFromGui(Gui &gui, Camera &camera, const CityContext &cityCTX)
    {

        static bool prev3D = false;
        bool is3D = gui.Is3DEnabled();

        if (is3D != prev3D)
        {
            camera.Set3DEnabled(is3D, cityCTX.centerXZ, cityCTX.cityR);
            prev3D = is3D;
        }

        if (gui.ConsumeResetCamera())
        {
            camera.ResetViewToCity(cityCTX.centerXZ, cityCTX.cityR);
        }

        return is3D;
    }

    // computes frame timing, frambuffer size and the current view projection matrix
    FrameContext BeginFrameTimingAndVP(GLFWwindow *window, Camera &camera)
    {

        FrameContext frameCTX;

        static float last = (float)glfwGetTime();
        float now = (float)glfwGetTime();
        frameCTX.deltaTime = now - last;
        last = now;

        glfwGetFramebufferSize(window, &frameCTX.w, &frameCTX.h);
        frameCTX.aspect = (frameCTX.h == 0) ? 1.0f : (float)frameCTX.w / (float)frameCTX.h;

        frameCTX.viewProjection = camera.GetVP(frameCTX.aspect);
        return frameCTX;
    }
}