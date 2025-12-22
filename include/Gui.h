#pragma once 
#include <GLFW/glfw3.h>

class Gui 
{
    private:
    GLFWwindow* mWindow = nullptr;
    bool mInitialized = false;
    bool mEnable3D = false;
    bool mGenerateRequested = false;
    bool mQuitRequested = false;

    public:
    Gui() = default;
    ~Gui() = default;

    //Initialization once after window + OpenGL context is created
    bool Initialize_GUI(GLFWwindow *window, const char *glslVersion = "#version 330");

    //Once per frame
    void BeginFrameGUI();
    void DrawGUI();
    void EndFrameGUI();

    //Once on shutdown
    void ShutdownGUI();

    //State getters
    bool Is3DEnabled() const 
    {
        return mEnable3D;
    }

    bool WantsGenerate();
    bool WantsQuit();

};