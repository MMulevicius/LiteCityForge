#pragma once 
#include <GLFW/glfw3.h>
#include <string>
#include "Road/RoadParams.h"
#include "Road/LotParams.h"

class Gui 
{
public:
    enum class ExportFormat { OBJ };

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

    bool mExportRequested = false;
    ExportFormat mExportFormat = ExportFormat::OBJ;

    char mExportBaseName[64] = "city";
    char mExportDir[256] = "";

    bool mLastExportOk = false;
    std::string mLastExportPath;

    // params
    road::LotParams mLotParams;
    road::RoadParams mRoadParams;

public:
    Gui() = default;
    ~Gui() = default;

    // export API for main.cpp
    bool ConsumeExportRequest(std::string& outDir,
                              std::string& outBaseName,
                              ExportFormat& outFmt);

    void SetLastExportResult(bool ok, const std::string& fullPath);

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
