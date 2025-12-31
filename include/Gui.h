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
    bool mShowFootprints = false;
    bool mWantsResetCamera = false;
    road::LotParams mLotParams;
    road::RoadParams mRoadParams;
    

    public:
    Gui() = default;
    ~Gui() = default;

    //initialization once 
    bool Initialize_GUI(GLFWwindow *window, const char *glslVersion = "#version 330");

    //once per frame
    void BeginFrameGUI();
    void DrawGUI();
    void EndFrameGUI();

    //once on shutdown
    void ShutdownGUI();

    //state getters
    bool Is3DEnabled() const 
    {
        return mEnable3D;
    }
    bool _3DEnabled() const { return mEnable3D;}
    bool ShowGardens() const { return mShowGardens;}
    bool ShowLotDebug() const { return mShowLotDebug;}
    bool ShowSideWalks() const { return mShowSidewalks;}
    bool ShowFootprints() const { return mShowFootprints;}
    bool WantsResetCamera() const {return mWantsResetCamera;}
    bool WantsGenerate();
    bool WantsQuit();

    bool ConsumeResetCamera();
    float GetCityRadius() const {return mRoadParams.cityRadius;}
    const road::LotParams& GetLotParams() const {return mLotParams; }
    const road::RoadParams& GetParams() const {return mRoadParams; }
};