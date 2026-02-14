#pragma once 
#include <GLFW/glfw3.h>
#include <string>
#include "Road/RoadParams.h"
#include "Road/LotParams.h"
#include "Road/LotTypes.h"

class Gui 
{
public:
    //stores export extensions
    enum class ExportFormat { OBJ };

private:
    //window + init 
    GLFWwindow* mWindow = nullptr;
    bool mInitialized = false;

    // building material / texture UI
    bool mShowBuildingMaterialsWindow = false;

    struct TextureSlot
    {
        bool isImported = false;
        bool isApplied = false;

        char importedPath[256] = "";

        bool requestApply = false;
        bool requestClear = false;
    };

    TextureSlot mBuildingTextures[3];

    //3D toggle and generate + quite one shot events
    bool mEnable3D = false;
    bool mGenerateRequested = false;
    bool mQuitRequested = false;

    //debug toggles
    bool mShowLotDebug = false;
    bool mShowSidewalks = false;
    bool mShowGardens = false;
    bool mShowFootprints = false;

    // settings window + UI
    bool mShowSettingsWindow = false;
    bool mFullscreen = false;
    bool mHasWindowedRect = false;
    bool mRequestGuiRecreate = false;
    bool mSkipDockspaceNextFrame = false;
    bool mForceMainWindowDefaultNextFrame = false;
    bool mForceSettingsWindowDefaultNextFrame = false;

    // UI scale
    float mUiScale = 1.0f;

    // fullscreen state + restore windowed rect
    int mWindowedX = 100;
    int mWindowedY = 100;
    int mWindowedW = 1280;
    int mWindowedH = 800;


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

    // settings
    void ApplyUiScale();
    void SetFullscreen(bool enabled);
    void ResetImGuiLayout();
    

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
    bool ConsumeGuiRecreateRequest();

    bool ConsumeBuildingTextureApply(road::LotZone zone, std::string& outPath);
    bool ConsumeBuildingTextureClear(road::LotZone zone);


    float GetCityRadius() const { return mRoadParams.cityRadius; }
    const road::LotParams& GetLotParams() const { return mLotParams; }
    const road::RoadParams& GetParams() const { return mRoadParams; }
};
