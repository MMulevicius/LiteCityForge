#pragma once 
#include <GLFW/glfw3.h>
#include <string>
#include "Road/RoadParams.h"
#include "Road/LotParams.h"

class Gui 
{
public:
    //stores export extensions
    enum class ExportFormat { OBJ };

private:
    //window + init 
    GLFWwindow* mWindow = nullptr;
    bool mInitialized = false;

    //3D toggle and generate + quite one shot events
    bool mEnable3D = false;
    bool mGenerateRequested = false;
    bool mQuitRequested = false;

    //debug toggles
    bool mShowLotDebug = false;
    bool mShowSidewalks = false;
    bool mShowGardens = false;
    bool mShowFootprints = false;

    //camera reset one shot
    bool mWantsResetCamera = false;

    //export state and UI fields
    bool mExportRequested = false;
    ExportFormat mExportFormat = ExportFormat::OBJ;

    char mExportBaseName[64] = "city";
    char mExportDir[256] = "";

    //export result feedback
    bool mHasLastExport = false;
    bool mLastExportOk = false;
    std::string mLastExportPath;

    //time
    double mLastGenerationMs = -1.0;

    //params
    road::LotParams mLotParams;
    road::RoadParams mRoadParams;

public:
    Gui() = default;
    ~Gui() = default;

    double GetLastGenerationMs() const { return mLastGenerationMs; }

    // export API for main.cpp
    bool ConsumeExportRequest(std::string& outDir,
                              std::string& outBaseName,
                              ExportFormat& outFmt);

    void SetLastExportResult(bool ok, const std::string& fullPath);

    void SetLastGenerationMs(double ms) { mLastGenerationMs = ms; }

    // init / frame
    bool Initialize_GUI(GLFWwindow *window, const char *glslVersion = "#version 330");
    void BeginFrameGUI();
    void DrawGUI();
    void EndFrameGUI();
    void ShutdownGUI();
    

    // getters
    bool Is3DEnabled() const { return mEnable3D; }
    bool _3DEnabled() const { return mEnable3D; }

    bool ShowGardens() const { return mShowGardens; }
    bool ShowLotDebug() const { return mShowLotDebug; }
    bool ShowSideWalks() const { return mShowSidewalks; }
    bool ShowFootprints() const { return mShowFootprints; }

    bool WantsGenerate();
    bool WantsQuit();

    bool ConsumeResetCamera();

    float GetCityRadius() const { return mRoadParams.cityRadius; }
    const road::LotParams& GetLotParams() const { return mLotParams; }
    const road::RoadParams& GetParams() const { return mRoadParams; }
};
