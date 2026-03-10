#include "GUI/Gui.h"
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "ImGuiFileDialog.h"
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <algorithm>
#include <filesystem>


// image filter used across texture import
static const char* kImageFileFilter = "Image files{.png,.jpg,.jpeg,.bmp,.tga},.*";

// draws a standard UI block for a TextureSlot:
// imported path display
// import / Apply / Clear buttons
// handles ImGuiFileDialog result and writes into slot.importedPath
void Gui::DrawTextureSlotUI(
    TextureSlot& slot,
    const char* headerLabel,
    const char* dialogKey,
    const char* dialogTitle
)
{
    ImGui::SeparatorText(headerLabel);

    // imported path label
    if (slot.isImported && slot.importedPath[0] != '\0')
        ImGui::TextWrapped("Imported: %s", slot.importedPath);
    else
        ImGui::TextUnformatted("Imported: (none)");

    // import
    if (ImGui::Button((std::string("Import##") + dialogKey).c_str()))
    {
        IGFD::FileDialogConfig config;
        config.flags = ImGuiFileDialogFlags_Modal;
        config.path = std::filesystem::current_path().string();

        ImGuiFileDialog::Instance()->OpenDialog(
            dialogKey,
            dialogTitle,
            kImageFileFilter,
            config
        );
    }

    // apply
    ImGui::SameLine();
    const bool canApply = slot.isImported && slot.importedPath[0] != '\0';
    if (!canApply) ImGui::BeginDisabled();
    if (ImGui::Button((std::string("Apply##") + dialogKey).c_str()))
    {
        slot.requestApply = true;
        slot.isApplied = true;
    }
    if (!canApply) ImGui::EndDisabled();

    // clear
    ImGui::SameLine();
    if (ImGui::Button((std::string("Clear##") + dialogKey).c_str()))
    {
        slot.requestClear = true;
        slot.isImported = false;
        slot.isApplied = false;
        slot.importedPath[0] = '\0';
    }

    // dialog result handling
    ImGui::SetNextWindowSize(ImVec2(900, 550), ImGuiCond_FirstUseEver);
    if (ImGuiFileDialog::Instance()->Display(dialogKey))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            auto selection = ImGuiFileDialog::Instance()->GetSelection();
            if (!selection.empty())
            {
                const std::string picked = selection.begin()->second;
                std::snprintf(slot.importedPath, IM_ARRAYSIZE(slot.importedPath), "%s", picked.c_str());
                slot.isImported = true;
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }
}


// finds project's assets/ folder
static std::filesystem::path FindAssetsRoot()
{
    // 8 parent hoops to search for assets/textures
    std::filesystem::path p = std::filesystem::current_path();
    for (int i = 0; i < 8; ++i)
    {
        std::filesystem::path candidate = p / "assets" / "textures";
        if (std::filesystem::exists(candidate))
            return p / "assets";
        if (!p.has_parent_path())
            break;
        p = p.parent_path();
    }
    return {}; // not found
}

//converts a filesystem path to string
static std::string ToStringPath(const std::filesystem::path& p)
{
    return p.empty() ? std::string{} : p.string();
}

//setup GUI
bool Gui::Initialize_GUI(GLFWwindow *window, const char* glslVersion)
{
    
    //store the GLFW window pointer for further use
    mWindow = window;

    //checks if header and compiled versions match
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    //IO struct keyboard/mouse config
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    io.IniFilename = "imgui.ini";

    //enable docking
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    //set theme is dark
    ImGui::StyleColorsDark();

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    //connects ImGui to GLFW and sets up OpenGL rendering backend
    if(!ImGui_ImplGlfw_InitForOpenGL(mWindow, true)) return false;
    if(!ImGui_ImplOpenGL3_Init(glslVersion)) return false;

    //making sure that the GUI is ready
    mInitialized = true;
    return true;
}

void Gui::SetFullscreen(bool enabled)
{
    if (!mWindow) return;
    if (enabled == mFullscreen) return;

    if (enabled)
    {
        // save windowed position/size so we can restore it later
        glfwGetWindowPos(mWindow, &mWindowedX, &mWindowedY);
        glfwGetWindowSize(mWindow, &mWindowedW, &mWindowedH);
        mHasWindowedRect = true;

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        // fullscreen on primary monitor
        glfwSetWindowMonitor(
            mWindow,
            monitor,
            0, 0,
            mode->width, mode->height,
            mode->refreshRate
        );
    }
    else
    {
        // restore previous windowed size/position (fallback if unknown)
        const int x = mHasWindowedRect ? mWindowedX : 100;
        const int y = mHasWindowedRect ? mWindowedY : 100;
        const int w = mHasWindowedRect ? mWindowedW : 1280;
        const int h = mHasWindowedRect ? mWindowedH : 800;

        glfwSetWindowMonitor(mWindow, nullptr, x, y, w, h, 0);
    }

    mFullscreen = enabled;
}
//consume block:
//return the flag
//reset it to false
bool Gui::ConsumeGuiRecreateRequest()
{
    bool v = mRequestGuiRecreate;
    mRequestGuiRecreate = false;
    return v;
}


