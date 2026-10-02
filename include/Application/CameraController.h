#pragma once

#include "Core/Camera.h"
#include "GUI/Gui.h"
#include "Rendering/RenderContexts.h"

struct GLFWwindow;

namespace application
{
    void UpdateCamera3DControls(
        Camera &camera,
        GLFWwindow *window,
        float deltaTime);

    void UpdateCamera2DControls(
        Camera &camera,
        GLFWwindow *window,
        float deltaTime,
        float &scrollY);

    void UpdateCameraPerFrame(
        Camera &camera,
        GLFWwindow *window,
        float deltaTime,
        bool is3D,
        float &scrollY);

    bool UpdateCameraModeAndResetFromGui(
        Gui &gui,
        Camera &camera,
        const CityContext &city);

    FrameContext BeginFrameTimingAndVP(
        GLFWwindow *window,
        Camera &camera);
}