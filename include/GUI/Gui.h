#pragma once 
#include <string>
#include "Road/RoadParams.h"
#include "Lots/LotParams.h"
#include "Lots/LotTypes.h"

struct GLFWwindow;

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

    // single texture selection UI
    struct TextureSlot
    {
        bool isImported = false;
        bool isApplied = false;

        char importedPath[256] = "";

        bool requestApply = false;
        bool requestClear = false;
    };
    // use of texture slots
    TextureSlot mBuildingTextures[3];
    TextureSlot mRoofTextures[3];
    TextureSlot mRoadTexture;
    TextureSlot mSidewalkTexture;
    TextureSlot mGroundTexture;

    //3D toggle and generate + quite one shot events
    bool mEnable3D = false;
    bool mGenerateRequested = false;
    bool mQuitRequested = false;

    //debug toggles
    bool mShowRoadLines = true;
    bool mShowLotDebug = false;
    bool mShowSidewalks = false;
    bool mShowGardens = false;
    bool mShowFootprints = false;

    //building details toggles rendering
    bool mRenderBuildingsRoofs = false;
    bool mRenderBuildingsWindows = false;

    //building details toggles exporting
    bool mExportBuildingsBase = true;
    bool mExportBuildingsRoofs = false;
    bool mExportBuildingsWindows = false;

    //window texture request
    bool mWindowTexApplyRequested = false;
    bool mWindowTexClearRequested = false;
    std::string mWindowTexPath;

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

    //export result feedback22
    bool mHasLastExport = false;
    bool mLastExportOk = false;
    std::string mLastExportPath;

    //time
    double mLastGenerationMs = -1.0;

    //params
    road::LotParams mLotParams;
    road::RoadParams mRoadParams;

    void DrawTextureSlotUI(TextureSlot& slot,const char* headerLabel,const char* dialogKey,const char* dialogTitle);



public:
    Gui() = default;
    ~Gui() = default;

    //generation timer
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

    bool ShowRoadLines() const {return mShowRoadLines; }
    bool ShowGardens() const { return mShowGardens; }
    bool ShowLotDebug() const { return mShowLotDebug; }
    bool ShowSideWalks() const { return mShowSidewalks; }
    bool ShowFootprints() const { return mShowFootprints; }

    //building details toggles
    //bool RenderBuildingBase() const {return mRenderBuildingsBase; }
    bool RenderBuildingRoofs() const {return mRenderBuildingsRoofs; }
    bool RenderBuildingWindows() const {return mRenderBuildingsWindows; }

    //export toggles 
    bool ExportBuildingBase() const {return mExportBuildingsBase; }
    bool ExportBuildingRoofs() const {return mExportBuildingsRoofs; }
    bool ExportBuildingWindows() const {return mExportBuildingsWindows; }

    //window texture
    bool ConsumeWindowTextureApply(std::string& outPath);
    bool ConsumeWindowTextureClear();

    //close application
    bool WantsGenerate();
    bool WantsQuit();

    //camera reset
    bool ConsumeResetCamera();
    bool ConsumeGuiRecreateRequest();

    //building texture
    bool ConsumeBuildingTextureApply(road::LotZone zone, std::string& outPath);
    bool ConsumeBuildingTextureClear(road::LotZone zone);

    //roof texture
    bool ConsumeRoofTextureApply(road::LotZone zone, std::string& outPath);
    bool ConsumeRoofTextureClear(road::LotZone zone);

    // roads/sidewalk/ground
    bool ConsumeRoadTextureApply(std::string& outPath);
    bool ConsumeRoadTextureClear();

    bool ConsumeSidewalkTextureApply(std::string& outPath);
    bool ConsumeSidewalkTextureClear();

    bool ConsumeGroundTextureApply(std::string& outPath);
    bool ConsumeGroundTextureClear();

    //params accessor
    float GetCityRadius() const { return mRoadParams.cityRadius; }
    const road::LotParams& GetLotParams() const { return mLotParams; }
    const road::RoadParams& GetParams() const { return mRoadParams; }
};