bool Gui::ConsumeWindowTextureApply(std::string& outPath)
{
    if (!mWindowTexApplyRequested) return false;
    mWindowTexApplyRequested = false;
    outPath = mWindowTexPath;
    return true;
}

bool Gui::ConsumeWindowTextureClear()
{
    if (!mWindowTexClearRequested) return false;
    mWindowTexClearRequested = false;
    mWindowTexPath.clear();
    return true;
}

//reset ImGui layout
void Gui::ResetImGuiLayout()
{
    ImGuiIO& io = ImGui::GetIO();
    if (!io.IniFilename || io.IniFilename[0] == '\0')
        return;

    std::error_code ec;
    std::filesystem::remove(io.IniFilename, ec);

    //flags
    mSkipDockspaceNextFrame = true;              
    mForceMainWindowDefaultNextFrame = true;     
    mForceSettingsWindowDefaultNextFrame = true;
    mRequestGuiRecreate = true;                  
}



//Ui scaling
void Gui::ApplyUiScale()
{
    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = mUiScale;
}

//standard start for ImGui frame sequence
void Gui::BeginFrameGUI()
{
    if(!mInitialized) return;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();

    // apply UI scaling before NewFrame
    ApplyUiScale();

    ImGui::NewFrame();
}

void Gui::DrawGUI()
{
    
    if(!mInitialized) return;

    //docking config
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        static ImGuiDockNodeFlags dockspace_flags =
            ImGuiDockNodeFlags_PassthruCentralNode |
            ImGuiDockNodeFlags_NoDockingInCentralNode;

        ImGuiWindowFlags host_window_flags =
            ImGuiWindowFlags_NoDocking |
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus |
            ImGuiWindowFlags_NoBackground; 

        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGui::Begin("DockSpaceHost", nullptr, host_window_flags);
        ImGui::PopStyleVar(3);

        ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

        ImGui::End();
    }
    else if (mSkipDockspaceNextFrame)
    {
        mSkipDockspaceNextFrame = false;
    }
    //set default size and position of the GUI. original was 350x500
    if (mForceMainWindowDefaultNextFrame)
    {
        ImGui::SetNextWindowSize(ImVec2(600, 700), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_Always);

        ImGui::SetNextWindowDockID(0, ImGuiCond_Always);

        mForceMainWindowDefaultNextFrame = false;
    }
    else
    {
        ImGui::SetNextWindowSize(ImVec2(600, 700), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
    }


    //miscellaneous
    static int uiSeed = 1337;
    static float uiGridness = 0.7f;
    static float uiGridAngleStep = 90.0f;
    static const float angleSteps[] = {15.0f, 30.0f, 45.0f, 60.0f, 90.0f};
    static int angleStepIndex = 4;

    //global / city
    static float uiCityRadius = 40.0f;
    static float uiSeedJitterDeg = 25.0f;
    static float uiBranchTurnDeg = 60.0f;
    static int uiMaxIterations = 6000;

    //highways
    static int uiInitialRays = 4;
    static int uiMaxHighwaySegments = 50;
    static float uiHighwayBranchProbability = 0.15f;
    static float uiHighwayLength = 12.0f;
    static float uiHighwayPriorityWeight = 2.0f;

    //streets
    static int uiMaxStreetSegments = 750;
    static float uiStreetBranchProbability = 0.55f;
    static float uiStreetFromHighwayChance = 0.85f;
    static float uiStreetLength = 4.0f;
    static float uiStreetPriorityWeight = 1.0f;

    //loops
    static float uiLoopCloseChance = 0.10f;
    static float uiLoopCloseRadius = 18.0f;

    
    //title of the menu
    ImGui::Begin("ProceduralCityGenerator");





        if (ImGui::Button("Generate")) {
        
        mGenerateRequested = true;
        //mShowBlocks = true;

        //seed + grid
        mRoadParams.seed = (unsigned int)uiSeed;
        mRoadParams.gridness = uiGridness;
        mRoadParams.gridAngleStepDeg = uiGridAngleStep;

        //global / city
        mRoadParams.cityRadius = uiCityRadius;
        mRoadParams.seedJitterDeg = uiSeedJitterDeg;
        mRoadParams.branchTurnDeg = uiBranchTurnDeg;
        mRoadParams.maxIterations = uiMaxIterations;

        //highways
        mRoadParams.initialRays = uiInitialRays;
        mRoadParams.maxHighwaySegments = uiMaxHighwaySegments;
        mRoadParams.branchProbabilityHighway = uiHighwayBranchProbability;
        mRoadParams.highwayLength = uiHighwayLength;
        mRoadParams.highwayPriorityWeight = uiHighwayPriorityWeight;

        //streets
        mRoadParams.maxStreetSegments = uiMaxStreetSegments;
        mRoadParams.branchProbabilityStreet = uiStreetBranchProbability;
        mRoadParams.streetFromHighwayChance = uiStreetFromHighwayChance;
        mRoadParams.streetLength = uiStreetLength;
        mRoadParams.streetPriorWeight = uiStreetPriorityWeight;

        //loops
        mRoadParams.loopCloseChance = uiLoopCloseChance;
        mRoadParams.loopCloseRadius = uiLoopCloseRadius;

        //safety clamps
        mRoadParams.cityRadius = std::clamp(mRoadParams.cityRadius, 5.0f, 500.0f);
        mRoadParams.maxIterations = std::clamp(mRoadParams.maxIterations, 100, 200000);
        mRoadParams.initialRays = std::clamp(mRoadParams.initialRays, 1, 32);
        //mRoadParams.maxHighwaySegments = std::clamp(mRos = std::clamp(mRoadParams.maxHighwaySegments, 0, mRoadParams.maxSegments);
        //mRoadParams.maxStreetSegmentadParams.maxStreetSegments, 0, mRoadParams.maxSegments);
        mRoadParams.branchProbabilityHighway = std::clamp(mRoadParams.branchProbabilityHighway, 0.0f, 1.0f);
        mRoadParams.branchProbabilityStreet = std::clamp(mRoadParams.branchProbabilityStreet, 0.0f, 1.0f);
        mRoadParams.streetFromHighwayChance = std::clamp(mRoadParams.streetFromHighwayChance, 0.0f, 1.0f);
        mRoadParams.loopCloseChance = std::clamp(mRoadParams.loopCloseChance, 0.0f, 1.0f);

    }
    ImGui::SameLine();
    if(ImGui::Button("Reset to defaults"))
    {
        //default params
        road::RoadParams d;
        
        uiSeed = (int)d.seed;
        uiGridness = d.gridness;

        //grid angle step
        {
            angleStepIndex = 0;
            for (int i = 0; i < 5; i++)
            {
                if (angleSteps[i] == d.gridAngleStepDeg) { angleStepIndex = i; break; }
            }
            uiGridAngleStep = angleSteps[angleStepIndex];
        }

        //city / global
        uiCityRadius = d.cityRadius;
        uiSeedJitterDeg = d.seedJitterDeg;
        uiBranchTurnDeg = d.branchTurnDeg;
        uiMaxIterations = d.maxIterations;

        //highways
        uiInitialRays = d.initialRays;
        uiMaxHighwaySegments = d.maxHighwaySegments;
        uiHighwayLength = d.highwayLength;
        uiHighwayPriorityWeight = d.highwayPriorityWeight;

        //streets
        uiMaxStreetSegments = d.maxStreetSegments;
        uiStreetBranchProbability = d.branchProbabilityStreet;
        uiStreetFromHighwayChance = d.streetFromHighwayChance;
        uiStreetLength = d.streetLength;
        uiStreetPriorityWeight = d.streetPriorWeight;

        //loops
        uiLoopCloseChance = d.loopCloseChance;
        uiLoopCloseRadius = d.loopCloseRadius;
    }

    ImGui::SameLine();
    if(ImGui::Button("Reset Camera"))
    {
        mWantsResetCamera = true;
    }

    ImGui::SameLine();
    if (ImGui::Button("Materials"))
    {
    mShowBuildingMaterialsWindow = !mShowBuildingMaterialsWindow;
    }
    // settings button
    ImGui::SameLine();
    if (ImGui::Button(u8"⚙ Settings"))
    {
        mShowSettingsWindow = !mShowSettingsWindow;
    }


    if (mLastGenerationMs >= 0.0)
    {
        ImGui::Text("Generation time: %.2f ms", mLastGenerationMs);
    }

    ImGui::Separator();

    //checkboxes
    ImGui::Checkbox("3D mode", &mEnable3D);
    ImGui::SameLine();
    ImGui::Checkbox("Render Roofs##RenderRoofs", &mRenderBuildingsRoofs);
    ImGui::SameLine();
    ImGui::Checkbox("Render Windows##RenderWindows", &mRenderBuildingsWindows);

    ImGui::Separator();

    ImGui::SetNextItemOpen(false, ImGuiCond_FirstUseEver);
    if (ImGui::CollapsingHeader("Outline Toggle", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Show Roads", &mShowRoadLines);

        ImGui::Checkbox("Show Lots", &mShowLotDebug);

        ImGui::Checkbox("Show Sidewalks", &mShowSidewalks);

        ImGui::Checkbox("Show Gardens", &mShowGardens);

        ImGui::Checkbox("Show Footprints", &mShowFootprints);
    }

    ImGui::Separator();


    ImGui::InputInt("Seed", &uiSeed);
    ImGui::SameLine();
    if (ImGui::Button("Randomize"))
    {
        uiSeed = (int)std::chrono::high_resolution_clock::now().time_since_epoch().count();
    }


    ImGui::SetNextItemOpen(false, ImGuiCond_FirstUseEver);
    if (ImGui::CollapsingHeader("Export Settings", ImGuiTreeNodeFlags_DefaultOpen))
    {

        // format
        const char* fmtItems[] = { "Wavefront OBJ (.obj)" };
        int fmtIndex = 0; // only OBJ for now
        ImGui::Combo("Format", &fmtIndex, fmtItems, IM_ARRAYSIZE(fmtItems));
        mExportFormat = ExportFormat::OBJ;

        // inputs
        ImGui::InputText("File name", mExportBaseName, IM_ARRAYSIZE(mExportBaseName));
        ImGui::InputText("Directory", mExportDir, IM_ARRAYSIZE(mExportDir));

        ImGui::SameLine();
        auto GetDefaultStartDir = []() -> std::string
        {
            if (const char* home = std::getenv("HOME"))
                return std::string(home);

            // fallback if home isn't set
            return std::filesystem::current_path().string();
        };

        if (ImGui::Button("Browse..."))
        {
            IGFD::FileDialogConfig config;

            std::string start;
            if (mExportDir[0] != '\0')
                start = mExportDir;
            else if (const char* home = std::getenv("HOME"))
                start = home;
            else
                start = std::filesystem::current_path().string();

            config.path = start;
            config.flags = ImGuiFileDialogFlags_Modal;

            ImGuiFileDialog::Instance()->OpenDialog(
                "ChooseExportDir",
                "Choose Export Folder",
                ".",       
                config
            );
        }



        ImGui::SetNextWindowSize(ImVec2(900, 550), ImGuiCond_FirstUseEver);

        if (ImGuiFileDialog::Instance()->Display("ChooseExportDir"))
        {
            if (ImGuiFileDialog::Instance()->IsOk())
            {
                std::string pickedDir;
                std::string pickedName;

                // directory 
                auto selection = ImGuiFileDialog::Instance()->GetSelection();
                if (!selection.empty())
                {
                    const std::string pickedPath = selection.begin()->second;
                    std::filesystem::path p(pickedPath);

                    if (std::filesystem::exists(p) && std::filesystem::is_directory(p))
                        pickedDir = p.string();
                    else
                        pickedDir = p.parent_path().string();

                    if (p.has_filename() && (!std::filesystem::is_directory(p)))
                        pickedName = p.stem().string();
                }
                else
                {
                    pickedDir = ImGuiFileDialog::Instance()->GetCurrentPath();
                }

                std::string typed = ImGuiFileDialog::Instance()->GetCurrentFileName();
                if (!typed.empty())
                    pickedName = typed;

                if (!pickedName.empty())
                {
                    std::filesystem::path np(pickedName);
                    if (np.has_extension())
                        pickedName = np.stem().string();
                }

                // apply to UI fields
                if (!pickedDir.empty())
                    std::snprintf(mExportDir, IM_ARRAYSIZE(mExportDir), "%s", pickedDir.c_str());

                if (!pickedName.empty())
                    std::snprintf(mExportBaseName, IM_ARRAYSIZE(mExportBaseName), "%s", pickedName.c_str());
            }

            ImGuiFileDialog::Instance()->Close();
        }




        bool hasDir  = (mExportDir[0] != '\0');
        bool hasName = (mExportBaseName[0] != '\0');

        if (!hasDir)
            ImGui::TextColored(ImVec4(1, 0.6f, 0.2f, 1), "Pick a folder first.");
        if (!hasName)
            ImGui::TextColored(ImVec4(1, 0.6f, 0.2f, 1), "Enter a file name.");


        // export trigger button
        if (ImGui::Button("Export##ExportButton"))
        {
            if (hasDir && hasName)
                mExportRequested = true;
        }

        ImGui::SameLine();
        ImGui::TextUnformatted("Building Export Groups");
        ImGui::Checkbox("Buildings Base##ExpBase", &mExportBuildingsBase);
        ImGui::SameLine();
        ImGui::Checkbox("Roofs##ExpRoofs", &mExportBuildingsRoofs);
        ImGui::SameLine();
        ImGui::Checkbox("Windows##ExpWindows", &mExportBuildingsWindows);


        ImGui::Text("DEBUG: hasDir=%d hasName=%d requested=%d",
            (int)hasDir, (int)hasName, (int)mExportRequested);
        ImGui::Text("DEBUG: dir='%s'", mExportDir);
        ImGui::Text("DEBUG: name='%s'", mExportBaseName);



        // status line
        if (mHasLastExport)
        {
            if (mLastExportOk)
            {
                ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.2f, 1.0f),
                                "Exported: %s", mLastExportPath.c_str());
            }
            else
            {
                ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f),
                                "Export failed (check folder permissions / path)");
                ImGui::Text("Dir: %s", mExportDir);
                ImGui::Text("Name: %s", mExportBaseName);
            }
        }

    }

    // city / global controls
    ImGui::SetNextItemOpen(false, ImGuiCond_FirstUseEver);
    if (ImGui::CollapsingHeader("City / Global", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SliderFloat("City Radius", &uiCityRadius, 10.0f, 200.0f, "%.1f");
        ImGui::SliderInt("Max Iterations", &uiMaxIterations, 500, 50000);
    }

    //building footprint
    ImGui::SetNextItemOpen(false, ImGuiCond_FirstUseEver);
    if (ImGui::CollapsingHeader("Building Footprint", ImGuiTreeNodeFlags_DefaultOpen))
    {
    ImGui::Separator();
    ImGui::SliderFloat("Building Setback Front", &mLotParams.buildingSetbackFront, 0.0f, 2.0f, "%.2f");
    ImGui::SliderFloat("Building Setback Side",  &mLotParams.buildingSetbackSide,  0.0f, 2.0f, "%.2f");
    ImGui::SliderFloat("Building Setback Back",  &mLotParams.buildingSetbackBack,  0.0f, 2.0f, "%.2f");
    ImGui::SliderFloat("Coverage Min",  &mLotParams.buildingCoverageMin,  0.05f, 0.9f, "%.2f");
    ImGui::SliderFloat("Coverage Max",  &mLotParams.buildingCoverageMax,  0.05f, 0.9f, "%.2f");
    }
    
    //grid controls
    ImGui::SetNextItemOpen(false, ImGuiCond_FirstUseEver);
    if (ImGui::CollapsingHeader("Grid", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SliderFloat("Gridness", &uiGridness, 0.0f, 1.0f);
        ImGui::Text("Grid Angle Step (deg)");
        ImGui::SameLine();
        ImGui::Combo("##GridAngleStep", &angleStepIndex, " 15\0 30\0 45\0 60\0 90\0\0");
        uiGridAngleStep = angleSteps[angleStepIndex];
    }
    // highway controls
    ImGui::SetNextItemOpen(false, ImGuiCond_FirstUseEver);
    if (ImGui::CollapsingHeader("Highways", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SliderInt("Initial Highway Rays", &uiInitialRays, 2, 4);
        ImGui::SliderInt("Max Highways", &uiMaxHighwaySegments, 0, 400);
        ImGui::SliderFloat("Highway Branch Prob", &uiHighwayBranchProbability, 0.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("Highway Length", &uiHighwayLength, 2.0f, 50.0f, "%.1f");
        ImGui::SliderFloat("Highway Priority Weight", &uiHighwayPriorityWeight, 0.1f, 6.0f, "%.2f");
    }
    //street controls
    ImGui::SetNextItemOpen(false, ImGuiCond_FirstUseEver);
    if (ImGui::CollapsingHeader("Streets", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SliderInt("Max Street", &uiMaxStreetSegments, 0,8000);
        ImGui::SliderFloat("Street Branch Prob", &uiStreetBranchProbability, 0.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("Street From Highway Chance", &uiStreetFromHighwayChance, 0.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("Street Length", &uiStreetLength, 1.0f, 20.0f, "%.1f");
        ImGui::SliderFloat("Street Priority Weight", &uiStreetPriorityWeight, 0.1f, 6.0f, "%.2f");
    }


    //lots/urbanization
    ImGui::SetNextItemOpen(false, ImGuiCond_FirstUseEver);
    if (ImGui::CollapsingHeader("Lots / Urbanization", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SliderFloat("Global Urbanization", &mLotParams.globalUrbanization, 0.0f, 1.0f, "%.2f");

        ImGui::Separator();
        ImGui::SliderFloat("Urban Threshold", &mLotParams.urbanThreshold, 0.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("Suburban Threshold", &mLotParams.suburbanThreshold, 0.0f, 1.0f, "%.2f");

        ImGui::Separator();
        ImGui::SliderFloat("Min Lot Area For Garden", &mLotParams.minLotAreaForGarden, 0.0f, 50.0f, "%.1f");
        ImGui::SliderFloat("Garden Back Ratio", &mLotParams.gardenBackRatio, 0.0f, 0.9f, "%.2f");
        ImGui::SliderFloat("Min Garden Depth", &mLotParams.minGardenDepth, 0.0f, 5.0f, "%.2f");
    }

    ImGui::Separator();

    if (ImGui::Button("Quit")) {
        mQuitRequested = true;
    }
    
    ImGui::End();
    
    if (mShowSettingsWindow)
    {
        if (mForceSettingsWindowDefaultNextFrame)
        {
            ImGui::SetNextWindowSize(ImVec2(360, 200), ImGuiCond_Always);
            ImGui::SetNextWindowPos(ImVec2(650, 40), ImGuiCond_Always); 
            ImGui::SetNextWindowDockID(0, ImGuiCond_Always);           
            mForceSettingsWindowDefaultNextFrame = false;
        }
        else
        {
            ImGui::SetNextWindowSize(ImVec2(360, 200), ImGuiCond_FirstUseEver);
        }

        if (ImGui::Begin("Settings", &mShowSettingsWindow))
        {
            ImGui::TextUnformatted("Display");
            bool wantFullscreen = mFullscreen;
            if (ImGui::Checkbox("Fullscreen", &wantFullscreen))
            {
                SetFullscreen(wantFullscreen);
            }

            ImGui::Separator();
            ImGui::TextUnformatted("UI");
            if (ImGui::SliderFloat("UI Scale", &mUiScale, 0.75f, 1.50f, "%.2f"))
            {
                ApplyUiScale();
            }

            ImGui::Separator();
            ImGui::TextUnformatted("Layout");
            if (ImGui::Button("Reset Layout"))
            {
                ResetImGuiLayout();
            }
        }

        ImGui::End();
    }

    if (mShowBuildingMaterialsWindow)
    {
        ImGui::SetNextWindowSize(ImVec2(520, 320), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Materials", &mShowBuildingMaterialsWindow))
        {
            // apply Defaults 
            if (ImGui::Button("Apply Defaults"))
            {
                const std::filesystem::path assetsRoot = FindAssetsRoot();
                const std::filesystem::path texRoot = assetsRoot.empty() ? std::filesystem::path{} : (assetsRoot / "textures");

                auto SetSlot = [&](TextureSlot& slot, const std::filesystem::path& p)
                {
                    const std::string s = ToStringPath(p);
                    if (s.empty()) return;
                    std::snprintf(slot.importedPath, IM_ARRAYSIZE(slot.importedPath), "%s", s.c_str());
                    slot.isImported = true;
                    slot.isApplied = true;
                    slot.requestApply = true;
                    slot.requestClear = false;
                };

                // building walls (Urban/Suburban/Rural)
                SetSlot(mBuildingTextures[0], texRoot / "Building" / "UrbanWalls.jpg");
                SetSlot(mBuildingTextures[1], texRoot / "Building" / "SuburbanWalls.jpg");
                SetSlot(mBuildingTextures[2], texRoot / "Building" / "RuralWalls.jpg");

                // roofs (Urban/Suburban/Rural)
                SetSlot(mRoofTextures[0], texRoot / "Roof" / "UrbanRoofs.jpg");
                SetSlot(mRoofTextures[1], texRoot / "Roof" / "SuburbanRoofs.jpg");
                SetSlot(mRoofTextures[2], texRoot / "Roof" / "RuralRoofs.jpg");

                // scene (single textures)
                SetSlot(mRoadTexture,     texRoot / "Road" / "Asphalt.jpg");
                SetSlot(mSidewalkTexture, texRoot / "Sidewalk" / "SidewalkTex.jpg");
                SetSlot(mGroundTexture,   texRoot / "Ground" / "GroundTex.jpg");

                // windows (single texture)
                const std::filesystem::path winP = texRoot / "Window" / "BuildingWindows.png";
                const std::string winS = ToStringPath(winP);
                if (!winS.empty())
                {
                    mWindowTexPath = winS;
                    mWindowTexApplyRequested = true;
                    mWindowTexClearRequested = false;
                }
            }

            ImGui::SetNextItemOpen(false, ImGuiCond_FirstUseEver);
            if (ImGui::CollapsingHeader("Wall Textures", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto DrawZone = [&](road::LotZone zone, const char* label, const char* dialogKey)
                {
                    TextureSlot& slot = (zone == road::LotZone::Urban)
                        ? mBuildingTextures[0]
                        : (zone == road::LotZone::Suburban)
                            ? mBuildingTextures[1]
                            : mBuildingTextures[2];

                    DrawTextureSlotUI(slot, label, dialogKey, "Choose Texture");
                };


                DrawZone(road::LotZone::Urban, "Urban", "ChooseUrbanTex");
                DrawZone(road::LotZone::Suburban, "Suburban", "ChooseSuburbanTex");
                DrawZone(road::LotZone::Rural, "Rural", "ChooseRuralTex");
            }

            ImGui::SetNextItemOpen(false, ImGuiCond_FirstUseEver);
            if (ImGui::CollapsingHeader("Window Textures", ImGuiTreeNodeFlags_DefaultOpen))
            {

                ImGui::TextUnformatted("Windows");

                ImGui::Text("Imported:");
                ImGui::TextWrapped("%s", mWindowTexPath.empty() ? "(none)" : mWindowTexPath.c_str());

                if (ImGui::Button("Import##WinTex"))
                {
                    IGFD::FileDialogConfig config;
                    config.path = ".";
                    ImGuiFileDialog::Instance()->OpenDialog("WinTexDlg", "Choose Window Texture",
                                                            ".png,.jpg,.jpeg", config);
                }
                ImGui::SameLine();

                bool canApplyWin = !mWindowTexPath.empty();
                if (!canApplyWin) ImGui::BeginDisabled();
                if (ImGui::Button("Apply##WinTex"))
                {
                    mWindowTexApplyRequested = true;
                }
                if (!canApplyWin) ImGui::EndDisabled();

                ImGui::SameLine();
                if (ImGui::Button("Clear##WinTex"))
                {
                    mWindowTexClearRequested = true;
                }

                if (ImGuiFileDialog::Instance()->Display("WinTexDlg"))
                {
                    if (ImGuiFileDialog::Instance()->IsOk())
                    {
                        mWindowTexPath = ImGuiFileDialog::Instance()->GetFilePathName();
                    }
                    ImGuiFileDialog::Instance()->Close();
                }
            }


            ImGui::SetNextItemOpen(false, ImGuiCond_FirstUseEver);
            if (ImGui::CollapsingHeader("Roof Textures", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto DrawRoofZone = [&](road::LotZone zone, const char* label, const char* dialogKey)
                {
                    TextureSlot& slot = (zone == road::LotZone::Urban)
                        ? mRoofTextures[0]
                        : (zone == road::LotZone::Suburban)
                            ? mRoofTextures[1]
                            : mRoofTextures[2];

                    DrawTextureSlotUI(slot, label, dialogKey, "Choose Roof Texture");
                };


                DrawRoofZone(road::LotZone::Urban, "Urban Roof", "ChooseUrbanRoofTex");
                DrawRoofZone(road::LotZone::Suburban, "Suburban Roof", "ChooseSuburbanRoofTex");
                DrawRoofZone(road::LotZone::Rural, "Rural Roof", "ChooseRuralRoofTex");
            }


            ImGui::SetNextItemOpen(false, ImGuiCond_FirstUseEver);
            if (ImGui::CollapsingHeader("Scene Textures", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto DrawSingleSlot = [&](TextureSlot& slot, const char* header, const char* dialogKey, const char* dialogTitle)
                {
                    DrawTextureSlotUI(slot, header, dialogKey, dialogTitle);
                };


                DrawSingleSlot(mRoadTexture,     "Roads",     "ChooseRoadTex",     "Choose Road Texture");
                DrawSingleSlot(mSidewalkTexture, "Sidewalks", "ChooseSidewalkTex", "Choose Sidewalk Texture");
                DrawSingleSlot(mGroundTexture,   "Ground",    "ChooseGroundTex",   "Choose Ground Texture");
            }
        }
        ImGui::End();
    }
}



void Gui::EndFrameGUI () 
{
    if (!mInitialized) return;

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow* backupCurrentContext = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backupCurrentContext);
    }

}

void Gui::ShutdownGUI()
{
    if (!mInitialized) return;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    mInitialized = false;
    mWindow = nullptr;
}

bool Gui::WantsGenerate()
{
    bool v = mGenerateRequested;
    mGenerateRequested = false;
    return v;
}

bool Gui::WantsQuit()
{
    bool v = mQuitRequested;
    mQuitRequested = false;
    return v;
}

bool Gui::ConsumeResetCamera()
{
    bool v = mWantsResetCamera;
    mWantsResetCamera = false;
    return v;
}

bool Gui::ConsumeExportRequest(std::string& outDir,
                               std::string& outBaseName,
                               ExportFormat& outFmt)
{
    if (!mExportRequested)
        return false;

    mExportRequested = false; // consume (one-shot)

    outDir = mExportDir;
    outBaseName = mExportBaseName;
    outFmt = mExportFormat;

    return true;
}

bool Gui::ConsumeBuildingTextureApply(road::LotZone zone, std::string& outPath)
{
    auto SlotFor = [&](road::LotZone z) -> TextureSlot&
    {
        switch (z)
        {
            case road::LotZone::Urban:    return mBuildingTextures[0];
            case road::LotZone::Suburban: return mBuildingTextures[1];
            case road::LotZone::Rural:    return mBuildingTextures[2];
            default:                      return mBuildingTextures[1];
        }
    };

    TextureSlot& slot = SlotFor(zone);
    if (!slot.requestApply) return false;

    slot.requestApply = false;
    outPath = slot.importedPath;
    return !outPath.empty();
}

bool Gui::ConsumeBuildingTextureClear(road::LotZone zone)
{
    auto SlotFor = [&](road::LotZone z) -> TextureSlot&
    {
        switch (z)
        {
            case road::LotZone::Urban:    return mBuildingTextures[0];
            case road::LotZone::Suburban: return mBuildingTextures[1];
            case road::LotZone::Rural:    return mBuildingTextures[2];
            default:                      return mBuildingTextures[1];
        }
    };

    TextureSlot& slot = SlotFor(zone);
    if (!slot.requestClear) return false;

    slot.requestClear = false;
    return true;
}

bool Gui::ConsumeRoofTextureApply(road::LotZone zone, std::string& outPath)
{
    TextureSlot& slot = (zone == road::LotZone::Urban)
        ? mRoofTextures[0]
        : (zone == road::LotZone::Suburban)
            ? mRoofTextures[1]
            : mRoofTextures[2];

    if (!slot.requestApply) return false;
    slot.requestApply = false;

    outPath = slot.importedPath;
    return !outPath.empty();
}

bool Gui::ConsumeRoofTextureClear(road::LotZone zone)
{
    TextureSlot& slot = (zone == road::LotZone::Urban)
        ? mRoofTextures[0]
        : (zone == road::LotZone::Suburban)
            ? mRoofTextures[1]
            : mRoofTextures[2];

    if (!slot.requestClear) return false;
    slot.requestClear = false;
    return true;
}

bool Gui::ConsumeRoadTextureApply(std::string& outPath)
{
    if (!mRoadTexture.requestApply) return false;
    mRoadTexture.requestApply = false;
    outPath = mRoadTexture.importedPath;
    return !outPath.empty();
}

bool Gui::ConsumeRoadTextureClear()
{
    if (!mRoadTexture.requestClear) return false;
    mRoadTexture.requestClear = false;
    return true;
}

bool Gui::ConsumeSidewalkTextureApply(std::string& outPath)
{
    if (!mSidewalkTexture.requestApply) return false;
    mSidewalkTexture.requestApply = false;
    outPath = mSidewalkTexture.importedPath;
    return !outPath.empty();
}

bool Gui::ConsumeSidewalkTextureClear()
{
    if (!mSidewalkTexture.requestClear) return false;
    mSidewalkTexture.requestClear = false;
    return true;
}

bool Gui::ConsumeGroundTextureApply(std::string& outPath)
{
    if (!mGroundTexture.requestApply) return false;
    mGroundTexture.requestApply = false;
    outPath = mGroundTexture.importedPath;
    return !outPath.empty();
}

bool Gui::ConsumeGroundTextureClear()
{
    if (!mGroundTexture.requestClear) return false;
    mGroundTexture.requestClear = false;
    return true;
}

void Gui::SetLastExportResult(bool ok, const std::string& fullPath)
{
    mHasLastExport = true;
    mLastExportOk = ok;
    mLastExportPath = fullPath;
}





