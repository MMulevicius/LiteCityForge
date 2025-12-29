#pragma once 
#include <GLFW/glfw3.h>
#include "Road/RoadParams.h"
#include "Road/LotParams.h"

class Gui 
{
    private:
    GLFWwindow* mWindow = nullptr;
    bool mInitialized = false;
    bool mEnable3D = false;
    bool mGenerateRequested = false;
    bool mQuitRequested = false;
    bool mShowLotDebug = false;
    bool mShowSidewalks = false;
    bool mShowGardens = false;
    road::LotParams mLotParams;
    road::RoadParams mRoadParams;
    

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
    bool ShowGardens() const { return mShowGardens;}
    bool ShowLotDebug() const { return mShowLotDebug;}
    bool ShowSideWalks() const { return mShowSidewalks;}
    //bool ShowBlocks() const { return mShowBlocks;}
    bool WantsGenerate();
    bool WantsQuit();

    const road::LotParams& GetLotParams() const {return mLotParams; }
    const road::RoadParams& GetParams() const {return mRoadParams; }
};