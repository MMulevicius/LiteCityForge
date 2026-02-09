#include "Gui.h"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "ImGuiFileDialog.h"
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <algorithm>
#include <filesystem>

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

bool Gui::ConsumeGuiRecreateRequest()
{
    bool v = mRequestGuiRecreate;
    mRequestGuiRecreate = false;
    return v;
}

void Gui::ResetImGuiLayout()
{
    ImGuiIO& io = ImGui::GetIO();
    if (!io.IniFilename || io.IniFilename[0] == '\0')
        return;

    std::error_code ec;
    std::filesystem::remove(io.IniFilename, ec);

    mSkipDockspaceNextFrame = true;              
    mForceMainWindowDefaultNextFrame = true;     
    mForceSettingsWindowDefaultNextFrame = true;
    mRequestGuiRecreate = true;                  
}




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
    ImGui::Begin("LiteCityForge");
    // settings button
    ImGui::SameLine();
    if (ImGui::Button(u8"⚙ Settings"))
    {
        mShowSettingsWindow = !mShowSettingsWindow;
    }


    ImGui::Separator();

    //checkboxes
    ImGui::Checkbox("3D mode", &mEnable3D);
    ImGui::SameLine();

    ImGui::Checkbox("Show Lots", &mShowLotDebug);
    ImGui::SameLine();

    ImGui::Checkbox("Show Sidewalks", &mShowSidewalks);
    ImGui::SameLine();

    ImGui::Checkbox("Show Gardens", &mShowGardens);
    ImGui::SameLine();

    ImGui::Checkbox("Show Footprints", &mShowFootprints);

    ImGui::Separator();

    //generate button
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
    if (mLastGenerationMs >= 0.0)
    {
        ImGui::Text("Generation time: %.2f ms", mLastGenerationMs);
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

        // format (future-proof)
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

                auto selection = ImGuiFileDialog::Instance()->GetSelection(); 

                if (!selection.empty())
                {
                    const std::string pickedPath = selection.begin()->second;
                    std::filesystem::path p(pickedPath);

                    if (std::filesystem::exists(p) && std::filesystem::is_directory(p))
                        pickedDir = p.string();
                    else
                        pickedDir = p.parent_path().string();
                }
                else
                {
                    // Fallback
                    pickedDir = ImGuiFileDialog::Instance()->GetCurrentPath();
                }

                if (!pickedDir.empty())
                {
                    std::snprintf(mExportDir, IM_ARRAYSIZE(mExportDir), "%s", pickedDir.c_str());
                }
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
        //ImGui::SliderFloat("Seed Jitter (deg)", &uiSeedJitterDeg, 0.0f, 60.0f, "%.1f");
        //ImGui::SliderFloat("Branch Turn (deg)", &uiBranchTurnDeg, 15.0f, 90.0f, "%.1f");
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
    //ImGui::SliderFloat("Setback Jitter",&mLotParams.buildingSetBackJitter,0.0f, 0.5f, "%.2f");
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

    //loop controls
    // if (ImGui::CollapsingHeader("Loops / Parcels", ImGuiTreeNodeFlags_DefaultOpen))
    // {
    //     ImGui::SliderFloat("Loop Close Chance", &uiLoopCloseChance, 0.0f, 1.0f, "%.2f");
    //     ImGui::SliderFloat("Loop Close Radius", &uiLoopCloseRadius, 1.0f, 60.0f, "%.1f");
    // }

    //lots/urbanization
    ImGui::SetNextItemOpen(false, ImGuiCond_FirstUseEver);
    if (ImGui::CollapsingHeader("Lots / Urbanization", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SliderFloat("Global Urbanization", &mLotParams.globalUrbanization, 0.0f, 1.0f, "%.2f");
        //ImGui::SliderFloat("Global Bias Strength", &mLotParams.globalBiasStrength, 0.0f, 1.0f, "%.2f");

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

void Gui::SetLastExportResult(bool ok, const std::string& fullPath)
{
    mHasLastExport = true;
    mLastExportOk = ok;
    mLastExportPath = fullPath;
}





