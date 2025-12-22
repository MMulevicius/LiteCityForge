#include "Gui.h"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <chrono>

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

    //Making sure that the GUI is ready
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
    //Set default size and position of the GUI
    ImGui::SetNextWindowSize(ImVec2(350, 500), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);

    static int uiSeed = 1337;
    static float uiGridness = 0.7f;
    static float uiGridAngleStep = 90.0f;

    ImGui::Begin("LiteCityForge");

    ImGui::Separator();

    ImGui::Checkbox("3D mode", &mEnable3D);

    if (ImGui::Button("Generate")) {
        mGenerateRequested = true;

        //road::RoadParams mParams;

        mRoadParams.seed = (unsigned int)uiSeed;
        mRoadParams.gridness = uiGridness;
        mRoadParams.gridAngleStepDeg = uiGridAngleStep;

    }
    ImGui::Separator();

    ImGui::InputInt("Seed", &uiSeed);
    ImGui::SameLine();
    if (ImGui::Button("Randomize"))
    {
        uiSeed = (int)std::chrono::high_resolution_clock::now().time_since_epoch().count();
    }

    ImGui::SliderFloat("Gridness", &uiGridness, 0.0f, 1.0f);

    static const float angleSteps[] = {15.0f, 30.0f, 45.0f, 60.0f, 90.0f};
    static int angleStepIndex = 4;

    ImGui::Text("Grid Angle Step (deg)");
    ImGui::SameLine();
    ImGui::Combo("##GridAngleStep", &angleStepIndex, " 15\0 30\0 45\0 60\0 90\0\0");

    uiGridAngleStep = angleSteps[angleStepIndex];


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