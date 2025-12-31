#include "Gui.h"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <chrono>
#include <algorithm>

bool Gui::Initialize_GUI(GLFWwindow *window, const char* glslVersion)
{
    
    //store the GLFW window pointer for further use
    mWindow = window;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO(); (void)io;

    //set theme is dark
    ImGui::StyleColorsDark();


    if(!ImGui_ImplGlfw_InitForOpenGL(mWindow, true)) return false;
    if(!ImGui_ImplOpenGL3_Init(glslVersion)) return false;

    //making sure that the GUI is ready
    mInitialized = true;
    return true;
}

void Gui::BeginFrameGUI()
{
    if(!mInitialized) return;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Gui::DrawGUI()
{
    
    if(!mInitialized) return;
    //set default size and position of the GUI
    ImGui::SetNextWindowSize(ImVec2(350, 500), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);

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

    

    ImGui::Begin("LiteCityForge");
    ImGui::Separator();

    ImGui::Checkbox("3D mode", &mEnable3D);

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

    ImGui::Separator();

    ImGui::InputInt("Seed", &uiSeed);
    ImGui::SameLine();
    if (ImGui::Button("Randomize"))
    {
        uiSeed = (int)std::chrono::high_resolution_clock::now().time_since_epoch().count();
    }
    // city / global controls
    if (ImGui::CollapsingHeader("City / Global", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SliderFloat("City Radius", &uiCityRadius, 10.0f, 200.0f, "%.1f");
        ImGui::SliderFloat("Seed Jitter (deg)", &uiSeedJitterDeg, 0.0f, 60.0f, "%.1f");
        ImGui::SliderFloat("Branch Turn (deg)", &uiBranchTurnDeg, 15.0f, 90.0f, "%.1f");
        ImGui::SliderInt("Max Iterations", &uiMaxIterations, 500, 50000);
    }

    //building
    ImGui::Separator();
    ImGui::SliderFloat("Building Setback Front", &mLotParams.buildingSetbackFront, 0.0f, 2.0f, "%.2f");
    ImGui::SliderFloat("Building Setback Side",  &mLotParams.buildingSetbackSide,  0.0f, 2.0f, "%.2f");
    ImGui::SliderFloat("Building Setback Back",  &mLotParams.buildingSetbackBack,  0.0f, 2.0f, "%.2f");
    ImGui::SliderFloat("Setback Jitter",&mLotParams.buildingSetBackJitter,0.0f, 0.5f, "%.2f");
    ImGui::SliderFloat("Coverage Min",  &mLotParams.buildingCoverageMin,  0.05f, 0.9f, "%.2f");
    ImGui::SliderFloat("Coverage Max",  &mLotParams.buildingCoverageMax,  0.05f, 0.9f, "%.2f");
    
    //grid controls
    if (ImGui::CollapsingHeader("Grid", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SliderFloat("Gridness", &uiGridness, 0.0f, 1.0f);
        ImGui::Text("Grid Angle Step (deg)");
        ImGui::SameLine();
        ImGui::Combo("##GridAngleStep", &angleStepIndex, " 15\0 30\0 45\0 60\0 90\0\0");
        uiGridAngleStep = angleSteps[angleStepIndex];
    }
    // highway controls
    if (ImGui::CollapsingHeader("Highways", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SliderInt("Initial Highway Rays", &uiInitialRays, 1, 12);
        ImGui::SliderInt("Max Highways", &uiMaxHighwaySegments, 0, 400);
        ImGui::SliderFloat("Highway Branch Prob", &uiHighwayBranchProbability, 0.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("Highway Length", &uiHighwayLength, 2.0f, 50.0f, "%.1f");
        ImGui::SliderFloat("Highway Priority Weight", &uiHighwayPriorityWeight, 0.1f, 6.0f, "%.2f");
    }
    //street controls
    if (ImGui::CollapsingHeader("Streets", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SliderInt("Max Street", &uiMaxStreetSegments, 0,8000);
        ImGui::SliderFloat("Street Branch Prob", &uiStreetBranchProbability, 0.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("Street From Highway Chance", &uiStreetFromHighwayChance, 0.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("Street Length", &uiStreetLength, 1.0f, 20.0f, "%.1f");
        ImGui::SliderFloat("Street Priority Weight", &uiStreetPriorityWeight, 0.1f, 6.0f, "%.2f");
    }

    //loop controls
    if (ImGui::CollapsingHeader("Loops / Parcels", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SliderFloat("Loop Close Chance", &uiLoopCloseChance, 0.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("Loop Close Radius", &uiLoopCloseRadius, 1.0f, 60.0f, "%.1f");
    }

    //lots/urbanization
    if (ImGui::CollapsingHeader("Lots / Urbanization", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SliderFloat("Global Urbanization", &mLotParams.globalUrbanization, 0.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("Global Bias Strength", &mLotParams.globalBiasStrength, 0.0f, 1.0f, "%.2f");

        ImGui::Separator();
        ImGui::SliderFloat("Urban Threshold", &mLotParams.urbanThreshold, 0.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("Suburban Threshold", &mLotParams.suburbanThreshold, 0.0f, 1.0f, "%.2f");

        ImGui::Separator();
        ImGui::SliderFloat("Min Lot Area For Garden", &mLotParams.minLotAreaForGarden, 0.0f, 50.0f, "%.1f");
        ImGui::SliderFloat("Garden Back Ratio", &mLotParams.gardenBackRatio, 0.0f, 0.9f, "%.2f");
        ImGui::SliderFloat("Min Garden Depth", &mLotParams.minGardenDepth, 0.0f, 5.0f, "%.2f");
    }

    ImGui::Checkbox("Show Lots", &mShowLotDebug);
    ImGui::Checkbox("Show Sidewalks", &mShowSidewalks);
    ImGui::Checkbox("Show Gardens", &mShowGardens);
    ImGui::Checkbox("Show Footprints", &mShowFootprints);

    ImGui::Separator();

    if (ImGui::Button("Quit")) {
        mQuitRequested = true;
    }
    
    ImGui::End();

}

void Gui::EndFrameGUI () 
{
    if (!mInitialized) return;

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

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
